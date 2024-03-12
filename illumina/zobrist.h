#ifndef ILLUMINA_ZOBRIST_H
#define ILLUMINA_ZOBRIST_H

#include "types.h"

namespace illumina {

inline ui64 zob_piece_square_key(Piece piece, Square sqr) {
    extern ui64 g_PieceSquareKeys[PT_COUNT][CL_COUNT][64];
    return g_PieceSquareKeys[piece.type()][piece.color()][sqr];
}

inline ui64 zob_castling_rights_key(CastlingRights castling_rights) {
    extern ui64 g_CastlingRightsKey[16];
    return g_CastlingRightsKey[castling_rights];
}

inline ui64 zob_color_to_move_key(Color c) {
    extern ui64 g_ColorToMoveKey[CL_COUNT];
    return g_ColorToMoveKey[c];
}

inline ui64 zob_en_passant_square_key(Square sqr) {
    extern ui64 g_EnPassantSquareKey[256];
    return g_EnPassantSquareKey[sqr];
}

} // illumina

#endif // ILLUMINA_ZOBRIST_H
