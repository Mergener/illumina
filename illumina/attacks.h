#ifndef ILLUMINA_ATTACKS_H
#define ILLUMINA_ATTACKS_H

#include "types.h"

namespace illumina {

inline Bitboard pawn_pushes(Square s, Bitboard occ, Color c); // TODO
inline Bitboard pawn_captures(Square s, Bitboard occ, Color c); // TODO

inline Bitboard pawn_attacks(Square s, Bitboard occ, Color c) {
    return pawn_pushes(s, occ, c) | pawn_captures(s, occ, c);
}

inline Bitboard knight_attacks(Square s) {
    ILLUMINA_ASSERT_VALID_SQUARE(s);
    extern const Bitboard g_knight_attacks[];
    return g_knight_attacks[s];
}

inline Bitboard bishop_attacks(Square s, Bitboard occ); // TODO
inline Bitboard rook_attacks(Square s, Bitboard occ); // TODO

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
