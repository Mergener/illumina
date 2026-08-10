#include "nnue.h"

#define INCBIN_ALIGNMENT_INDEX 5
#include <incbin/incbin.h>

#include <algorithm>
#include <cstddef>
#include <stdexcept>

#ifdef HAS_AVX2
#include <immintrin.h>
#endif

namespace illumina {

INCBIN(_default_network, NNUE_PATH);

static const EvalNetwork* s_default_network = nullptr;

constexpr int SCALE = 400;
constexpr int Q1    = 255;
constexpr int Q2    = 64;

constexpr size_t L1_WEIGHTS_BYTES = N_INPUTS * L1_SIZE * sizeof(i16);
constexpr size_t L1_BIASES_BYTES = L1_SIZE * sizeof(i16);
constexpr size_t OUTPUT_WEIGHTS_BYTES = 2 * L1_SIZE * sizeof(i16);
constexpr size_t NETWORK_PAYLOAD_BYTES = L1_WEIGHTS_BYTES
                                       + L1_BIASES_BYTES
                                       + OUTPUT_WEIGHTS_BYTES
                                       + sizeof(i16);
constexpr size_t NETWORK_OBJECT_BYTES = (NETWORK_PAYLOAD_BYTES + 31) & ~size_t(31);
constexpr size_t NETWORK_FILE_BYTES = (NETWORK_PAYLOAD_BYTES + 63) & ~size_t(63);

static_assert(offsetof(EvalNetwork, l1_weights) == 0);
static_assert(offsetof(EvalNetwork, l1_biases) == L1_WEIGHTS_BYTES);
static_assert(offsetof(EvalNetwork, output_weights) == L1_WEIGHTS_BYTES + L1_BIASES_BYTES);
static_assert(offsetof(EvalNetwork, output_bias) == L1_WEIGHTS_BYTES + L1_BIASES_BYTES + OUTPUT_WEIGHTS_BYTES);
static_assert(std::is_standard_layout_v<EvalNetwork>);
static_assert(std::is_trivially_copyable_v<EvalNetwork>);
static_assert(sizeof(EvalNetwork) == NETWORK_OBJECT_BYTES);
static_assert(sizeof(EvalNetwork) <= NETWORK_FILE_BYTES);
static_assert(alignof(EvalNetwork) <= INCBIN_ALIGNMENT);

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

    for (size_t i = 0; i < L1_SIZE / STRIDE; ++i)
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

    int output = _mm_cvtsi128_si32(sum0);
    output /= Q1;
    output += m_net->output_bias;
    return output * SCALE / (Q1 * Q2);
#else
    int sum = 0;

    auto& our_accum = color == CL_WHITE ? m_accum.white : m_accum.black;
    auto& their_accum = color == CL_WHITE ? m_accum.black : m_accum.white;

    for (size_t i = 0; i < L1_SIZE; ++i) {
        int our_activated = std::clamp(int(our_accum[i]), 0, Q1);
        our_activated *= our_activated;
        sum += our_activated * m_net->output_weights[i];

        int their_activated = std::clamp(int(their_accum[i]), 0, Q1);
        their_activated *= their_activated;
        sum += their_activated * m_net->output_weights[L1_SIZE + i];
    }

    sum /= Q1;
    sum += m_net->output_bias;
    return sum * SCALE / (Q1 * Q2);
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

void init_nnue() {
    if (g_default_networkSize != NETWORK_FILE_BYTES) {
        throw std::runtime_error("Embedded NNUE has an unexpected size");
    }

    s_default_network = reinterpret_cast<const EvalNetwork*>(g_default_networkData);
}

} // illumina
