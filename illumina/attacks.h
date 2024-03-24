#ifndef ILLUMINA_ATTACKS_H
#define ILLUMINA_ATTACKS_H

#include "types.h"

#include <immintrin.h>

namespace illumina {

constexpr size_t N_ATTACK_KEYS = 4096;

template <Color C, bool ASSUME_STARTING_RANK = false>
inline Bitboard pawn_pushes(Square s, Bitboard occ = 0) {
    ILLUMINA_ASSERT_VALID_SQUARE(s);

    constexpr Direction PUSH_DIR = pawn_push_direction(C);

    Bitboard not_occ = ~occ;
    Bitboard pushes  = shift_bb<PUSH_DIR>(BIT(s)) & not_occ;
    if (pushes && (ASSUME_STARTING_RANK || square_rank(s) == pawn_starting_rank(C))) {
        pushes |= shift_bb<PUSH_DIR>(pushes) & not_occ;
    }
    return pushes;
}

template <bool ASSUME_STARTING_RANK = false>
inline Bitboard pawn_pushes(Square s, Color c, Bitboard occ = 0) {
    ILLUMINA_ASSERT_VALID_SQUARE(s);
    ILLUMINA_ASSERT_VALID_COLOR(c);

    return c == CL_WHITE
           ? pawn_pushes<CL_WHITE, ASSUME_STARTING_RANK>(s, occ)
           : pawn_pushes<CL_BLACK, ASSUME_STARTING_RANK>(s, occ);
}

template <Color C>
inline Bitboard pawn_attacks(Square s) {
    ILLUMINA_ASSERT_VALID_SQUARE(s);

    Bitboard s_bb = BIT(s);
    if constexpr (C == CL_WHITE) {
        return shift_bb<DIR_NORTHEAST>(s_bb) | shift_bb<DIR_NORTHWEST>(s_bb);
    }
    else {
        return shift_bb<DIR_SOUTHEAST>(s_bb) | shift_bb<DIR_SOUTHWEST>(s_bb);
    }
}

inline Bitboard pawn_attacks(Square s, Color c) {
    ILLUMINA_ASSERT_VALID_SQUARE(s);
    ILLUMINA_ASSERT_VALID_COLOR(c);

    return c == CL_WHITE
           ? pawn_attacks<CL_WHITE>(s)
           : pawn_attacks<CL_BLACK>(s);
}

inline Bitboard knight_attacks(Square s) {
    ILLUMINA_ASSERT_VALID_SQUARE(s);

    extern const Bitboard g_knight_attacks[];
    return g_knight_attacks[s];
}

inline Bitboard bishop_attacks(Square s, Bitboard occ) {
    ILLUMINA_ASSERT_VALID_SQUARE(s);

    extern Bitboard g_bishop_attacks[SQ_COUNT][N_ATTACK_KEYS];
    extern const Bitboard g_bishop_masks[];
    return g_bishop_attacks[s][_pext_u64(occ, g_bishop_masks[s])];
}

inline Bitboard rook_attacks(Square s, Bitboard occ) {
    ILLUMINA_ASSERT_VALID_SQUARE(s);

    extern Bitboard g_rook_attacks[SQ_COUNT][N_ATTACK_KEYS];
    extern const Bitboard g_rook_masks[];
    return g_rook_attacks[s][_pext_u64(occ, g_rook_masks[s])];
}

inline Bitboard queen_attacks(Square s, Bitboard occ) {
    ILLUMINA_ASSERT_VALID_SQUARE(s);

    return rook_attacks(s, occ) | bishop_attacks(s, occ);
}

inline Bitboard king_attacks(Square s) {
    ILLUMINA_ASSERT_VALID_SQUARE(s);

    extern const Bitboard g_king_attacks[];
    return g_king_attacks[s];
}

inline Bitboard piece_attacks(Piece p, Square s, Bitboard occ) {
    ILLUMINA_ASSERT_VALID_SQUARE(s);

    switch (p.type()) {
        case PT_PAWN:   return pawn_attacks(s, p.color());
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
