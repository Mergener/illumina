#ifndef ILLUMINA_NNUE_H
#define ILLUMINA_NNUE_H

#include <array>

#include "types.h"

namespace illumina {

static constexpr size_t N_INPUTS = 768;
static constexpr size_t L1_SIZE  = 96;

struct EvalNetwork {
    alignas(32) std::array<i16, N_INPUTS * L1_SIZE> l1_weights;
    alignas(32) std::array<i16, L1_SIZE> l1_biases;
    alignas(32) std::array<i16, L1_SIZE * 2> output_weights;
    i16 output_bias;

    EvalNetwork() = default;
    EvalNetwork(std::istream& stream);
};

struct Accumulator {
    alignas(32) std::array<i16, L1_SIZE> white {};
    alignas(32) std::array<i16, L1_SIZE> black {};
};

class NNUE {
public:
    void clear();

    void enable_feature(Square square, Piece piece);
    void disable_feature(Square square, Piece piece);

    int forward(Color color) const;

    NNUE();

private:
    const EvalNetwork* m_net;
    Accumulator m_accum {};
};

} // illumina

#endif // ILLUMINA_NNUE_H
