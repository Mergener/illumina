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

    auto* begin = moves;

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
        auto* last = moves - 1;

        for (moves = begin; moves <= last;) {
            auto& move = *moves;
            if (!board.is_move_legal(move)) {
                auto temp = move;
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

    constexpr auto GEN_PAWN   = bit_is_set(PIECE_TYPE_MASK, PT_PAWN);
    constexpr auto GEN_KNIGHT = bit_is_set(PIECE_TYPE_MASK, PT_KNIGHT);
    constexpr auto GEN_BISHOP = bit_is_set(PIECE_TYPE_MASK, PT_BISHOP);
    constexpr auto GEN_ROOK   = bit_is_set(PIECE_TYPE_MASK, PT_ROOK);
    constexpr auto GEN_QUEEN  = bit_is_set(PIECE_TYPE_MASK, PT_QUEEN);
    constexpr auto GEN_KING   = bit_is_set(PIECE_TYPE_MASK, PT_KING);

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

    constexpr auto PAWN = Piece(C, PT_PAWN);

    // Get movement constants
    constexpr auto PUSH_DIR = pawn_push_direction(C);
    constexpr auto LEFT_CAPT_DIR = pawn_left_capture_direction(C);
    constexpr auto RIGHT_CAPT_DIR = pawn_right_capture_direction(C);
    constexpr auto DOUBLE_PUSH_RANK = double_push_dest_rank(C);
    constexpr auto DOUBLE_PUSH_RANK_BB = rank_bb(DOUBLE_PUSH_RANK);
    constexpr auto PROM_RANK_BB = rank_bb(promotion_rank(C));
    constexpr auto BEHIND_PROM_RANK_BB = shift_bb<-PUSH_DIR>(PROM_RANK_BB);

    auto occ = board.occupancy();
    auto their_bb = board.color_bb(opposite_color(C));
    auto our_pawns = board.piece_bb(PAWN);

    // Generate promotion captures
    constexpr auto GEN_PROM_CAPTURES = bit_is_set(MOVE_TYPE_MASK, MT_PROMOTION_CAPTURE);
    if constexpr (GEN_PROM_CAPTURES) {
        // Left captures
        auto left_attacks = shift_bb<LEFT_CAPT_DIR>(our_pawns) & their_bb;

        // Include only promotion rank
        left_attacks &= PROM_RANK_BB;

        while (left_attacks) {
            auto dst = lsb(left_attacks);
            auto src = dst - LEFT_CAPT_DIR;

            for (PieceType pt: PROMOTION_PIECE_TYPES) {
                *moves++ = Move::new_promotion_capture(src, dst, C, board.piece_at(dst), pt);
            }

            left_attacks = unset_lsb(left_attacks);
        }

        // Right captures
        auto right_attacks = shift_bb<RIGHT_CAPT_DIR>(our_pawns) & their_bb;

        // Include only promotion rank
        right_attacks &= PROM_RANK_BB;

        while (right_attacks) {
            auto dst = lsb(right_attacks);
            auto src = dst - RIGHT_CAPT_DIR;

            for (PieceType pt: PROMOTION_PIECE_TYPES) {
                *moves++ = Move::new_promotion_capture(src, dst, C, board.piece_at(dst), pt);
            }

            right_attacks = unset_lsb(right_attacks);
        }
    }

    // Generate push-promotions
    constexpr auto GEN_SIMPLE_PROMOTIONS = bit_is_set(MOVE_TYPE_MASK, MT_SIMPLE_PROMOTION);
    if constexpr (GEN_SIMPLE_PROMOTIONS) {
        // Promoting pawns are one step behind the promotion rank
        auto promoting_pawns = BEHIND_PROM_RANK_BB & our_pawns & (~shift_bb<-PUSH_DIR>(occ));

        while (promoting_pawns) {
            auto src = lsb(promoting_pawns);

            for (PieceType pt: PROMOTION_PIECE_TYPES) {
                auto dst = src + PUSH_DIR;
                *moves++ = Move::new_simple_promotion(src, dst, C, pt);
            }

            promoting_pawns = unset_lsb(promoting_pawns);
        }
    }

    // Generate non-promotion and non-en-passant captures
    constexpr auto GEN_SIMPLE_CAPTURES = bit_is_set(MOVE_TYPE_MASK, MT_SIMPLE_CAPTURE);
    if constexpr (GEN_SIMPLE_CAPTURES) {
        // Left captures
        auto left_attacks = shift_bb<LEFT_CAPT_DIR>(our_pawns) & their_bb;

        // Exclude promotion rank
        left_attacks &= ~PROM_RANK_BB;

        while (left_attacks) {
            auto dst = lsb(left_attacks);
            auto src = dst - LEFT_CAPT_DIR;

            *moves++ = Move::new_simple_capture(src, dst, PAWN, board.piece_at(dst));

            left_attacks = unset_lsb(left_attacks);
        }

        // Right captures
        auto right_attacks = shift_bb<RIGHT_CAPT_DIR>(our_pawns) & their_bb;

        // Exclude promotion rank
        right_attacks &= ~PROM_RANK_BB;

        while (right_attacks) {
            auto dst = lsb(right_attacks);
            auto src = dst - RIGHT_CAPT_DIR;

            *moves++ = Move::new_simple_capture(src, dst, PAWN, board.piece_at(dst));

            right_attacks = unset_lsb(right_attacks);
        }
    }

    // Generate en-passant captures
    constexpr auto GEN_EP = bit_is_set(MOVE_TYPE_MASK, MT_EN_PASSANT);
    if constexpr (GEN_EP) {
        auto ep_square = board.ep_square();
        if (ep_square != SQ_NULL) {
            auto ep_bb = BIT(ep_square);

            auto ep_pawns = (shift_bb<-LEFT_CAPT_DIR>(ep_bb) | shift_bb<-RIGHT_CAPT_DIR>(ep_bb)) & our_pawns;

            while (ep_pawns) {
                auto src = lsb(ep_pawns);
                *moves++ = Move::new_en_passant_capture(src, ep_square, C);
                ep_pawns = unset_lsb(ep_pawns);
            }
        }
    }

    // Generate pawn pushes
    constexpr auto GEN_PUSHES = bit_is_set(MOVE_TYPE_MASK, MT_NORMAL);
    if constexpr (GEN_PUSHES) {
        // Pretend all squares in the promotion rank are occupied so that
        // we prevent "quiet" pushes to the promotion rank.
        auto push_occ = occ | PROM_RANK_BB;

        // Single pushes (prevent pushing to occupied squares)
        auto push_bb = shift_bb<PUSH_DIR>(our_pawns) & ~push_occ;

        for (auto bb = push_bb; bb; bb = unset_lsb(bb)) {
            auto dst = lsb(bb);
            auto src = dst - PUSH_DIR;
            TMOVE move = Move::new_normal(src, dst, PAWN);
            *moves++ = move;
        }

        // Double pushes
        push_bb = shift_bb<PUSH_DIR>(push_bb) & ~push_occ;

        // Filter out pawns that can't perform double pushes
        push_bb &= DOUBLE_PUSH_RANK_BB;

        while (push_bb) {
            auto dst = lsb(push_bb);
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

    constexpr auto KNIGHT = Piece(C, PT_KNIGHT);

    auto our_knights = board.piece_bb(KNIGHT);
    auto their_bb = board.color_bb(opposite_color(C));
    auto occ = board.occupancy();

    constexpr auto GEN_SIMPLE_CAPTURES = bit_is_set(MOVE_TYPE_MASK, MT_SIMPLE_CAPTURE);
    constexpr auto GEN_QUIET = bit_is_set(MOVE_TYPE_MASK, MT_NORMAL);

    auto src_squares = our_knights;
    while (src_squares) {
        auto src = lsb(src_squares);
        auto attacks = knight_attacks(src);

        if constexpr (GEN_SIMPLE_CAPTURES) {
            auto capture_dsts = attacks & their_bb;
            while (capture_dsts) {
                auto dst = lsb(capture_dsts);
                *moves++ = Move::new_simple_capture(src, dst, KNIGHT, board.piece_at(dst));
                capture_dsts = unset_lsb(capture_dsts);
            }
        }

        if constexpr (GEN_QUIET) {
            auto normal_dsts = attacks & (~occ);
            while (normal_dsts) {
                auto dst = lsb(normal_dsts);
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

    constexpr auto BISHOP = Piece(C, PT_BISHOP);

    auto our_bishops = board.piece_bb(BISHOP);
    auto their_bb = board.color_bb(opposite_color(C));
    auto occ = board.occupancy();

    constexpr auto GEN_SIMPLE_CAPTURES = bit_is_set(MOVE_TYPE_MASK, MT_SIMPLE_CAPTURE);
    constexpr auto GEN_QUIET = bit_is_set(MOVE_TYPE_MASK, MT_NORMAL);

    auto src_squares = our_bishops;
    while (src_squares) {
        auto src = lsb(src_squares);
        auto attacks = bishop_attacks(src, occ);

        if constexpr (GEN_SIMPLE_CAPTURES) {
            auto capture_dsts = attacks & their_bb;
            while (capture_dsts) {
                auto dst = lsb(capture_dsts);
                *moves++ = Move::new_simple_capture(src, dst, BISHOP, board.piece_at(dst));
                capture_dsts = unset_lsb(capture_dsts);
            }
        }

        if constexpr (GEN_QUIET) {
            auto normal_dsts = attacks & (~occ);
            while (normal_dsts) {
                auto dst = lsb(normal_dsts);
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

    constexpr auto ROOK = Piece(C, PT_ROOK);

    auto our_rooks = board.piece_bb(ROOK);
    auto their_bb = board.color_bb(opposite_color(C));
    auto occ = board.occupancy();

    constexpr auto GEN_SIMPLE_CAPTURES = bit_is_set(MOVE_TYPE_MASK, MT_SIMPLE_CAPTURE);
    constexpr auto GEN_QUIET = bit_is_set(MOVE_TYPE_MASK, MT_NORMAL);

    auto src_squares = our_rooks;
    while (src_squares) {
        auto src = lsb(src_squares);
        auto attacks = rook_attacks(src, occ);

        if constexpr (GEN_SIMPLE_CAPTURES) {
            auto capture_dsts = attacks & their_bb;
            while (capture_dsts) {
                auto dst = lsb(capture_dsts);
                *moves++ = Move::new_simple_capture(src, dst, ROOK, board.piece_at(dst));
                capture_dsts = unset_lsb(capture_dsts);
            }
        }

        if constexpr (GEN_QUIET) {
            auto normal_dsts = attacks & (~occ);
            while (normal_dsts) {
                auto dst = lsb(normal_dsts);
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

    constexpr auto QUEEN = Piece(C, PT_QUEEN);

    auto our_queens = board.piece_bb(QUEEN);
    auto their_bb = board.color_bb(opposite_color(C));
    auto occ = board.occupancy();

    constexpr auto GEN_SIMPLE_CAPTURES = bit_is_set(MOVE_TYPE_MASK, MT_SIMPLE_CAPTURE);
    constexpr auto GEN_QUIET = bit_is_set(MOVE_TYPE_MASK, MT_NORMAL);

    auto src_squares = our_queens;
    while (src_squares) {
        auto src = lsb(src_squares);
        auto attacks = queen_attacks(src, occ);

        if constexpr (GEN_SIMPLE_CAPTURES) {
            auto capture_dsts = attacks & their_bb;
            while (capture_dsts) {
                auto dst = lsb(capture_dsts);
                *moves++ = Move::new_simple_capture(src, dst, QUEEN, board.piece_at(dst));
                capture_dsts = unset_lsb(capture_dsts);
            }
        }

        if constexpr (GEN_QUIET) {
            auto normal_dsts = attacks & (~occ);
            while (normal_dsts) {
                auto dst = lsb(normal_dsts);
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

    constexpr auto KING = Piece(C, PT_KING);

    auto their_bb = board.color_bb(opposite_color(C));
    auto occ = board.occupancy();

    constexpr auto GEN_SIMPLE_CAPTURES = bit_is_set(MOVE_TYPE_MASK, MT_SIMPLE_CAPTURE);
    constexpr auto GEN_QUIET = bit_is_set(MOVE_TYPE_MASK, MT_NORMAL);
    constexpr auto GEN_CASTLES = bit_is_set(MOVE_TYPE_MASK, MT_CASTLES);

    auto src = board.king_square(C);
    auto attacks = king_attacks(src);

    if constexpr (GEN_SIMPLE_CAPTURES) {
        // Generate captures.
        auto capture_dsts = attacks & their_bb;
        while (capture_dsts) {
            auto dst = lsb(capture_dsts);
            *moves++ = Move::new_simple_capture(src, dst, KING, board.piece_at(dst));
            capture_dsts = unset_lsb(capture_dsts);
        }
    }

    if constexpr (GEN_QUIET) {
        // Generate normal moves.
        auto normal_dsts = attacks & (~occ);
        while (normal_dsts) {
            auto dst = lsb(normal_dsts);
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

            auto castle_rook_sq = board.castle_rook_square(C, side);
            auto king_dest = castled_king_square(C, side);
            auto rook_dest = castled_rook_square(C, side);

            auto vacant_path = (between_bb_inclusive(src, king_dest) | between_bb_inclusive(castle_rook_sq, rook_dest));
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

            auto full_path = between_bb_inclusive(src, king_dest);
            while (full_path) {
                auto s = lsb(full_path);

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

    constexpr auto GEN_QUIET              = bit_is_set(MOVE_TYPE_MASK, MT_NORMAL);
    constexpr auto GEN_SIMPLE_CAPTURES    = bit_is_set(MOVE_TYPE_MASK, MT_SIMPLE_CAPTURE);
    constexpr auto GEN_EN_PASSANTS        = bit_is_set(MOVE_TYPE_MASK, MT_EN_PASSANT);
    constexpr auto GEN_SIMPLE_PROMOTIONS  = bit_is_set(MOVE_TYPE_MASK, MT_SIMPLE_PROMOTION);
    constexpr auto GEN_PROMOTION_CAPTURES = bit_is_set(MOVE_TYPE_MASK, MT_SIMPLE_CAPTURE);

    constexpr auto KING = Piece(C, PT_KING);
    constexpr auto THEM = opposite_color(C);

    // We'll start by generating king moves to non attacked squares.
    auto king_square = board.king_square(C);
    auto king_atks = king_attacks(king_square);
    auto occ = board.occupancy();
    auto xray_occ = unset_bit(occ, king_square);
    auto their_pieces = board.color_bb(THEM);

    // Generate quiet king movements.
    if constexpr (GEN_QUIET) {
        auto dsts = king_atks & (~occ);
        while (dsts) {
            auto dst = lsb(dsts);
            if (!board.is_attacked_by(THEM, dst, xray_occ)) {
                *moves++ = Move::new_normal(king_square, dst, KING);
            }
            dsts = unset_lsb(dsts);
        }
    }
    // Generate noisy king movements.
    if constexpr (GEN_SIMPLE_CAPTURES) {
        auto dsts = king_atks & their_pieces;
        while (dsts) {
            auto dst = lsb(dsts);
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
    auto checker_sq = board.first_attacker_of(opposite_color(C), king_square);
    auto checker_piece = board.piece_at(checker_sq);

    if constexpr (GEN_EN_PASSANTS) {
        if (checker_piece.type() == PT_PAWN && board.ep_square() != SQ_NULL) {
            // In this situation, we can assume the check comes from a double push move.
            auto adjacent = adjacent_bb(checker_sq);
            auto our_pawns = board.piece_bb(Piece(C, PT_PAWN));
            auto ep_blockers = our_pawns & adjacent;

            while (ep_blockers) {
                auto pawn_sq = lsb(ep_blockers);

                if (!bit_is_set(board.pinned_bb(), pawn_sq)) {
                    *moves++ = Move::new_en_passant_capture(pawn_sq, board.ep_square(), C);
                }

                ep_blockers = unset_lsb(ep_blockers);
            }
        }
    }

    if constexpr (GEN_QUIET || GEN_SIMPLE_PROMOTIONS) {
        auto between = between_bb(king_square, checker_sq);
        while (between) {
            auto s = lsb(between);

            auto all_blockers = board.all_attackers_of<true, true>(C, s);
            while (all_blockers) {
                auto blocker_sq = lsb(all_blockers);
                auto blocker_piece = board.piece_at(blocker_sq);

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
                            else if (    square_rank(blocker_sq) == pawn_starting_rank(C)
                                     && (between_bb(blocker_sq, s) & occ) == 0) {
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
        auto all_blockers = board.all_attackers_of<false, true>(C, checker_sq);
        while (all_blockers) {
            auto blocker_sq = lsb(all_blockers);
            auto blocker_piece = board.piece_at(blocker_sq);

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
