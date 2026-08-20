#include "nnue.h"

#define INCBIN_ALIGNMENT_INDEX 6
#include <incbin/incbin.h>

#include <cstddef>
#include <stdexcept>

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
constexpr size_t NETWORK_OBJECT_BYTES = (NETWORK_PAYLOAD_BYTES + 63) & ~size_t(63);
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
    SimdVecI32 sum = SimdVecI32::zero();
    const SimdVecI16 zero = SimdVecI16::zero();
    const SimdVecI16 max  = SimdVecI16::broadcast(Q1);

    const auto& our_accum   = color == CL_WHITE ? m_accum.white : m_accum.black;
    const auto& their_accum = color == CL_WHITE ? m_accum.black : m_accum.white;

    for (size_t i = 0; i < L1_SIZE; i += SimdVecI16::STRIDE) {
        SimdVecI16 activated = SimdVecI16::clamp(SimdVecI16::load_aligned(&our_accum[i]), zero, max);
        SimdVecI16 weighted  = activated * SimdVecI16::load_aligned(&m_net->output_weights[i]);
        sum += SimdVecI16::madd(activated, weighted);

        activated = SimdVecI16::clamp(SimdVecI16::load_aligned(&their_accum[i]), zero, max);
        weighted = activated * SimdVecI16::load_aligned(&m_net->output_weights[L1_SIZE + i]);
        sum += SimdVecI16::madd(activated, weighted);
    }

    int output = sum.hadd();
    output /= Q1;
    output += m_net->output_bias;
    return output * SCALE / (Q1 * Q2);
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
