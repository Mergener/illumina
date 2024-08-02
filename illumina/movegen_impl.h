#include "movegen.h"

namespace illumina {

#define MOVEGEN_ASSERTIONS() \
    static_assert(std::is_assignable_v<TMOVE, Move>, "Move must be assignable to TMOVE"); \
    ILLUMINA_ASSERT(moves != nullptr)

template<ui64 MOVE_TYPE_MASK,
    bool LEGAL,
    ui64 PIECE_TYPE_MASK,
    typename TMOVE>
TMOVE* generate_moves(const Board& board, TMOVE* moves) {
    MOVEGEN_ASSERTIONS();

    TMOVE* begin = moves;

    if (LEGAL && board.in_check()) {
        // When generating legal moves, we only need to generate evasions
        // during check positions.
        return board.color_to_move() == CL_WHITE
             ? generate_evasions_by_color<CL_WHITE, MOVE_TYPE_MASK, TMOVE>(board, moves)
             : generate_evasions_by_color<CL_BLACK, MOVE_TYPE_MASK, TMOVE>(board, moves);
    }

    if (board.color_to_move() == CL_WHITE) {
        moves = generate_moves_by_color<CL_WHITE, MOVE_TYPE_MASK, PIECE_TYPE_MASK, TMOVE>(board, moves);
    }
    else {
        moves = generate_moves_by_color<CL_BLACK, MOVE_TYPE_MASK, PIECE_TYPE_MASK, TMOVE>(board, moves);
    }

    if constexpr (LEGAL) {
        TMOVE* last = moves - 1;

        for (moves = begin; moves <= last;) {
            TMOVE& move = *moves;
            if (!board.is_move_legal(move)) {
                TMOVE temp = move;
                move = *last;
                *last = temp;
                --last;
            }
            else {
                ++moves;
            }
        }
    }

    return moves;
}


template<ui64 MOVE_TYPE_MASK,
         typename TMOVE>
TMOVE* generate_evasions(const Board& board, TMOVE* moves) {
    MOVEGEN_ASSERTIONS();

    if (board.color_to_move() == CL_WHITE) {
        return generate_evasions_by_color<CL_WHITE, MOVE_TYPE_MASK, TMOVE>(board, moves);
    }

    return generate_evasions_by_color<CL_BLACK, MOVE_TYPE_MASK, TMOVE>(board, moves);
}

template<Color C,
    ui64 MOVE_TYPE_MASK,
    ui64 PIECE_TYPE_MASK,
    typename TMOVE>
TMOVE* generate_moves_by_color(const Board& board, TMOVE* moves) {
    MOVEGEN_ASSERTIONS();

    constexpr bool GEN_PAWN   = bit_is_set(PIECE_TYPE_MASK, PT_PAWN);
    constexpr bool GEN_KNIGHT = bit_is_set(PIECE_TYPE_MASK, PT_KNIGHT);
    constexpr bool GEN_BISHOP = bit_is_set(PIECE_TYPE_MASK, PT_BISHOP);
    constexpr bool GEN_ROOK   = bit_is_set(PIECE_TYPE_MASK, PT_ROOK);
    constexpr bool GEN_QUEEN  = bit_is_set(PIECE_TYPE_MASK, PT_QUEEN);
    constexpr bool GEN_KING   = bit_is_set(PIECE_TYPE_MASK, PT_KING);

    if constexpr (GEN_PAWN) {
        moves = generate_pawn_moves_by_color<C, MOVE_TYPE_MASK, TMOVE>(board, moves);
    }
    if constexpr (GEN_KNIGHT) {
        moves = generate_knight_moves_by_color<C, MOVE_TYPE_MASK, TMOVE>(board, moves);
    }
    if constexpr (GEN_BISHOP) {
        moves = generate_bishop_moves_by_color<C, MOVE_TYPE_MASK, TMOVE>(board, moves);
    }
    if constexpr (GEN_ROOK) {
        moves = generate_rook_moves_by_color<C, MOVE_TYPE_MASK, TMOVE>(board, moves);
    }
    if constexpr (GEN_QUEEN) {
        moves = generate_queen_moves_by_color<C, MOVE_TYPE_MASK, TMOVE>(board, moves);
    }
    if constexpr (GEN_KING) {
        moves = generate_king_moves_by_color<C, MOVE_TYPE_MASK, TMOVE>(board, moves);
    }

    return moves;
}

template<Color C,
    ui64 MOVE_TYPE_MASK,
    typename TMOVE>
TMOVE* generate_pawn_moves_by_color(const Board& board, TMOVE* moves) {
    MOVEGEN_ASSERTIONS();

    constexpr Piece PAWN = Piece(C, PT_PAWN);

    // Get movement constants
    constexpr Direction PUSH_DIR = pawn_push_direction(C);
    constexpr Direction LEFT_CAPT_DIR = pawn_left_capture_direction(C);
    constexpr Direction RIGHT_CAPT_DIR = pawn_right_capture_direction(C);
    constexpr BoardRank DOUBLE_PUSH_RANK = double_push_dest_rank(C);
    constexpr Bitboard DOUBLE_PUSH_RANK_BB = rank_bb(DOUBLE_PUSH_RANK);
    constexpr Bitboard PROM_RANK_BB = rank_bb(promotion_rank(C));
    constexpr Bitboard BEHIND_PROM_RANK_BB = shift_bb<-PUSH_DIR>(PROM_RANK_BB);

    Bitboard occ = board.occupancy();
    Bitboard their_bb = board.color_bb(opposite_color(C));
    Bitboard our_pawns = board.piece_bb(PAWN);

    // Generate promotion captures
    constexpr bool GEN_PROM_CAPTURES = bit_is_set(MOVE_TYPE_MASK, MT_PROMOTION_CAPTURE);
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
    constexpr bool GEN_SIMPLE_PROMOTIONS = bit_is_set(MOVE_TYPE_MASK, MT_SIMPLE_PROMOTION);
    if constexpr (GEN_SIMPLE_PROMOTIONS) {
        // Promoting pawns are one step behind the promotion rank
        Bitboard promoting_pawns = BEHIND_PROM_RANK_BB & our_pawns & (~shift_bb<-PUSH_DIR>(occ));

        while (promoting_pawns) {
            Square src = lsb(promoting_pawns);

            for (PieceType pt: PROMOTION_PIECE_TYPES) {
                Square dst = src + PUSH_DIR;
                *moves++ = Move::new_simple_promotion(src, dst, C, pt);
            }

            promoting_pawns = unset_lsb(promoting_pawns);
        }
    }

    // Generate non-promotion and non-en-passant captures
    constexpr bool GEN_SIMPLE_CAPTURES = bit_is_set(MOVE_TYPE_MASK, MT_SIMPLE_CAPTURE);
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
    constexpr bool GEN_EP = bit_is_set(MOVE_TYPE_MASK, MT_EN_PASSANT);
    if constexpr (GEN_EP) {
        Square ep_square = board.ep_square();
        if (ep_square != SQ_NULL) {
            Bitboard ep_bb = BIT(ep_square);

            Bitboard ep_pawns = (shift_bb<-LEFT_CAPT_DIR>(ep_bb) | shift_bb<-RIGHT_CAPT_DIR>(ep_bb)) & our_pawns;

            while (ep_pawns) {
                Square src = lsb(ep_pawns);
                *moves++ = Move::new_en_passant_capture(src, ep_square, C);
                ep_pawns = unset_lsb(ep_pawns);
            }
        }
    }

    // Generate pawn pushes
    constexpr bool GEN_PUSHES = bit_is_set(MOVE_TYPE_MASK, MT_NORMAL);
    if constexpr (GEN_PUSHES) {
        // Pretend all squares in the promotion rank are occupied so that
        // we prevent "quiet" pushes to the promotion rank.
        Bitboard push_occ = occ | PROM_RANK_BB;

        // Single pushes (prevent pushing to occupied squares)
        Bitboard push_bb = shift_bb<PUSH_DIR>(our_pawns) & ~push_occ;

        for (Bitboard bb = push_bb; bb; bb = unset_lsb(bb)) {
            Square dst = lsb(bb);
            Square src = dst - PUSH_DIR;
            TMOVE move = Move::new_normal(src, dst, PAWN);
            *moves++ = move;
        }

        // Double pushes
        push_bb = shift_bb<PUSH_DIR>(push_bb) & ~push_occ;

        // Filter out pawns that can't perform double pushes
        push_bb &= DOUBLE_PUSH_RANK_BB;

        while (push_bb) {
            Square dst = lsb(push_bb);
            *moves++ = Move::new_double_push_from_dest(dst, C);
            push_bb = unset_lsb(push_bb);
        }
    }

    return moves;
}

template<Color C,
    ui64 MOVE_TYPE_MASK,
    typename TMOVE>
TMOVE* generate_knight_moves_by_color(const Board& board, TMOVE* moves) {
    MOVEGEN_ASSERTIONS();

    constexpr Piece KNIGHT = Piece(C, PT_KNIGHT);

    Bitboard our_knights = board.piece_bb(KNIGHT);
    Bitboard their_bb = board.color_bb(opposite_color(C));
    Bitboard occ = board.occupancy();

    constexpr bool GEN_SIMPLE_CAPTURES = bit_is_set(MOVE_TYPE_MASK, MT_SIMPLE_CAPTURE);
    constexpr bool GEN_QUIET = bit_is_set(MOVE_TYPE_MASK, MT_NORMAL);

    Bitboard src_squares = our_knights;
    while (src_squares) {
        Square src = lsb(src_squares);
        Bitboard attacks = knight_attacks(src);

        if constexpr (GEN_SIMPLE_CAPTURES) {
            Bitboard capture_dsts = attacks & their_bb;
            while (capture_dsts) {
                Square dst = lsb(capture_dsts);
                *moves++ = Move::new_simple_capture(src, dst, KNIGHT, board.piece_at(dst));
                capture_dsts = unset_lsb(capture_dsts);
            }
        }

        if constexpr (GEN_QUIET) {
            Bitboard normal_dsts = attacks & (~occ);
            while (normal_dsts) {
                Square dst = lsb(normal_dsts);
                *moves++ = Move::new_normal(src, dst, KNIGHT);
                normal_dsts = unset_lsb(normal_dsts);
            }
        }

        src_squares = unset_lsb(src_squares);
    }

    return moves;
}

template<Color C,
    ui64 MOVE_TYPE_MASK,
    typename TMOVE>
TMOVE* generate_bishop_moves_by_color(const Board& board, TMOVE* moves) {
    MOVEGEN_ASSERTIONS();

    constexpr Piece BISHOP = Piece(C, PT_BISHOP);

    Bitboard our_bishops = board.piece_bb(BISHOP);
    Bitboard their_bb = board.color_bb(opposite_color(C));
    Bitboard occ = board.occupancy();

    constexpr bool GEN_SIMPLE_CAPTURES = bit_is_set(MOVE_TYPE_MASK, MT_SIMPLE_CAPTURE);
    constexpr bool GEN_QUIET = bit_is_set(MOVE_TYPE_MASK, MT_NORMAL);

    Bitboard src_squares = our_bishops;
    while (src_squares) {
        Square src = lsb(src_squares);
        Bitboard attacks = bishop_attacks(src, occ);

        if constexpr (GEN_SIMPLE_CAPTURES) {
            Bitboard capture_dsts = attacks & their_bb;
            while (capture_dsts) {
                Square dst = lsb(capture_dsts);
                *moves++ = Move::new_simple_capture(src, dst, BISHOP, board.piece_at(dst));
                capture_dsts = unset_lsb(capture_dsts);
            }
        }

        if constexpr (GEN_QUIET) {
            Bitboard normal_dsts = attacks & (~occ);
            while (normal_dsts) {
                Square dst = lsb(normal_dsts);
                *moves++ = Move::new_normal(src, dst, BISHOP);
                normal_dsts = unset_lsb(normal_dsts);
            }
        }

        src_squares = unset_lsb(src_squares);
    }

    return moves;
}

template<Color C,
    ui64 MOVE_TYPE_MASK,
    typename TMOVE>
TMOVE* generate_rook_moves_by_color(const Board& board, TMOVE* moves) {
    MOVEGEN_ASSERTIONS();

    constexpr Piece ROOK = Piece(C, PT_ROOK);

    Bitboard our_rooks = board.piece_bb(ROOK);
    Bitboard their_bb = board.color_bb(opposite_color(C));
    Bitboard occ = board.occupancy();

    constexpr bool GEN_SIMPLE_CAPTURES = bit_is_set(MOVE_TYPE_MASK, MT_SIMPLE_CAPTURE);
    constexpr bool GEN_QUIET = bit_is_set(MOVE_TYPE_MASK, MT_NORMAL);

    Bitboard src_squares = our_rooks;
    while (src_squares) {
        Square src = lsb(src_squares);
        Bitboard attacks = rook_attacks(src, occ);

        if constexpr (GEN_SIMPLE_CAPTURES) {
            Bitboard capture_dsts = attacks & their_bb;
            while (capture_dsts) {
                Square dst = lsb(capture_dsts);
                *moves++ = Move::new_simple_capture(src, dst, ROOK, board.piece_at(dst));
                capture_dsts = unset_lsb(capture_dsts);
            }
        }

        if constexpr (GEN_QUIET) {
            Bitboard normal_dsts = attacks & (~occ);
            while (normal_dsts) {
                Square dst = lsb(normal_dsts);
                *moves++ = Move::new_normal(src, dst, ROOK);
                normal_dsts = unset_lsb(normal_dsts);
            }
        }

        src_squares = unset_lsb(src_squares);
    }

    return moves;
}

template<Color C,
    ui64 MOVE_TYPE_MASK,
    typename TMOVE>
TMOVE* generate_queen_moves_by_color(const Board& board, TMOVE* moves) {
    MOVEGEN_ASSERTIONS();

    constexpr Piece QUEEN = Piece(C, PT_QUEEN);

    Bitboard our_queens = board.piece_bb(QUEEN);
    Bitboard their_bb = board.color_bb(opposite_color(C));
    Bitboard occ = board.occupancy();

    constexpr bool GEN_SIMPLE_CAPTURES = bit_is_set(MOVE_TYPE_MASK, MT_SIMPLE_CAPTURE);
    constexpr bool GEN_QUIET = bit_is_set(MOVE_TYPE_MASK, MT_NORMAL);

    Bitboard src_squares = our_queens;
    while (src_squares) {
        Square src = lsb(src_squares);
        Bitboard attacks = queen_attacks(src, occ);

        if constexpr (GEN_SIMPLE_CAPTURES) {
            Bitboard capture_dsts = attacks & their_bb;
            while (capture_dsts) {
                Square dst = lsb(capture_dsts);
                *moves++ = Move::new_simple_capture(src, dst, QUEEN, board.piece_at(dst));
                capture_dsts = unset_lsb(capture_dsts);
            }
        }

        if constexpr (GEN_QUIET) {
            Bitboard normal_dsts = attacks & (~occ);
            while (normal_dsts) {
                Square dst = lsb(normal_dsts);
                *moves++ = Move::new_normal(src, dst, QUEEN);
                normal_dsts = unset_lsb(normal_dsts);
            }
        }

        src_squares = unset_lsb(src_squares);
    }

    return moves;
}

template<Color C,
    ui64 MOVE_TYPE_MASK,
    typename TMOVE>
TMOVE* generate_king_moves_by_color(const Board& board, TMOVE* moves) {
    MOVEGEN_ASSERTIONS();

    constexpr Piece KING = Piece(C, PT_KING);

    Bitboard their_bb = board.color_bb(opposite_color(C));
    Bitboard occ = board.occupancy();

    constexpr bool GEN_SIMPLE_CAPTURES = bit_is_set(MOVE_TYPE_MASK, MT_SIMPLE_CAPTURE);
    constexpr bool GEN_QUIET = bit_is_set(MOVE_TYPE_MASK, MT_NORMAL);
    constexpr bool GEN_CASTLES = bit_is_set(MOVE_TYPE_MASK, MT_CASTLES);

    Square src = board.king_square(C);
    Bitboard attacks = king_attacks(src);

    if constexpr (GEN_SIMPLE_CAPTURES) {
        // Generate captures.
        Bitboard capture_dsts = attacks & their_bb;
        while (capture_dsts) {
            Square dst = lsb(capture_dsts);
            *moves++ = Move::new_simple_capture(src, dst, KING, board.piece_at(dst));
            capture_dsts = unset_lsb(capture_dsts);
        }
    }

    if constexpr (GEN_QUIET) {
        // Generate normal moves.
        Bitboard normal_dsts = attacks & (~occ);
        while (normal_dsts) {
            Square dst = lsb(normal_dsts);
            *moves++ = Move::new_normal(src, dst, KING);
            normal_dsts = unset_lsb(normal_dsts);
        }

    }

    if constexpr (GEN_CASTLES) {
        // Generate castles.
        for (Side side: SIDES) {
            if (!board.has_castling_rights(C, side)) {
                continue;
            }

            Square castle_rook_sq = board.castle_rook_square(C, side);
            Square king_dest = castled_king_square(C, side);
            Square rook_dest = castled_rook_square(C, side);

            Bitboard vacant_path = (between_bb_inclusive(src, king_dest) | between_bb_inclusive(castle_rook_sq, rook_dest));
            vacant_path &= ~board.piece_bb(KING);
            vacant_path &= ~BIT(castle_rook_sq);

            if (vacant_path & occ) {
                // Can't castle, pieces between king and rook.
                continue;
            }

            if (board.is_pinned(castle_rook_sq)) {
                // Can't castle, rook is pinned to the king.
                // This can only happen in FRC.
                continue;
            }

            Bitboard full_path = between_bb_inclusive(src, king_dest);
            while (full_path) {
                Square s = lsb(full_path);

                if (board.is_attacked_by(opposite_color(C), s)) {
                    break;
                }
                full_path = unset_lsb(full_path);
            }

            if (!full_path) {
                *moves++ = Move::new_castles(src, C, side, castle_rook_sq);
            }
        }
    }

    return moves;
}

template<Color C,
    ui64 MOVE_TYPE_MASK,
    typename TMOVE>
TMOVE* generate_evasions_by_color(const Board& board, TMOVE* moves) {
    MOVEGEN_ASSERTIONS();
    ILLUMINA_ASSERT(board.in_check());

    constexpr bool GEN_QUIET              = bit_is_set(MOVE_TYPE_MASK, MT_NORMAL);
    constexpr bool GEN_SIMPLE_CAPTURES    = bit_is_set(MOVE_TYPE_MASK, MT_SIMPLE_CAPTURE);
    constexpr bool GEN_EN_PASSANTS        = bit_is_set(MOVE_TYPE_MASK, MT_EN_PASSANT);
    constexpr bool GEN_SIMPLE_PROMOTIONS  = bit_is_set(MOVE_TYPE_MASK, MT_SIMPLE_PROMOTION);
    constexpr bool GEN_PROMOTION_CAPTURES = bit_is_set(MOVE_TYPE_MASK, MT_SIMPLE_CAPTURE);

    constexpr Piece KING = Piece(C, PT_KING);
    constexpr Color THEM = opposite_color(C);

    // We'll start by generating king moves to non attacked squares.
    Square king_square = board.king_square(C);
    Bitboard king_atks = king_attacks(king_square);
    Bitboard occ = board.occupancy();
    Bitboard xray_occ = unset_bit(occ, king_square);
    Bitboard their_pieces = board.color_bb(THEM);

    // Generate quiet king movements.
    if constexpr (GEN_QUIET) {
        Bitboard dsts = king_atks & (~occ);
        while (dsts) {
            Square dst = lsb(dsts);
            if (!board.is_attacked_by(THEM, dst, xray_occ)) {
                *moves++ = Move::new_normal(king_square, dst, KING);
            }
            dsts = unset_lsb(dsts);
        }
    }
    // Generate noisy king movements.
    if constexpr (GEN_SIMPLE_CAPTURES) {
        Bitboard dsts = king_atks & their_pieces;
        while (dsts) {
            Square dst = lsb(dsts);
            if (!board.is_attacked_by(THEM, dst, xray_occ)) {
                *moves++ = Move::new_simple_capture(king_square, dst, KING, board.piece_at(dst));
            }
            dsts = unset_lsb(dsts);
        }
    }

    if (board.in_double_check()) {
        // Double check positions only have king legal moves.
        // Since those have already been generated at this point, return.
        return moves;
    }

    // Now, generate piece movements to block attacks towards the king.

    // Find king attacker.
    Square checker_sq = board.first_attacker_of(opposite_color(C), king_square);
    Piece checker_piece = board.piece_at(checker_sq);

    if constexpr (GEN_EN_PASSANTS) {
        if (checker_piece.type() == PT_PAWN && board.ep_square() != SQ_NULL) {
            // In this situation, we can assume the check comes from a double push move.
            Bitboard adjacent = adjacent_bb(checker_sq);
            Bitboard our_pawns = board.piece_bb(Piece(C, PT_PAWN));
            Bitboard ep_blockers = our_pawns & adjacent;

            while (ep_blockers) {
                Square pawn_sq = lsb(ep_blockers);

                if (!bit_is_set(board.pinned_bb(), pawn_sq)) {
                    *moves++ = Move::new_en_passant_capture(pawn_sq, board.ep_square(), C);
                }

                ep_blockers = unset_lsb(ep_blockers);
            }
        }
    }

    if constexpr (GEN_QUIET || GEN_SIMPLE_PROMOTIONS) {
        Bitboard between = between_bb(king_square, checker_sq);
        while (between) {
            Square s = lsb(between);

            Bitboard all_blockers = board.all_attackers_of<true, true>(C, s);
            while (all_blockers) {
                Square blocker_sq = lsb(all_blockers);
                Piece blocker_piece = board.piece_at(blocker_sq);

                // A piece can only block a check if it's not being pinned
                // from another angle.
                if (!bit_is_set(board.pinned_bb(), blocker_sq)) {
                    // Blocker is not pinned.
                    if (blocker_piece.type() != PT_PAWN || square_rank(s) != promotion_rank(C)) {
                        if constexpr (GEN_QUIET) {
                            // Blocker is not a pawn or is not going to the promotion rank.
                            if (blocker_piece.type() != PT_PAWN) {
                                *moves++ = Move::new_normal(blocker_sq, s, blocker_piece);
                            }
                            else if (std::abs(s - blocker_sq) == DIR_NORTH) {
                                *moves++ = Move::new_normal(blocker_sq, s, blocker_piece);
                            }
                            else if (square_rank(blocker_sq) == pawn_starting_rank(C) &&
                                    (between_bb(blocker_sq, s) & occ) == 0) {
                                *moves++ = Move::new_double_push(blocker_sq, C);
                            }
                        }
                    }
                    else if constexpr (GEN_SIMPLE_PROMOTIONS) {
                        // Blocker is a pawn and is going to the promotion rank.
                        for (PieceType pt: PROMOTION_PIECE_TYPES) {
                            *moves++ = Move::new_simple_promotion(blocker_sq,
                                                                  s, C, pt);
                        }
                    }
                }

                all_blockers = unset_lsb(all_blockers);
            }

            between = unset_lsb(between);
        }
    }

    // Finally, generate captures from our other pieces against checking opponent pieces.
    if constexpr (GEN_SIMPLE_CAPTURES || GEN_PROMOTION_CAPTURES) {
        Bitboard all_blockers = board.all_attackers_of<false, true>(C, checker_sq);
        while (all_blockers) {
            Square blocker_sq = lsb(all_blockers);
            Piece blocker_piece = board.piece_at(blocker_sq);

            // A piece can only block a check if it's not being pinned
            // from another angle.
            if (!bit_is_set(board.pinned_bb(), blocker_sq)) {
                // Blocker is not pinned
                if (blocker_piece.type() != PT_PAWN || square_rank(checker_sq) != promotion_rank(C)) {
                    if constexpr (GEN_SIMPLE_CAPTURES) {
                        // Blocker is not a pawn or is not going to the promotion rank.
                        *moves++ = Move::new_simple_capture(blocker_sq, checker_sq, blocker_piece,
                                                            board.piece_at(checker_sq));
                    }
                }
                else if constexpr (GEN_PROMOTION_CAPTURES) {
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

    return moves;
}

}

#undef MOVEGEN_ASSERTIONS
