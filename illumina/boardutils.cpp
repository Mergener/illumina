#include "boardutils.h"

namespace illumina {

static Square least_valuable_attacker_of(const Board& board,
                                         Color c,
                                         Square s,
                                         Bitboard occ) {
    auto their_pawns = board.piece_bb(Piece(c, PT_PAWN)) & occ;
    Bitboard pawn_targets;
    pawn_targets = pawn_attacks(s, opposite_color(c));
    auto pawn_attacker = pawn_targets & their_pawns;
    if (pawn_attacker) {
        return lsb(pawn_attacker);
    }

    auto their_knights   = board.piece_bb(Piece(c, PT_KNIGHT)) & occ;
    auto knight_atks     = knight_attacks(s);
    auto knight_attacker = knight_atks & their_knights;
    if (knight_attacker) {
        return lsb(knight_attacker);
    }

    auto their_bishops   = board.piece_bb(Piece(c, PT_BISHOP)) & occ;
    auto bishop_atks     = bishop_attacks(s, occ);
    auto bishop_attacker = bishop_atks & their_bishops;
    if (bishop_attacker) {
        return lsb(bishop_attacker);
    }

    auto their_rooks   = board.piece_bb(Piece(c, PT_ROOK)) & occ;
    auto rook_atks     = rook_attacks(s, occ);
    auto rook_attacker = rook_atks & their_rooks;
    if (rook_attacker) {
        return lsb(rook_attacker);
    }

    auto their_queens   = board.piece_bb(Piece(c, PT_QUEEN)) & occ;
    auto queen_atks     = rook_atks | bishop_atks;
    auto queen_attacker = queen_atks & their_queens;
    if (queen_attacker) {
        return lsb(queen_attacker);
    }

    auto their_king_bb  = board.piece_bb(Piece(c, PT_KING)) & occ;
    auto king_atks      = king_attacks(s);
    auto king_attacker  = king_atks & their_king_bb;
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

    auto gain = 0; // End gained value, at the first source piece color perspective.
    auto sign = 1;

    // Setup variables.
    auto src_piece = board.piece_at(source);
    auto dst_piece = board.piece_at(dest);
    auto color  = src_piece.color();
    auto occ = board.occupancy();

    // Simulate our first capture.
    gain += PIECE_VALUES[dst_piece.type()];
    dst_piece = src_piece;
    occ   = unset_bit(occ, source);
    color = opposite_color(color);
    sign  = -1;

    while (true) {
        auto attacker_sq = least_valuable_attacker_of(board, color, dest, occ);
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
    auto source_piece = board.piece_at(source);

    auto occ = unset_bit(board.occupancy(), source);
    auto us     = source_piece.color();
    auto them   = opposite_color(us);

    // Get all opponent potential attackers.
    auto opponent_attackers = get_defenders(board, destination, them, occ);

    auto defenders_computed = false;
    bool has_defenders; // Will be calculated lazily.

    while (opponent_attackers) {
        auto attacker_sq   = lsb(opponent_attackers);
        auto attacker_piece = board.piece_at(attacker_sq);

        if (attacker_piece.type() < source_piece.type()) {
            // Attacker is of lesser value. Bad SEE.
            return false;
        }

        // Attacker is of higher value.
        // Compute our defenders.
        if (!defenders_computed) {
            defenders_computed = true;

            auto our_defenders = get_defenders(board, destination, us, occ);

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
    auto source_piece = board.piece_at(source);
    auto occ       = unset_bit(board.occupancy(), source);
    auto us     = source_piece.color();
    auto them   = opposite_color(us);
    auto their_pieces = board.color_bb(them);

    auto targets = their_pieces;
    targets &= piece_attacks(source_piece, dest, occ);

    // Ignore kings.
    targets &= ~board.piece_bb(Piece(them, PT_KING));

    // Equal type pieces can attack the piece itself.
    targets &= ~board.piece_bb(Piece(them, source_piece.type()));

    while (targets) {
        auto target = lsb(targets);

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
    auto queens = board.piece_bb(Piece(c, PT_QUEEN));
    auto rooks  = board.piece_bb(Piece(c, PT_ROOK));
    return rook_attacks(s, occ) & (queens | rooks);
}

static Bitboard diagonal_attackers(const Board& board,
                                   Square s,
                                   Bitboard occ,
                                   Color c) {
    auto queens  = board.piece_bb(Piece(c, PT_QUEEN));
    auto bishops = board.piece_bb(Piece(c, PT_BISHOP));
    return bishop_attacks(s, occ) & (queens | bishops);
}

Bitboard discovered_attacks(const Board& board,
                            Square source,
                            Square destination) {
    auto us   = board.color_to_move();
    auto them = opposite_color(us);
    auto their_pieces = board.color_bb(them);
    auto occ_before = board.occupancy();
    auto occ_after  = set_bit(unset_bit(occ_before, source), destination);

    // Check all pieces this piece was blocking.
    auto diagonal_atks = diagonal_attackers(board, source, occ_before, us);
    auto line_atks     = line_attackers(board, source, occ_before, us);

    auto released = Bitboard(0);

    if (square_file(source) != square_file(destination)) {
        released |= line_atks;
    }
    if (square_rank(source) != square_rank(destination)) {
        released |= diagonal_atks;
    }

    auto revealed = Bitboard(0);
    while (released != 0) {
        auto s = lsb(released);
        auto p = board.piece_at(s);

        revealed |= piece_attacks(p, s, occ_after) & their_pieces;

        released = unset_lsb(released);
    }

    return revealed;
}

Bitboard non_pawn_bb(const Board& board) {
    auto occupancy = board.occupancy();
    auto kings     = board.piece_type_bb(PT_KING);
    auto pawns     = board.piece_type_bb(PT_PAWN);
    return occupancy & (~kings) & (~pawns);
}

} // illumina
