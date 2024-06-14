#ifndef ILLUMINA_NNUE_H
#define ILLUMINA_NNUE_H

#include "types.h"

namespace illumina {

static constexpr size_t NNUE_FEATURES = 768;
static constexpr size_t NNUE_L1_OUT   = 32;
static constexpr size_t NNUE_L2_OUT   = 1;

template <Color PERSPECTIVE>
static constexpr size_t feature_index(Piece piece, Square s) {
    s = PERSPECTIVE == CL_WHITE ? s : mirror_vertical(s);
    return (piece.type() - 1) * 128 + piece.color() * 64 + s;
}


} // illumina

#endif // ILLUMINA_NNUE_H
