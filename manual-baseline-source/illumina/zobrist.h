#ifndef ILLUMINA_ZOBRIST_H
#define ILLUMINA_ZOBRIST_H

#include "types.h"

namespace illumina {

inline ui64 zob_piece_square_key(Piece piece, Square sqr) {
    extern ui64 g_piece_square_keys[PT_COUNT][CL_COUNT][SQ_COUNT];
    return g_piece_square_keys[piece.type()][piece.color()][sqr];
}

inline ui64 zob_castling_rights_key(CastlingRights castling_rights) {
    extern ui64 g_castling_rights_keys[16];
    return g_castling_rights_keys[castling_rights];
}

inline ui64 zob_color_to_move_key(Color c) {
    extern ui64 g_color_to_move_keys[CL_COUNT];
    return g_color_to_move_keys[c];
}

inline ui64 zob_en_passant_square_key(Square sqr) {
    extern ui64 g_en_passant_square_keys[256];
    return g_en_passant_square_keys[sqr];
}

} // illumina

#endif // ILLUMINA_ZOBRIST_H
