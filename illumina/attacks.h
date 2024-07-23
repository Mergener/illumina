#ifndef ILLUMINA_ATTACKS_H
#define ILLUMINA_ATTACKS_H

#include "types.h"

#ifdef USE_PEXT
#include <immintrin.h>
#endif

namespace illumina {

constexpr size_t N_ATTACK_KEYS = 4096;

template <Color C>
inline Bitboard pawn_pushes(Square s, Bitboard occ = 0) {
    ILLUMINA_ASSERT_VALID_SQUARE(s);

    constexpr Direction PUSH_DIR = pawn_push_direction(C);

    Bitboard not_occ = ~occ;
    Bitboard pushes  = shift_bb<PUSH_DIR>(BIT(s)) & not_occ;
    if (pushes && (square_rank(s) == pawn_starting_rank(C))) {
        pushes |= shift_bb<PUSH_DIR>(pushes) & not_occ;
    }
    return pushes;
}

inline Bitboard pawn_pushes(Square s, Color c, Bitboard occ = 0) {
    ILLUMINA_ASSERT_VALID_SQUARE(s);
    ILLUMINA_ASSERT_VALID_COLOR(c);

    return c == CL_WHITE
           ? pawn_pushes<CL_WHITE>(s, occ)
           : pawn_pushes<CL_BLACK>(s, occ);
}

template <Color C>
inline Bitboard reverse_pawn_pushes(Square s, Bitboard occ = 0) {
    ILLUMINA_ASSERT_VALID_SQUARE(s);
    constexpr Direction PUSH_DIR = pawn_push_direction(opposite_color(C));

    Bitboard not_occ = ~occ;
    Bitboard pushes  = shift_bb<PUSH_DIR>(BIT(s)) & not_occ;
    if (pushes &&
        ((C == CL_WHITE && square_rank(s) == RNK_4) || (C == CL_BLACK && square_rank(s) == RNK_5))) {
        pushes |= shift_bb<PUSH_DIR>(pushes) & not_occ;
    }
    return pushes;
}

inline Bitboard reverse_pawn_pushes(Square s, Color c, Bitboard occ = 0) {
    ILLUMINA_ASSERT_VALID_SQUARE(s);
    ILLUMINA_ASSERT_VALID_COLOR(c);

    return c == CL_WHITE
           ? reverse_pawn_pushes<CL_WHITE>(s, occ)
           : reverse_pawn_pushes<CL_BLACK>(s, occ);
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

#ifdef USE_PEXT
    extern Bitboard g_bishop_attacks[SQ_COUNT][N_ATTACK_KEYS];
    extern const Bitboard g_bishop_masks[];
    return g_bishop_attacks[s][_pext_u64(occ, g_bishop_masks[s])];
#else
    extern Bitboard g_bishop_attacks[64][512];
    extern const Bitboard g_bishop_masks[64];
    extern const Bitboard g_bishop_magics[64];
    extern const int g_bishop_shifts[64];
    occ &= g_bishop_masks[s];
    ui64 key = (occ * g_bishop_magics[s]) >> (g_bishop_shifts[s]);
    return g_bishop_attacks[s][key];
#endif
}

inline Bitboard rook_attacks(Square s, Bitboard occ) {
    ILLUMINA_ASSERT_VALID_SQUARE(s);

#ifdef USE_PEXT
    extern Bitboard g_rook_attacks[SQ_COUNT][N_ATTACK_KEYS];
    extern const Bitboard g_rook_masks[];
    return g_rook_attacks[s][_pext_u64(occ, g_rook_masks[s])];
#else
    extern Bitboard g_rook_attacks[64][4096];
    extern const Bitboard g_rook_masks[64];
    extern const Bitboard g_rook_magics[64];
    extern const int g_rook_shifts[64];
    occ &= g_rook_masks[s];
    ui64 key = (occ * g_rook_magics[s]) >> (g_rook_shifts[s]);
    return g_rook_attacks[s][key];
#endif
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
