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

void NNUE::clear() {
    // Copy all biases.
    std::copy(m_net->l1_biases.begin(), m_net->l1_biases.end(), m_accum.white.begin());
    std::copy(m_net->l1_biases.begin(), m_net->l1_biases.end(), m_accum.black.begin());
}

int NNUE::forward(Color color) const {
#ifdef HAS_AVX2
    constexpr size_t STRIDE = sizeof(__m256i) / sizeof(i16);
    __m256i sum = _mm256_setzero_si256();

    auto& our_accum   = color == CL_WHITE ? m_accum.white : m_accum.black;
    auto& their_accum = color == CL_WHITE ? m_accum.black : m_accum.white;

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

    auto& our_accum = color == CL_WHITE ? m_accum.white : m_accum.black;
    auto& their_accum = color == CL_WHITE ? m_accum.black : m_accum.white;

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
    update_features<1, 0>({square}, {piece}, {}, {});
}

void NNUE::disable_feature(Square square, Piece piece) {
    update_features<0, 1>({}, {}, {square}, {piece});
}

void NNUE::push_accumulator() {
    m_accum_stack.push_back(m_accum);
}

void NNUE::pop_accumulator() {
    ILLUMINA_ASSERT(!m_accum_stack.empty());

    m_accum = m_accum_stack.back();
    m_accum_stack.pop_back();
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