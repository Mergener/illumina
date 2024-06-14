#include "accumulator.h"

namespace illumina {

template <Color C>
static void refresh_accum_perspective(NNUEAccumulator& accumulator,
                                      const Board& board,
                                      const i16* l1_weights,
                                      const i16* l1_biases) {
    for (size_t i = 0; i < NNUE_L1_OUT; ++i) {
        
    }
}

void NNUEAccumulator::refresh(const Board& board,
                              const i16* l1_weights,
                              const i16* l1_biases) {
    refresh_accum_perspective<CL_WHITE>(*this, board, l1_weights, l1_biases);
    refresh_accum_perspective<CL_BLACK>(*this, board, l1_weights, l1_biases);
}

void NNUEAccumulator::update(const Board& board,
                             Move move,
                             const i16* l1_weights) {

}

} // illumina