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
    constexpr int PIECE_VALUES[] = { 0, 1, 4, 4, 6, 12, 999 };

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
        if (   board.is_pinned(attacker_sq)
            && !bit_is_set(between_bb_inclusive(attacker_sq, board.pinner_square(attacker_sq)), dest)) {
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

static Bitboard get_defenders(const Board& board,
                              Square s,
                              Color c,
                              Bitboard occ) {
    return
          (king_attacks(s)        & board.piece_bb(Piece(c, PT_KING)))
        | (queen_attacks(s, occ)  & board.piece_bb(Piece(c, PT_QUEEN)))
        | (rook_attacks(s, occ)   & board.piece_bb(Piece(c, PT_ROOK)))
        | (bishop_attacks(s, occ) & board.piece_bb(Piece(c, PT_BISHOP)))
        | (knight_attacks(s)      & board.piece_bb(Piece(c, PT_KNIGHT)))
        | (pawn_attacks(s, opposite_color(c)) & board.piece_bb(Piece(c, PT_PAWN)));

}

bool has_good_see_simple(const Board& board,
                         Square source,
                         Square destination) {
    Piece source_piece = board.piece_at(source);

    Bitboard occ = unset_bit(board.occupancy(), source);
    Color us     = source_piece.color();
    Color them   = opposite_color(us);

    // Get all opponent potential attackers.
    Bitboard opponent_attackers = get_defenders(board, destination, them, occ);

    bool defenders_computed = false;
    bool has_defenders; // Will be calculated lazily.

    while (opponent_attackers) {
        Square attacker_sq   = lsb(opponent_attackers);
        Piece attacker_piece = board.piece_at(attacker_sq);

        if (attacker_piece.type() < source_piece.type()) {
            // Attacker is of lesser value. Bad SEE.
            return false;
        }

        // Attacker is of higher value.
        // Compute our defenders.
        if (!defenders_computed) {
            defenders_computed = true;

            Bitboard our_defenders = get_defenders(board, destination, us, occ);

            // Remove moving piece from defenders.
            our_defenders = unset_bit(our_defenders, source);

            has_defenders = our_defenders != 0;
        }

        // If there are no defenders, bad shallow SEE.
        if (!has_defenders) {
            return false;
        }

        opponent_attackers = unset_lsb(opponent_attackers);
    }
    return true;
}

bool attacks_vulnerable_pieces(const Board& board,
                               Square source,
                               Square dest) {
    Piece source_piece = board.piece_at(source);
    Bitboard occ       = unset_bit(board.occupancy(), source);
    Color us     = source_piece.color();
    Color them   = opposite_color(us);
    Bitboard their_pieces = board.color_bb(them);

    Bitboard targets = their_pieces;
    targets &= piece_attacks(source_piece, dest, occ);

    // Ignore kings.
    targets &= ~board.piece_bb(Piece(them, PT_KING));

    // Equal type pieces can attack the piece itself.
    targets &= ~board.piece_bb(Piece(them, source_piece.type()));

    while (targets) {
        Square target = lsb(targets);

        if (  board.piece_at(target).type() > source_piece.type()
            || get_defenders(board, target, them, occ) == 0) {
            return true;
        }

        targets = unset_lsb(targets);
    }
    return false;
}

static Bitboard line_attackers(const Board& board,
                               Square s,
                               Bitboard occ,
                               Color c) {
    Bitboard queens = board.piece_bb(Piece(c, PT_QUEEN));
    Bitboard rooks  = board.piece_bb(Piece(c, PT_ROOK));
    return rook_attacks(s, occ) & (queens | rooks);
}

static Bitboard diagonal_attackers(const Board& board,
                                   Square s,
                                   Bitboard occ,
                                   Color c) {
    Bitboard queens  = board.piece_bb(Piece(c, PT_QUEEN));
    Bitboard bishops = board.piece_bb(Piece(c, PT_BISHOP));
    return bishop_attacks(s, occ) & (queens | bishops);
}

Bitboard discovered_attacks(const Board& board,
                            Square source,
                            Square destination) {
    Color us   = board.color_to_move();
    Color them = opposite_color(us);
    Bitboard their_pieces = board.color_bb(them);
    Bitboard occ_before = board.occupancy();
    Bitboard occ_after  = set_bit(unset_bit(occ_before, source), destination);

    // Check all pieces this piece was blocking.
    Bitboard diagonal_atks = diagonal_attackers(board, source, occ_before, us);
    Bitboard line_atks     = line_attackers(board, source, occ_before, us);

    Bitboard released = 0;

    if (square_file(source) != square_file(destination)) {
        released |= line_atks;
    }
    if (square_rank(source) != square_rank(destination)) {
        released |= diagonal_atks;
    }

    Bitboard revealed = 0;
    while (released != 0) {
        Square s = lsb(released);
        Piece  p = board.piece_at(s);

        revealed |= piece_attacks(p, s, occ_after) & their_pieces;

        released = unset_lsb(released);
    }

    return revealed;
}

Bitboard non_pawn_bb(const Board& board) {
    Bitboard occupancy = board.occupancy();
    Bitboard kings     = board.piece_type_bb(PT_KING);
    Bitboard pawns     = board.piece_type_bb(PT_PAWN);
    return occupancy & (~kings) & (~pawns);
}

} // illumina