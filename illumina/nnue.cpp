#include "nnue.h"

#include <incbin/incbin.h>
#include <nlohmann/json/json.hpp>

#ifdef HAS_AVX2
#include <immintrin.h>
#endif

namespace illumina {

INCTXT(_default_network, NNUE_PATH);

static const EvalNetwork* s_default_network = nullptr;

constexpr int SCALE = 400;
constexpr int Q1    = 255;
constexpr int Q2    = 64;
constexpr int Q     = L1_ACTIVATION == ActivationFunction::CReLU
                      ? (Q1 * Q2)
                      : (Q1 * Q1 * Q2);

template <Color C>
static size_t feature_index(Square square, Piece piece) {
    Color color     = piece.color();
    size_t type_idx = piece.type() - 1;

    if constexpr (C == CL_BLACK) {
        square = mirror_vertical(square);
        color  = opposite_color(color);
    }

    size_t index = 0;
    index = index * CL_COUNT + color;
    index = index * (PT_COUNT - 1) + type_idx;
    index = index * SQ_COUNT + square;
    return index;
}

void NNUE::clear() {
    // Copy all biases.
    m_accum_stack.clear();

    m_accum_stack.resize(1);
    m_accum_stack.reserve(32);
    Accumulator& accum = accumulator();

    std::copy(m_net->l1_biases.begin(), m_net->l1_biases.end(), accum.white.begin());
    std::copy(m_net->l1_biases.begin(), m_net->l1_biases.end(), accum.black.begin());
}

int NNUE::forward(Color color) const {
#ifdef HAS_AVX2
    constexpr size_t STRIDE = sizeof(__m256i) / sizeof(i16);
    __m256i sum = _mm256_setzero_si256();

    const Accumulator& accum = accumulator();

    const auto& our_accum   = color == CL_WHITE ? accum.white : accum.black;
    const auto& their_accum = color == CL_WHITE ? accum.black : accum.white;

    for (int i = 0; i < L1_SIZE / STRIDE; ++i)
    {
        __m256i accum_val;
        __m256i clamped;
        __m256i squared;

        accum_val = _mm256_load_si256(reinterpret_cast<const __m256i*>(&our_accum[i * STRIDE]));
        clamped   = _mm256_max_epi16(_mm256_min_epi16(accum_val, _mm256_set1_epi16(Q1)), _mm256_setzero_si256());
        squared   = _mm256_mullo_epi16(clamped, _mm256_load_si256(reinterpret_cast<const __m256i *>(&m_net->output_weights[i * STRIDE])));
        squared   = _mm256_madd_epi16(clamped, squared);
        sum       = _mm256_add_epi32(sum, squared);

        accum_val = _mm256_load_si256(reinterpret_cast<const __m256i*>(&their_accum[i * STRIDE]));
        clamped   = _mm256_max_epi16(_mm256_min_epi16(accum_val, _mm256_set1_epi16(Q1)), _mm256_setzero_si256());
        squared   = _mm256_mullo_epi16(clamped, _mm256_load_si256(reinterpret_cast<const __m256i *>(&m_net->output_weights[L1_SIZE + i * STRIDE])));
        squared   = _mm256_madd_epi16(clamped, squared);
        sum       = _mm256_add_epi32(sum, squared);
    }

    __m128i sum0;
    __m128i sum1;

    sum0 = _mm256_castsi256_si128(sum);
    sum1 = _mm256_extracti128_si256(sum, 1);
    sum0 = _mm_add_epi32(sum0, sum1);
    sum1 = _mm_unpackhi_epi64(sum0, sum0);
    sum0 = _mm_add_epi32(sum0, sum1);
    sum1 = _mm_shuffle_epi32(sum0, _MM_SHUFFLE(2, 3, 0, 1));
    sum0 = _mm_add_epi32(sum0, sum1);

    return (_mm_cvtsi128_si32(sum0) + m_net->output_bias) * SCALE / Q;
#else
    int sum = 0;

    const Accumulator& accum = accumulator();

    const auto& our_accum = color == CL_WHITE ? accum.white : accum.black;
    const auto& their_accum = color == CL_WHITE ? accum.black : accum.white;

    for (size_t i = 0; i < L1_SIZE; ++i) {
        int activated = std::clamp(int(our_accum[i]), 0, Q1);
        sum += activated * m_net->output_weights[i];
    }

    for (size_t i = 0; i < L1_SIZE; ++i) {
        int activated = std::clamp(int(their_accum[i]), 0, Q1);
        sum += activated * m_net->output_weights[L1_SIZE + i];
    }

    return (sum + m_net->output_bias) * SCALE / Q;
#endif
}

void NNUE::enable_feature(Square square, Piece piece) {
    Accumulator& accum = accumulator();

    size_t white_idx = feature_index<CL_WHITE>(square, piece);
    size_t black_idx = feature_index<CL_BLACK>(square, piece);

    for (size_t i = 0; i < L1_SIZE; ++i) {
        accum.white[i] += m_net->l1_weights[N_INPUTS * i + white_idx];
        accum.black[i] += m_net->l1_weights[N_INPUTS * i + black_idx];
    }
}

void NNUE::disable_feature(Square square, Piece piece) {
    Accumulator& accum = accumulator();

    size_t white_idx = feature_index<CL_WHITE>(square, piece);
    size_t black_idx = feature_index<CL_BLACK>(square, piece);

    for (size_t i = 0; i < L1_SIZE; ++i) {
        accum.white[i] -= m_net->l1_weights[N_INPUTS * i + white_idx];
        accum.black[i] -= m_net->l1_weights[N_INPUTS * i + black_idx];
    }
}

void NNUE::push_accumulator() {
    m_accum_stack.push_back(accumulator());
}

void NNUE::pop_accumulator() {
    ILLUMINA_ASSERT(m_accum_idx >= 0);
    m_accum_stack.pop_back();
}

Accumulator& NNUE::accumulator() {
    return m_accum_stack.back();
}

const Accumulator& NNUE::accumulator() const {
    return m_accum_stack.back();
}


NNUE::NNUE()
    : m_net(s_default_network) {
    clear();
}

template <typename T>
static void copy_params_from_json(const nlohmann::json& j,
                                  std::string_view json_field_name,
                                  T& arr) {
    nlohmann::json j_arr = j[json_field_name];
    for (size_t i = 0; i < arr.size(); ++i) {
        arr[i] = j_arr.at(i);
    }
}

EvalNetwork::EvalNetwork(std::istream& stream) {
    nlohmann::json j = nlohmann::json::parse(stream);

    copy_params_from_json(j, "l1_weights", l1_weights);
    copy_params_from_json(j, "l1_biases", l1_biases);
    copy_params_from_json(j, "out_weights", output_weights);

    output_bias = j.at("out_biases")[0];
}

void init_nnue() {
    std::string str = std::string(g_default_networkData, g_default_networkSize);
    std::istringstream stream(str);
    s_default_network = new EvalNetwork(stream);
}

} // illumina