#ifndef ILLUMINA_ACCUMULATOR_H
#define ILLUMINA_ACCUMULATOR_H

#include <array>

#include "../nnue.h"
#include "../board.h"

namespace illumina {

struct NNUEAccumulator {
    void refresh(const Board& board,
                 const i16* l1_weights,
                 const i16* l1_biases);

    void update(const Board& board,
                Move move,
                const i16* l1_weights);

    std::array<i16, NNUE_L1_OUT> white_accum;
    std::array<i16, NNUE_L1_OUT> black_accum;
};

} // illumina

#endif // ILLUMINA_ACCUMULATOR_H
