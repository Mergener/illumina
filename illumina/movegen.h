#ifndef ILLUMINA_MOVEGEN_H
#define ILLUMINA_MOVEGEN_H

#include <type_traits>

#include "debug.h"
#include "types.h"
#include "board.h"
#include "attacks.h"

namespace illumina {

/**
 * Defines what is the nature of the moves being generated.
 */
enum class MoveGenerationType {
    /** Generate all moves in the position. */
    ALL,

    /** Generate captures only. Includes en-passant and promotion-captures. */
    CAPTURES,

    /** Same as MoveGenerationType::CAPTURES but also includes simple promotions. */
    NOISY,

    /** Generate all non-noisy moves (non-capture and non-promotion moves)  */
    QUIET,

    /** Generate all moves that get out of check. */
    ALL_EVASIONS,

    /** Generate all noisy moves that get out of check. */
    NOISY_EVASIONS,

    /** Generate all noisy moves that get out of check. */
    QUIET_EVASIONS,
};

template <MoveGenerationType TYPE = MoveGenerationType::ALL,
          bool LEGAL = true,
          ui64 PIECE_TYPE_MASK = BITMASK(PT_COUNT),
          typename TMOVE = Move>
size_t generate_moves(const Board& board, TMOVE* moves);

template <Color C,
          MoveGenerationType TYPE = MoveGenerationType::ALL,
          bool LEGAL = true,
          ui64 PIECE_TYPE_MASK = BITMASK(PT_COUNT),
          typename TMOVE = Move>
size_t generate_moves_by_color(const Board& board, TMOVE* moves);

template <Color C,
          MoveGenerationType TYPE = MoveGenerationType::ALL,
          typename TMOVE = Move>
size_t generate_pawn_moves_by_color(const Board& board, TMOVE* moves);

template <Color C,
          MoveGenerationType TYPE = MoveGenerationType::ALL,
          typename TMOVE = Move>
size_t generate_knight_moves_by_color(const Board& board, TMOVE* moves);

template <Color C,
          MoveGenerationType TYPE = MoveGenerationType::ALL,
          typename TMOVE = Move>
size_t generate_bishop_moves_by_color(const Board& board, TMOVE* moves);

template <Color C,
          MoveGenerationType TYPE = MoveGenerationType::ALL,
          typename TMOVE = Move>
size_t generate_rook_moves_by_color(const Board& board, TMOVE* moves);

template <Color C,
          MoveGenerationType TYPE = MoveGenerationType::ALL,
          typename TMOVE = Move>
size_t generate_queen_moves_by_color(const Board& board, TMOVE* moves);

template <Color C,
          MoveGenerationType TYPE = MoveGenerationType::ALL,
          typename TMOVE = Move>
size_t generate_king_moves_by_color(const Board& board, TMOVE* moves);

template <Color C,
          MoveGenerationType TYPE = MoveGenerationType::ALL,
          typename TMOVE = Move>
size_t generate_evasions_by_color(const Board& board, TMOVE* moves);

template <Color C,
    MoveGenerationType TYPE = MoveGenerationType::ALL,
    typename TMOVE = Move>
size_t generate_pawn_evasions_by_color(const Board& board, TMOVE* moves);

template <Color C,
    MoveGenerationType TYPE = MoveGenerationType::ALL,
    typename TMOVE = Move>
size_t generate_knight_evasions_by_color(const Board& board, TMOVE* moves);

template <Color C,
    MoveGenerationType TYPE = MoveGenerationType::ALL,
    typename TMOVE = Move>
size_t generate_bishop_evasions_by_color(const Board& board, TMOVE* moves);

template <Color C,
    MoveGenerationType TYPE = MoveGenerationType::ALL,
    typename TMOVE = Move>
size_t generate_rook_evasions_by_color(const Board& board, TMOVE* moves);

template <Color C,
    MoveGenerationType TYPE = MoveGenerationType::ALL,
    typename TMOVE = Move>
size_t generate_queen_evasions_by_color(const Board& board, TMOVE* moves);

template <Color C,
    MoveGenerationType TYPE = MoveGenerationType::ALL,
    typename TMOVE = Move>
size_t generate_king_evasions_by_color(const Board& board, TMOVE* moves);

#define MOVEGEN_ASSERTIONS() \
    static_assert(std::is_assignable_v<TMOVE, Move>); \
    ILLUMINA_ASSERT(moves != nullptr)

template <MoveGenerationType TYPE,
          bool LEGAL,
          ui64 PIECE_TYPE_MASK,
          typename TMOVE>
size_t generate_moves(const Board& board, TMOVE* moves) {
    MOVEGEN_ASSERTIONS();

    size_t total;

    if (board.color_to_move() == CL_WHITE) {
        total = generate_moves_by_color<CL_WHITE, TYPE, LEGAL, PIECE_TYPE_MASK, TMOVE>(board, moves);
    }
    else {
        total = generate_moves_by_color<CL_WHITE, TYPE, LEGAL, PIECE_TYPE_MASK, TMOVE>(board, moves);
    }

    return total;
}

template <Color C,
          MoveGenerationType TYPE,
          bool LEGAL,
          ui64 PIECE_TYPE_MASK,
          typename TMOVE>
size_t generate_moves_by_color(const Board& board, TMOVE* moves) {
    MOVEGEN_ASSERTIONS();

    size_t total = 0;

    constexpr bool GEN_PAWN   = (PIECE_TYPE_MASK & BIT(PT_PAWN)) != 0;
    constexpr bool GEN_KNIGHT = (PIECE_TYPE_MASK & BIT(PT_KNIGHT)) != 0;
    constexpr bool GEN_BISHOP = (PIECE_TYPE_MASK & BIT(PT_BISHOP)) != 0;
    constexpr bool GEN_ROOK   = (PIECE_TYPE_MASK & BIT(PT_ROOK)) != 0;
    constexpr bool GEN_QUEEN  = (PIECE_TYPE_MASK & BIT(PT_QUEEN)) != 0;
    constexpr bool GEN_KING   = (PIECE_TYPE_MASK & BIT(PT_KING)) != 0;

    constexpr bool GEN_EVASIONS = TYPE == MoveGenerationType::ALL_EVASIONS
                               || TYPE == MoveGenerationType::NOISY_EVASIONS
                               || TYPE == MoveGenerationType::QUIET_EVASIONS;

    if constexpr (!GEN_EVASIONS) {
        if constexpr (GEN_PAWN) {
            total += generate_pawn_moves_by_color<C, TYPE, LEGAL, TMOVE>(board, moves);
        }
        if constexpr (GEN_KNIGHT) {
            total += generate_knight_moves_by_color<C, TYPE, LEGAL, TMOVE>(board, moves);
        }
        if constexpr (GEN_BISHOP) {
            total += generate_bishop_moves_by_color<C, TYPE, LEGAL, TMOVE>(board, moves);
        }
        if constexpr (GEN_ROOK) {
            total += generate_rook_moves_by_color<C, TYPE, LEGAL, TMOVE>(board, moves);
        }
        if constexpr (GEN_QUEEN) {
            total += generate_queen_moves_by_color<C, TYPE, LEGAL, TMOVE>(board, moves);
        }
        if constexpr (GEN_KING) {
            total += generate_king_moves_by_color<C, TYPE, LEGAL, TMOVE>(board, moves);
        }
    }
    else {
        total = generate_evasions_by_color<C, TYPE, TMOVE>(board, moves);
    }

    return total;
}

template <Color C,
          MoveGenerationType TYPE,
          typename TMOVE>
size_t generate_pawn_moves_by_color(const Board& board, TMOVE* moves) {
    MOVEGEN_ASSERTIONS();

    TMOVE* begin = moves;

    constexpr Piece PAWN = Piece(C, PT_PAWN);

    // Get movement constants
    constexpr Direction PUSH_DIR           = pawn_push_direction(C);
    constexpr Direction LEFT_CAPT_DIR      = pawn_left_capture_direction(C);
    constexpr Direction RIGHT_CAPT_DIR     = pawn_right_capture_direction(C);
    constexpr BoardRank DOUBLE_PUSH_RANK   = double_push_dest_rank(C);
    constexpr Bitboard DOUBLE_PUSH_RANK_BB = rank_bb(DOUBLE_PUSH_RANK);
    constexpr Bitboard PROM_RANK_BB        = rank_bb(promotion_rank(C));
    constexpr Bitboard BEHIND_PROM_RANK_BB = shift_bb<-PUSH_DIR>(PROM_RANK_BB);

    Bitboard occ       = board.occupancy();
    Bitboard their_bb  = board.color_bb(opposite_color(C));
    Bitboard our_pawns = board.piece_bb(PAWN);

    // Generate promotion captures
    constexpr bool GEN_PROM_CAPTURES =
        TYPE == MoveGenerationType::CAPTURES ||
        TYPE == MoveGenerationType::NOISY    ||
        TYPE == MoveGenerationType::ALL;
    if constexpr (GEN_PROM_CAPTURES) {
        // Left captures
        Bitboard left_attacks = shift_bb<LEFT_CAPT_DIR>(our_pawns) & their_bb;

        // Include only promotion rank
        left_attacks &= PROM_RANK_BB;

        while (left_attacks) {
            Square dst = lsb(left_attacks);
            Square src = dst - LEFT_CAPT_DIR;

            for (PieceType pt: PROMOTION_PIECE_TYPES) {
                *moves++ = Move::new_promotion_capture(src, dst, C, board.piece_at(dst), pt);
            }

            left_attacks = unset_lsb(left_attacks);
        }

        // Right captures
        Bitboard right_attacks = shift_bb<RIGHT_CAPT_DIR>(our_pawns) & their_bb;

        // Include only promotion rank
        right_attacks &= PROM_RANK_BB;

        while (right_attacks) {
            Square dst = lsb(right_attacks);
            Square src = dst - RIGHT_CAPT_DIR;

            for (PieceType pt: PROMOTION_PIECE_TYPES) {
                *moves++ = Move::new_promotion_capture(src, dst, C, board.piece_at(dst), pt);
            }

            right_attacks = unset_lsb(right_attacks);
        }
    }

    // Generate push-promotions
    constexpr bool GEN_SIMPLE_PROMOTIONS =
        TYPE == MoveGenerationType::NOISY    ||
        TYPE == MoveGenerationType::ALL;
    if constexpr (GEN_SIMPLE_PROMOTIONS) {
        // Promoting pawns are one step behind the promotion rank
        Bitboard promoting_pawns = BEHIND_PROM_RANK_BB & our_pawns;

        while (promoting_pawns) {
            Square src = lsb(promoting_pawns);

            for (PieceType pt: PROMOTION_PIECE_TYPES) {
                Square dst = src + PUSH_DIR;
                *moves++   = Move::new_simple_promotion(src, dst, C, pt);
            }

            promoting_pawns = unset_lsb(promoting_pawns);
        }
    }

    // Generate non-promotion and non-en-passant captures
    constexpr bool GEN_SIMPLE_CAPTURES =
        TYPE == MoveGenerationType::CAPTURES ||
        TYPE == MoveGenerationType::NOISY    ||
        TYPE == MoveGenerationType::ALL;
    if constexpr (GEN_SIMPLE_CAPTURES) {
        // Left captures
        Bitboard left_attacks = shift_bb<LEFT_CAPT_DIR>(our_pawns) & their_bb;

        // Exclude promotion rank
        left_attacks &= ~PROM_RANK_BB;

        while (left_attacks) {
            Square dst = lsb(left_attacks);
            Square src = dst - LEFT_CAPT_DIR;

            *moves++ = Move::new_simple_capture(src, dst, PAWN, board.piece_at(dst));

            left_attacks = unset_lsb(left_attacks);
        }

        // Right captures
        Bitboard right_attacks = shift_bb<RIGHT_CAPT_DIR>(our_pawns) & their_bb;

        // Exclude promotion rank
        right_attacks &= ~PROM_RANK_BB;

        while (right_attacks) {
            Square dst = lsb(right_attacks);
            Square src = dst - RIGHT_CAPT_DIR;

            *moves++ = Move::new_simple_capture(src, dst, PAWN, board.piece_at(dst));

            right_attacks = unset_lsb(right_attacks);
        }
    }

    // Generate en-passant captures
    constexpr bool GEN_EP =
        TYPE == MoveGenerationType::CAPTURES ||
        TYPE == MoveGenerationType::NOISY    ||
        TYPE == MoveGenerationType::ALL;
    if constexpr (GEN_EP) {
        Square ep_square = board.ep_square();
        if (ep_square != SQ_NULL) {
            Bitboard ep_bb = BIT(ep_square);

            Bitboard ep_pawns = (shift_bb<-LEFT_CAPT_DIR>(ep_bb) | shift_bb<-RIGHT_CAPT_DIR>(ep_bb)) & our_pawns;

            while (ep_pawns) {
                Square src = lsb(ep_square);
                *moves++ = Move::new_en_passant_capture(src, ep_square, C);
                ep_pawns = unset_lsb(ep_pawns);
            }
        }
    }

    // Generate pawn pushes
    constexpr bool GEN_PUSHES =
        TYPE == MoveGenerationType::QUIET ||
        TYPE == MoveGenerationType::ALL;
    if constexpr (GEN_PUSHES) {
        // Pretend all squares in the promotion rank are occupied so that
        // we prevent "quiet" pushes to the promotion rank.
        Bitboard push_occ    = occ | PROM_RANK_BB;

        // Single pushes (prevent pushing to occupied squares)
        Bitboard push_bb = shift_bb<PUSH_DIR>(our_pawns) & ~push_occ;

        for (Bitboard bb = push_bb; bb; bb = unset_lsb(bb)) {
            Square dst = lsb(bb);
            Square src = dst - PUSH_DIR;
            *moves++   = Move::new_normal(src, dst, PAWN);
        }

        // Double pushes
        push_bb = shift_bb<PUSH_DIR>(push_bb) & ~push_occ;

        // Filter out pawns that can't perform double pushes
        push_bb &= DOUBLE_PUSH_RANK_BB;

        while (push_bb) {
            Square dst = lsb(push_bb);
            *moves++   = Move::new_double_push_from_dest(dst, C);
            push_bb    = unset_lsb(push_bb);
        }

    }

    return moves - begin;
}

template <Color C,
          MoveGenerationType TYPE,
          typename TMOVE>
size_t generate_knight_moves_by_color(const Board& board, TMOVE* moves) {
    MOVEGEN_ASSERTIONS();

    TMOVE* begin = moves;

    constexpr Piece KNIGHT = Piece(C, PT_KNIGHT);

    Bitboard our_knights = board.piece_bb(KNIGHT);
    Bitboard their_bb    = board.color_bb(opposite_color(C));
    Bitboard occ         = board.occupancy();

    constexpr bool GEN_SIMPLE_CAPTURES =
        TYPE == MoveGenerationType::CAPTURES ||
        TYPE == MoveGenerationType::NOISY    ||
        TYPE == MoveGenerationType::ALL;

    constexpr bool GEN_QUIET =
        TYPE == MoveGenerationType::QUIET    ||
        TYPE == MoveGenerationType::ALL;

    Bitboard src_squares = our_knights;
    while (src_squares) {
        Square src       = lsb(src_squares);
        Bitboard attacks = knight_attacks(src);

        if constexpr (GEN_SIMPLE_CAPTURES) {
            Bitboard capture_dsts = attacks & their_bb;
            while (capture_dsts) {
                Square dst   = lsb(capture_dsts);
                *moves++     = Move::new_simple_capture(src, dst, KNIGHT, board.piece_at(dst));
                capture_dsts = unset_lsb(capture_dsts);
            }
        }

        if constexpr (GEN_QUIET) {
            Bitboard normal_dsts = attacks & (~occ);
            while (normal_dsts) {
                Square dst  = lsb(normal_dsts);
                *moves++    = Move::new_normal(src, dst, KNIGHT);
                normal_dsts = unset_lsb(normal_dsts);
            }
        }

        src_squares = unset_lsb(src_squares);
    }

    return moves - begin;
}

template <Color C,
          MoveGenerationType TYPE,
          typename TMOVE>
size_t generate_bishop_moves_by_color(const Board& board, TMOVE* moves) {
    MOVEGEN_ASSERTIONS();

    TMOVE* begin = moves;

    constexpr Piece BISHOP = Piece(C, PT_BISHOP);

    Bitboard our_bishops = board.piece_bb(BISHOP);
    Bitboard their_bb    = board.color_bb(opposite_color(C));
    Bitboard occ         = board.occupancy();

    constexpr bool GEN_SIMPLE_CAPTURES =
        TYPE == MoveGenerationType::CAPTURES ||
        TYPE == MoveGenerationType::NOISY    ||
        TYPE == MoveGenerationType::ALL;

    constexpr bool GEN_QUIET =
        TYPE == MoveGenerationType::QUIET    ||
        TYPE == MoveGenerationType::ALL;

    Bitboard src_squares = our_bishops;
    while (src_squares) {
        Square src       = lsb(src_squares);
        Bitboard attacks = bishop_attacks(src, occ);

        if constexpr (GEN_SIMPLE_CAPTURES) {
            Bitboard capture_dsts = attacks & their_bb;
            while (capture_dsts) {
                Square dst   = lsb(capture_dsts);
                *moves++     = Move::new_simple_capture(src, dst, BISHOP, board.piece_at(dst));
                capture_dsts = unset_lsb(capture_dsts);
            }
        }

        if constexpr (GEN_QUIET) {
            Bitboard normal_dsts = attacks & (~occ);
            while (normal_dsts) {
                Square dst  = lsb(normal_dsts);
                *moves++    = Move::new_normal(src, dst, BISHOP);
                normal_dsts = unset_lsb(normal_dsts);
            }
        }

        src_squares = unset_lsb(src_squares);
    }

    return moves - begin;
}

template <Color C,
          MoveGenerationType TYPE,
          typename TMOVE>
size_t generate_rook_moves_by_color(const Board& board, TMOVE* moves) {
    MOVEGEN_ASSERTIONS();

    TMOVE* begin = moves;

    constexpr Piece ROOK = Piece(C, PT_ROOK);

    Bitboard our_rooks = board.piece_bb(ROOK);
    Bitboard their_bb  = board.color_bb(opposite_color(C));
    Bitboard occ       = board.occupancy();

    constexpr bool GEN_SIMPLE_CAPTURES =
        TYPE == MoveGenerationType::CAPTURES ||
        TYPE == MoveGenerationType::NOISY    ||
        TYPE == MoveGenerationType::ALL;

    constexpr bool GEN_QUIET =
        TYPE == MoveGenerationType::QUIET    ||
        TYPE == MoveGenerationType::ALL;

    Bitboard src_squares = our_rooks;
    while (src_squares) {
        Square src       = lsb(src_squares);
        Bitboard attacks = rook_attacks(src, occ);

        if constexpr (GEN_SIMPLE_CAPTURES) {
            Bitboard capture_dsts = attacks & their_bb;
            while (capture_dsts) {
                Square dst   = lsb(capture_dsts);
                *moves++     = Move::new_simple_capture(src, dst, ROOK, board.piece_at(dst));
                capture_dsts = unset_lsb(capture_dsts);
            }
        }

        if constexpr (GEN_QUIET) {
            Bitboard normal_dsts = attacks & (~occ);
            while (normal_dsts) {
                Square dst  = lsb(normal_dsts);
                *moves++    = Move::new_normal(src, dst, ROOK);
                normal_dsts = unset_lsb(normal_dsts);
            }
        }

        src_squares = unset_lsb(src_squares);
    }

    return moves - begin;
}

template <Color C,
          MoveGenerationType TYPE,
          typename TMOVE>
size_t generate_queen_moves_by_color(const Board& board, TMOVE* moves) {
    MOVEGEN_ASSERTIONS();

    TMOVE* begin = moves;

    constexpr Piece QUEEN = Piece(C, PT_QUEEN);

    Bitboard our_queens = board.piece_bb(QUEEN);
    Bitboard their_bb   = board.color_bb(opposite_color(C));
    Bitboard occ        = board.occupancy();

    constexpr bool GEN_SIMPLE_CAPTURES =
        TYPE == MoveGenerationType::CAPTURES ||
        TYPE == MoveGenerationType::NOISY    ||
        TYPE == MoveGenerationType::ALL;

    constexpr bool GEN_QUIET =
        TYPE == MoveGenerationType::QUIET    ||
        TYPE == MoveGenerationType::ALL;

    Bitboard src_squares = our_queens;
    while (src_squares) {
        Square src       = lsb(src_squares);
        Bitboard attacks = queen_attacks(src, occ);

        if constexpr (GEN_SIMPLE_CAPTURES) {
            Bitboard capture_dsts = attacks & their_bb;
            while (capture_dsts) {
                Square dst   = lsb(capture_dsts);
                *moves++     = Move::new_simple_capture(src, dst, QUEEN, board.piece_at(dst));
                capture_dsts = unset_lsb(capture_dsts);
            }
        }

        if constexpr (GEN_QUIET) {
            Bitboard normal_dsts = attacks & (~occ);
            while (normal_dsts) {
                Square dst  = lsb(normal_dsts);
                *moves++    = Move::new_normal(src, dst, QUEEN);
                normal_dsts = unset_lsb(normal_dsts);
            }
        }

        src_squares = unset_lsb(src_squares);
    }

    return moves - begin;
}

template <Color C,
          MoveGenerationType TYPE,
          bool LEGAL,
          typename TMOVE>
size_t generate_king_moves_by_color(const Board& board, TMOVE* moves) {
    MOVEGEN_ASSERTIONS();

    TMOVE* begin = moves;

    constexpr Piece KING = Piece(C, PT_QUEEN);

    Bitboard their_bb = board.color_bb(opposite_color(C));
    Bitboard occ      = board.occupancy();

    constexpr bool GEN_SIMPLE_CAPTURES =
        TYPE == MoveGenerationType::CAPTURES ||
        TYPE == MoveGenerationType::NOISY    ||
        TYPE == MoveGenerationType::ALL;

    constexpr bool GEN_QUIET =
        TYPE == MoveGenerationType::QUIET    ||
        TYPE == MoveGenerationType::ALL;

    Square src       = board.king_square(C);
    Bitboard attacks = king_attacks(src);

    if constexpr (GEN_SIMPLE_CAPTURES) {
        // Generate captures
        Bitboard capture_dsts = attacks & their_bb;
        while (capture_dsts) {
            Square dst   = lsb(capture_dsts);
            *moves++     = Move::new_simple_capture(src, dst, KING, board.piece_at(dst));
            capture_dsts = unset_lsb(capture_dsts);
        }
    }

    if constexpr (GEN_QUIET) {
        // Generate normal moves
        Bitboard normal_dsts = attacks & (~occ);
        while (normal_dsts) {
            Square dst  = lsb(normal_dsts);
            *moves++    = Move::new_normal(src, dst, KING);
            normal_dsts = unset_lsb(normal_dsts);
        }

        // Generate castles

        for (Side side: SIDES) {
            Square castle_rook_sq = board.castle_rook_square(C, side);
            Bitboard between_kr   = between_bb(src, castle_rook_sq);

            if (between_kr & occ) {
                // Can't castle, pieces between king and rook.
                continue;
            }

            Bitboard their_attacks = board.color_attacks(opposite_color(C));
            Bitboard full_path     = between_bb_inclusive(src, castle_rook_sq);
            if (full_path & their_attacks) {
                // Can't castle, king in check or route being attacked
                // by opposing pieces.
                continue;
            }

            *moves++ = Move::new_castles(src, C, side, castle_rook_sq);
        }
    }

    return moves - begin;
}

template <Color C,
    MoveGenerationType TYPE,
    typename TMOVE>
size_t generate_evasions_by_color(const Board& board, TMOVE* moves) {
    MOVEGEN_ASSERTIONS();
    ILLUMINA_ASSERT(board.in_check());

    constexpr bool GEN_QUIET = TYPE != MoveGenerationType::NOISY_EVASIONS;
    constexpr bool GEN_NOISY = TYPE != MoveGenerationType::QUIET_EVASIONS;

    TMOVE* begin = moves;

    constexpr Piece KING = Piece(C, PT_KING);

    // We'll start by generating king moves to non attacked squares.
    Square king_square  = board.king_square(C);
    Bitboard king_atks  = board.piece_attacks(KING);
    Bitboard their_atks = board.color_attacks(opposite_color(C));
    Bitboard occ        = board.occupancy();

    // Generate quiet king movements.
    if constexpr (GEN_QUIET) {
        Bitboard dsts = king_atks & (~occ);
        while (dsts) {
            Square dst = lsb(dsts);
            *moves++ = Move::new_normal(king_square, dst, KING);
            dsts = unset_lsb(dsts);
        }
    }
    // Generate noisy king movements.
    if constexpr (GEN_NOISY) {
        Bitboard dsts = king_atks & their_atks;
        while (dsts) {
            Square dst = lsb(dsts);
            *moves++ = Move::new_simple_capture(king_square, dst, board.piece_at(dst), KING);
            dsts = unset_lsb(dsts);
        }
    }

    if (board.in_double_check()) {
        // Double check positions only have king legal moves.
        // Since those have already been generated at this point, return.
        return moves - begin;
    }

    // Now, generate piece movements to block attacks towards the king.

    // Find king attacker.
    Square checker_sq   = board.first_attacker_of(opposite_color(C), king_square);
    Piece checker_piece = board.piece_at(checker_sq);

    if (checker_piece.type() == PT_PAWN && board.ep_square() != SQ_NULL) {
        // In this situation, we can assume the check comes from a double push move.
        Bitboard adjacent    = adjacent_bb(checker_sq);
        Bitboard our_pawns   = board.piece_bb(Piece(C, PT_PAWN));
        Bitboard ep_blockers = our_pawns & adjacent;

        while (ep_blockers) {
            Square pawn_sq = lsb(ep_blockers);

            if (!bit_is_set(board.pinned_bb(), pawn_sq)) {
                *moves++ = Move::new_en_passant_capture(pawn_sq, board.ep_square(), C);
            }

            ep_blockers = unset_lsb(ep_blockers);
        }
    }

    if constexpr (GEN_QUIET) {
        Bitboard between = between_bb(king_square, checker_sq);
        while (between) {
            Square s = lsb(between);

            Bitboard all_blockers = board.all_attackers_of(C, s);
            while (all_blockers) {
                Square blocker_sq   = lsb(all_blockers);
                Piece blocker_piece = board.piece_at(blocker_sq);

                // A piece can only block a check if it's not being pinned
                // from another angle.
                if (!bit_is_set(board.pinned_bb(), blocker_sq)) {
                    // Blocker is not pinned
                    if (blocker_piece.type() != PT_PAWN || square_rank(checker_sq) != promotion_rank(C)) {
                        // Blocker is not a pawn or is not going to the promotion rank.
                        *moves++ = Move::new_normal(blocker_sq, checker_sq, blocker_piece);
                    }
                    else {
                        // Blocker is a pawn and is going to the promotion rank.
                        for (PieceType pt: PROMOTION_PIECE_TYPES) {
                            *moves++ = Move::new_simple_promotion(blocker_sq,
                                                                  checker_sq,
                                                                  C, pt);
                        }
                    }
                }

                all_blockers = unset_lsb(all_blockers);
            }

            between = unset_lsb(between);
        }
    }

    // Finally, generate captures from our other pieces against checking opponent pieces.
    if constexpr (GEN_NOISY) {
        Bitboard all_blockers = board.all_attackers_of(C, checker_sq);
        while (all_blockers) {
            Square blocker_sq   = lsb(all_blockers);
            Piece blocker_piece = board.piece_at(blocker_sq);

            // A piece can only block a check if it's not being pinned
            // from another angle.
            if (!bit_is_set(board.pinned_bb(), blocker_sq)) {
                // Blocker is not pinned
                if (blocker_piece.type() != PT_PAWN || square_rank(checker_sq) != promotion_rank(C)) {
                    // Blocker is not a pawn or is not going to the promotion rank.
                    *moves++ = Move::new_simple_capture(blocker_sq, checker_sq, blocker_piece,
                                                        board.piece_at(checker_sq));
                }
                else {
                    // Blocker is a pawn and is going to the promotion rank.
                    for (PieceType pt: PROMOTION_PIECE_TYPES) {
                        *moves++ = Move::new_promotion_capture(blocker_sq,
                                                               checker_sq,
                                                               C,
                                                               checker_piece,
                                                               pt);

                    }
                }
            }

            all_blockers = unset_lsb(all_blockers);
        }
    }

    return moves - begin;
}

#undef MOVEGEN_ASSERTIONS

} // illumina

#endif // ILLUMINA_MOVEGEN_H
