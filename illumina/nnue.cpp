#include "nnue.h"

#define INCBIN_ALIGNMENT_INDEX 6
#include <incbin/incbin.h>

#include <cstddef>
#include <stdexcept>

namespace illumina {

INCBIN(_default_network, NNUE_PATH);
INCBIN(_default_complexity_network, COMPLEXITY_NNUE_PATH);

static const i16* s_default_network = nullptr;
static const i16* s_default_complexity_network = nullptr;

constexpr int SCALE = 400;
constexpr int COMPLEXITY_SCALE = 16384;
constexpr size_t COMPLEXITY_L1_SIZE = 512;
constexpr size_t COMPLEXITY_OUTPUT_BUCKETS = 1;
constexpr int Q1    = 255;
constexpr int Q2    = 64;

constexpr size_t L1_WEIGHTS_BYTES = N_INPUTS * L1_SIZE * sizeof(i16);
constexpr size_t L1_BIASES_BYTES = L1_SIZE * sizeof(i16);
constexpr size_t OUTPUT_WEIGHTS_BYTES = OUTPUT_BUCKETS * 2 * L1_SIZE * sizeof(i16);
constexpr size_t OUTPUT_BIASES_BYTES = OUTPUT_BUCKETS * sizeof(i16);
constexpr size_t NETWORK_PAYLOAD_BYTES = L1_WEIGHTS_BYTES
                                       + L1_BIASES_BYTES
                                       + OUTPUT_WEIGHTS_BYTES
                                       + OUTPUT_BIASES_BYTES;
constexpr size_t NETWORK_OBJECT_BYTES = (NETWORK_PAYLOAD_BYTES + 63) & ~size_t(63);
constexpr size_t NETWORK_FILE_BYTES = (NETWORK_PAYLOAD_BYTES + 63) & ~size_t(63);
constexpr size_t COMPLEXITY_PAYLOAD_BYTES = (N_INPUTS * COMPLEXITY_L1_SIZE
        + COMPLEXITY_L1_SIZE
        + COMPLEXITY_OUTPUT_BUCKETS * 2 * COMPLEXITY_L1_SIZE
        + COMPLEXITY_OUTPUT_BUCKETS) * sizeof(i16);
constexpr size_t COMPLEXITY_FILE_BYTES = (COMPLEXITY_PAYLOAD_BYTES + 63) & ~size_t(63);

static_assert(offsetof(EvalNetwork, l1_weights) == 0);
static_assert(offsetof(EvalNetwork, l1_biases) == L1_WEIGHTS_BYTES);
static_assert(offsetof(EvalNetwork, output_weights) == L1_WEIGHTS_BYTES + L1_BIASES_BYTES);
static_assert(offsetof(EvalNetwork, output_biases) == L1_WEIGHTS_BYTES + L1_BIASES_BYTES + OUTPUT_WEIGHTS_BYTES);
static_assert(std::is_standard_layout_v<EvalNetwork>);
static_assert(std::is_trivially_copyable_v<EvalNetwork>);
static_assert(sizeof(EvalNetwork) == NETWORK_OBJECT_BYTES);
static_assert(sizeof(EvalNetwork) <= NETWORK_FILE_BYTES);
static_assert(alignof(EvalNetwork) <= INCBIN_ALIGNMENT);

constexpr size_t output_bucket(size_t piece_count, size_t buckets) {
    const size_t divisor = (32 + buckets - 1) / buckets;
    const size_t non_king_pieces = piece_count > 2 ? piece_count - 2 : 0;
    return std::min(non_king_pieces / divisor, buckets - 1);
}

void NNUE::clear() {
    // Copy all biases.
    std::copy_n(m_biases, m_l1_size, m_accum.white.begin());
    std::copy_n(m_biases, m_l1_size, m_accum.black.begin());
}

int NNUE::forward(Color color, size_t piece_count) const {
    size_t bucket = output_bucket(piece_count, m_output_buckets);
    ILLUMINA_ASSERT(bucket < m_output_buckets);

    SimdVecI32 sum = SimdVecI32::zero();
    const SimdVecI16 zero = SimdVecI16::zero();
    const SimdVecI16 max  = SimdVecI16::broadcast(Q1);

    const auto& our_accum   = color == CL_WHITE ? m_accum.white : m_accum.black;
    const auto& their_accum = color == CL_WHITE ? m_accum.black : m_accum.white;
    const i16* output_weights = m_output_weights + bucket * 2 * m_l1_size;

    for (size_t i = 0; i < m_l1_size; i += SimdVecI16::STRIDE) {
        SimdVecI16 activated = SimdVecI16::clamp(SimdVecI16::load_aligned(&our_accum[i]), zero, max);
        SimdVecI16 weighted  = activated * SimdVecI16::load_aligned(&output_weights[i]);
        sum += SimdVecI16::madd(activated, weighted);

        activated = SimdVecI16::clamp(SimdVecI16::load_aligned(&their_accum[i]), zero, max);
        weighted = activated * SimdVecI16::load_aligned(&output_weights[m_l1_size + i]);
        sum += SimdVecI16::madd(activated, weighted);
    }

    int output = sum.hadd();
    output /= Q1;
    output += m_output_biases[bucket];
    return int(i64(output) * m_scale / (Q1 * Q2));
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

NNUE::NNUE(bool complexity)
    : m_weights(complexity ? s_default_complexity_network : s_default_network),
      m_biases(m_weights + N_INPUTS * (complexity ? COMPLEXITY_L1_SIZE : L1_SIZE)),
      m_output_weights(m_biases + (complexity ? COMPLEXITY_L1_SIZE : L1_SIZE)),
      m_output_biases(m_output_weights + (complexity ? COMPLEXITY_OUTPUT_BUCKETS * 2 * COMPLEXITY_L1_SIZE : OUTPUT_BUCKETS * 2 * L1_SIZE)),
      m_l1_size(complexity ? COMPLEXITY_L1_SIZE : L1_SIZE),
      m_output_buckets(complexity ? COMPLEXITY_OUTPUT_BUCKETS : OUTPUT_BUCKETS),
      m_scale(complexity ? COMPLEXITY_SCALE : SCALE) {
    clear();
}

void init_nnue() {
    if (g_default_networkSize != NETWORK_FILE_BYTES) {
        throw std::runtime_error("Embedded NNUE has an unexpected size");
    }
    if (g_default_complexity_networkSize != COMPLEXITY_FILE_BYTES) {
        throw std::runtime_error("Embedded complexity NNUE has an unexpected size");
    }

    s_default_network = reinterpret_cast<const i16*>(g_default_networkData);
    s_default_complexity_network = reinterpret_cast<const i16*>(g_default_complexity_networkData);
}

} // illumina
