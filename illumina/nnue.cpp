#include "nnue.h"

#define INCBIN_ALIGNMENT_INDEX 6
#include <incbin/incbin.h>

#include <cstddef>
#include <stdexcept>

namespace illumina {

INCBIN(_default_network, NNUE_PATH);
INCBIN(_default_complexity_network, COMPLEXITY_NNUE_PATH);

static const EvalNetwork<EVAL_L1_SIZE, EVAL_BUCKETS>* s_default_network = nullptr;
static const EvalNetwork<COMPLEXITY_L1_SIZE, COMPLEXITY_BUCKETS>* s_default_complexity_network = nullptr;

constexpr int Q1    = 255;
constexpr int Q2    = 64;

template <size_t L1_SIZE, size_t N_BUCKETS>
constexpr size_t network_file_bytes() {
    constexpr size_t payload = (N_INPUTS * L1_SIZE + L1_SIZE
            + N_BUCKETS * 2 * L1_SIZE + N_BUCKETS) * sizeof(i16);
    return (payload + 63) & ~size_t(63);
}

template <size_t L1_SIZE, size_t N_BUCKETS>
constexpr bool valid_network_layout() {
    using Network = EvalNetwork<L1_SIZE, N_BUCKETS>;
    return offsetof(Network, l1_weights) == 0
        && offsetof(Network, l1_biases) == N_INPUTS * L1_SIZE * sizeof(i16)
        && offsetof(Network, output_weights) == (N_INPUTS + 1) * L1_SIZE * sizeof(i16)
        && offsetof(Network, output_biases) == (N_INPUTS + 1 + 2 * N_BUCKETS) * L1_SIZE * sizeof(i16)
        && std::is_standard_layout_v<Network>
        && std::is_trivially_copyable_v<Network>
        && sizeof(Network) == network_file_bytes<L1_SIZE, N_BUCKETS>()
        && alignof(Network) <= INCBIN_ALIGNMENT;
}

static_assert(valid_network_layout<EVAL_L1_SIZE, EVAL_BUCKETS>());
static_assert(valid_network_layout<COMPLEXITY_L1_SIZE, COMPLEXITY_BUCKETS>());

template <size_t N_BUCKETS>
constexpr size_t output_bucket(size_t piece_count) {
    const size_t divisor = (32 + N_BUCKETS - 1) / N_BUCKETS;
    const size_t non_king_pieces = piece_count > 2 ? piece_count - 2 : 0;
    return std::min(non_king_pieces / divisor, N_BUCKETS - 1);
}

template <size_t L1_SIZE, size_t N_BUCKETS>
void NNUE<L1_SIZE, N_BUCKETS>::clear() {
    // Copy all biases.
    std::copy(m_net->l1_biases.begin(), m_net->l1_biases.end(), m_accum.white.begin());
    std::copy(m_net->l1_biases.begin(), m_net->l1_biases.end(), m_accum.black.begin());
}

template <size_t L1_SIZE, size_t N_BUCKETS>
int NNUE<L1_SIZE, N_BUCKETS>::forward(Color color, size_t piece_count) const {
    size_t bucket = output_bucket<N_BUCKETS>(piece_count);
    ILLUMINA_ASSERT(bucket < N_BUCKETS);

    SimdVecI32 sum = SimdVecI32::zero();
    const SimdVecI16 zero = SimdVecI16::zero();
    const SimdVecI16 max  = SimdVecI16::broadcast(Q1);

    const auto& our_accum   = color == CL_WHITE ? m_accum.white : m_accum.black;
    const auto& their_accum = color == CL_WHITE ? m_accum.black : m_accum.white;
    const i16* output_weights = m_net->output_weights.data() + bucket * 2 * L1_SIZE;

    for (size_t i = 0; i < L1_SIZE; i += SimdVecI16::STRIDE) {
        SimdVecI16 activated = SimdVecI16::clamp(SimdVecI16::load_aligned(&our_accum[i]), zero, max);
        SimdVecI16 weighted  = activated * SimdVecI16::load_aligned(&output_weights[i]);
        sum += SimdVecI16::madd(activated, weighted);

        activated = SimdVecI16::clamp(SimdVecI16::load_aligned(&their_accum[i]), zero, max);
        weighted = activated * SimdVecI16::load_aligned(&output_weights[L1_SIZE + i]);
        sum += SimdVecI16::madd(activated, weighted);
    }

    int output = sum.hadd();
    output /= Q1;
    output += m_net->output_biases[bucket];
    return int(i64(output) * m_scale / (Q1 * Q2));
}

template <size_t L1_SIZE, size_t N_BUCKETS>
void NNUE<L1_SIZE, N_BUCKETS>::enable_feature(Square square, Piece piece) {
    update_features<1, 0>({square}, {piece}, {}, {});
}

template <size_t L1_SIZE, size_t N_BUCKETS>
void NNUE<L1_SIZE, N_BUCKETS>::disable_feature(Square square, Piece piece) {
    update_features<0, 1>({}, {}, {square}, {piece});
}

template <size_t L1_SIZE, size_t N_BUCKETS>
void NNUE<L1_SIZE, N_BUCKETS>::push_accumulator() {
    m_accum_stack.push_back(m_accum);
}

template <size_t L1_SIZE, size_t N_BUCKETS>
void NNUE<L1_SIZE, N_BUCKETS>::pop_accumulator() {
    ILLUMINA_ASSERT(!m_accum_stack.empty());

    m_accum = m_accum_stack.back();
    m_accum_stack.pop_back();
}

template <size_t L1_SIZE, size_t N_BUCKETS>
NNUE<L1_SIZE, N_BUCKETS>::NNUE(const EvalNetwork<L1_SIZE, N_BUCKETS>* net, int scale)
    : m_net(net), m_scale(scale) {
    clear();
}

template class NNUE<EVAL_L1_SIZE, EVAL_BUCKETS>;
template class NNUE<COMPLEXITY_L1_SIZE, COMPLEXITY_BUCKETS>;

const EvalNetwork<EVAL_L1_SIZE, EVAL_BUCKETS>* default_eval_network() {
    return s_default_network;
}

const EvalNetwork<COMPLEXITY_L1_SIZE, COMPLEXITY_BUCKETS>* default_complexity_network() {
    return s_default_complexity_network;
}

void init_nnue() {
    if (g_default_networkSize != network_file_bytes<EVAL_L1_SIZE, EVAL_BUCKETS>()) {
        throw std::runtime_error("Embedded NNUE has an unexpected size");
    }
    if (g_default_complexity_networkSize != network_file_bytes<COMPLEXITY_L1_SIZE, COMPLEXITY_BUCKETS>()) {
        throw std::runtime_error("Embedded complexity NNUE has an unexpected size");
    }

    s_default_network = reinterpret_cast<const EvalNetwork<EVAL_L1_SIZE, EVAL_BUCKETS>*>(g_default_networkData);
    s_default_complexity_network = reinterpret_cast<const EvalNetwork<COMPLEXITY_L1_SIZE, COMPLEXITY_BUCKETS>*>(g_default_complexity_networkData);
}

} // illumina
