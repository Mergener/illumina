#include "boardutils.h"

namespace illumina {

static Square least_valuable_attacker_of(const Board& board,
                                         Color c,
                                         Square s,
                                         Bitboard occ) {
    Bitboard their_pawns = board.piece_bb(Piece(c, PT_PAWN)) & occ;
    Bitboard pawn_targets;
    pawn_targets = pawn_attacks(s, opposite_color(c));
    Bitboard pawn_attacker = pawn_targets & their_pawns;
    if (pawn_attacker) {
        return lsb(pawn_attacker);
    }

    Bitboard their_knights   = board.piece_bb(Piece(c, PT_KNIGHT)) & occ;
    Bitboard knight_atks     = knight_attacks(s);
    Bitboard knight_attacker = knight_atks & their_knights;
    if (knight_attacker) {
        return lsb(knight_attacker);
    }

    Bitboard their_bishops   = board.piece_bb(Piece(c, PT_BISHOP)) & occ;
    Bitboard bishop_atks     = bishop_attacks(s, occ);
    Bitboard bishop_attacker = bishop_atks & their_bishops;
    if (bishop_attacker) {
        return lsb(bishop_attacker);
    }

    Bitboard their_rooks   = board.piece_bb(Piece(c, PT_ROOK)) & occ;
    Bitboard rook_atks     = rook_attacks(s, occ);
    Bitboard rook_attacker = rook_atks & their_rooks;
    if (rook_attacker) {
        return lsb(rook_attacker);
    }

    Bitboard their_queens   = board.piece_bb(Piece(c, PT_QUEEN)) & occ;
    Bitboard queen_atks     = rook_atks | bishop_atks;
    Bitboard queen_attacker = queen_atks & their_queens;
    if (queen_attacker) {
        return lsb(queen_attacker);
    }

    Bitboard their_king_bb  = board.piece_bb(Piece(c, PT_KING)) & occ;
    Bitboard king_atks      = king_attacks(s);
    Bitboard king_attacker  = king_atks & their_king_bb;
    if (king_attacker) {
        return lsb(king_attacker);
    }

    return SQ_NULL;
}

bool has_good_see(const Board& board,
                  Square source,
                  Square dest,
                  int threshold) {
    int gain = 0; // End gained value, at the first source piece color perspective.
    int sign = 1;

    // Setup variables.
    Piece src_piece = board.piece_at(source);
    Piece dst_piece = board.piece_at(dest);
    Color color  = src_piece.color();
    Bitboard occ = board.occupancy();

    // Simulate our first capture.
    gain += PIECE_VALUES[dst_piece.type()];
    dst_piece = src_piece;
    occ   = unset_bit(occ, source);
    color = opposite_color(color);
    sign  = -1;

    while (true) {
        Square attacker_sq = least_valuable_attacker_of(board, color, dest, occ);
        if (attacker_sq == SQ_NULL) {
            break;
        }

        // Prevent pinned attackers from trying to capture, unless 'dest' is between
        // them and their pinner.
        if (board.is_pinned(attacker_sq) &&
            !bit_is_set(between_bb_inclusive(attacker_sq, board.pinner_square(attacker_sq)), dest)) {
            occ = unset_bit(occ, attacker_sq);
            continue;
        }

        gain += sign * PIECE_VALUES[dst_piece.type()];
        dst_piece = board.piece_at(attacker_sq);
        occ   = unset_bit(occ, attacker_sq);
        color = opposite_color(color);
        sign  = -sign;

        if (sign == 1 && gain >= threshold) {
            break;
        }
    }

    return gain >= threshold;
}

} // illumina