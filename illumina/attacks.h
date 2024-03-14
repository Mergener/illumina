#ifndef ILLUMINA_ATTACKS_H
#define ILLUMINA_ATTACKS_H

#include "types.h"

#include <immintrin.h>

namespace illumina {

template <Color C>
inline Bitboard pawn_pushes(Square s, Bitboard occ) {
    if constexpr (C == CL_WHITE) {
        extern const Bitboard g_white_pawn_pushes[];
        occ |= shift_bb<DIR_NORTH>(occ);
        return g_white_pawn_pushes[s] & (~occ);
    }
    else {
        extern const Bitboard g_black_pawn_pushes[];
        occ |= shift_bb<DIR_SOUTH>(occ);
        return g_black_pawn_pushes[s] & (~occ);
    }
}

inline Bitboard pawn_pushes(Square s, Color c, Bitboard occ) {
    return c == CL_WHITE
           ? pawn_pushes<CL_WHITE>(s, occ)
           : pawn_pushes<CL_BLACK>(s, occ);
}

template <Color C>
inline Bitboard pawn_captures(Square s, Bitboard occ = 0xffffffffffffffff) {
    if constexpr (C == CL_WHITE) {
        extern const Bitboard g_white_pawn_captures[];
        return g_white_pawn_captures[s] & occ;
    }
    else {
        extern const Bitboard g_black_pawn_captures[];
        return g_black_pawn_captures[s] & occ;
    }
}

inline Bitboard pawn_captures(Square s, Color c, Bitboard occ = 0xffffffffffffffff) {
    return c == CL_WHITE
           ? pawn_captures<CL_WHITE>(s, occ)
           : pawn_captures<CL_BLACK>(s, occ);
}

template <Color C>
inline Bitboard pawn_attacks(Square s, Bitboard occ) {
    return pawn_pushes<C>(s, occ) | pawn_captures<C>(s, occ);
}

inline Bitboard pawn_attacks(Square s, Bitboard occ, Color c) {
    return pawn_pushes(s, occ, c) | pawn_captures(s, occ, c);
}

inline Bitboard knight_attacks(Square s) {
    ILLUMINA_ASSERT_VALID_SQUARE(s);
    extern const Bitboard g_knight_attacks[];
    return g_knight_attacks[s];
}

inline Bitboard bishop_attacks(Square s, Bitboard occ) {
    ILLUMINA_ASSERT_VALID_SQUARE(s);
    extern Bitboard g_bishop_attacks[SQ_COUNT][512];
    extern const Bitboard g_bishop_masks[];
    return g_bishop_attacks[s][_pext_u64(occ, g_bishop_masks[s])];
}

inline Bitboard rook_attacks(Square s, Bitboard occ) {
    ILLUMINA_ASSERT_VALID_SQUARE(s);
    extern Bitboard g_rook_attacks[SQ_COUNT][512];
    extern const Bitboard g_rook_masks[];
    return g_rook_attacks[s][_pext_u64(occ, g_rook_masks[s])];
}

inline Bitboard queen_attacks(Square s, Bitboard occ) {
    return rook_attacks(s, occ) | bishop_attacks(s, occ);
}

inline Bitboard king_attacks(Square s) {
    ILLUMINA_ASSERT_VALID_SQUARE(s);
    extern const Bitboard g_king_attacks[];
    return g_king_attacks[s];
}

inline Bitboard piece_attacks(Piece p, Square s, Bitboard occ) {
    switch (p.type()) {
        case PT_PAWN:   return pawn_attacks(s, occ, p.color());
        case PT_KNIGHT: return knight_attacks(s);
        case PT_BISHOP: return bishop_attacks(s, occ);
        case PT_ROOK:   return rook_attacks(s, occ);
        case PT_QUEEN:  return queen_attacks(s, occ);
        case PT_KING:   return king_attacks(s);
        default: return 0;
    }
}

}

#endif // ILLUMINA_ATTACKS_H
