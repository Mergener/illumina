#include "evaluation.h"

#include "endgame.h"
#include "tunablevalues.h"

namespace illumina {

void Evaluation::on_new_board(const Board& board) {
    m_nnue.clear();
    m_ctm = board.color_to_move();

    // Activate every feature individually.
    Bitboard bb = board.occupancy();
    while (bb) {
        Square s = lsb(bb);
        m_nnue.enable_feature(s, board.piece_at(s));
        bb = unset_lsb(bb);
    }
}

void Evaluation::on_make_move(const Board& board, Move move) {
    m_nnue.push_accumulator();

    m_ctm = opposite_color(m_ctm);

    // Store reused values.
    Square source      = move.source();
    Square destination = move.destination();
    Piece source_piece = move.source_piece();
    Color moving_color = source_piece.color();

    if (source != destination) {
        // Every move type always removes the source piece from the original square,
        // unless the piece is moving to the same square (FRC castling).
        m_nnue.disable_feature(source, source_piece);

        // Deactivate feature when moving to previously occupied squares (aka captures except en-passants).
        Piece dest_piece = board.piece_at(move.destination());
        if (dest_piece != PIECE_NULL) {
            m_nnue.disable_feature(destination, dest_piece);
        }

        // Activate the feature that matches the newly positioned piece.
        m_nnue.enable_feature(destination, !move.is_promotion()
                                           ? source_piece
                                           : Piece(moving_color, move.promotion_piece_type()));
    }

    // Castles need to activate/deactivate the castled rook properly.
    if (move.type() == MT_CASTLES) {
        Side side  = move.castles_side();
        Piece rook = Piece(moving_color, PT_ROOK);

        Square rook_src  = move.castles_rook_src_square();
        Square rook_dest = castled_rook_square(moving_color, side);

        if (rook_src != rook_dest) {
            if (rook_src != destination) {
                m_nnue.disable_feature(rook_src, rook);
            }
            m_nnue.enable_feature(rook_dest, rook);
        }
    }

    // En passants must deactivate the square right behind the moved pawn.
    if (move.type() == MT_EN_PASSANT) {
        Square captured_ep_square = move.destination() - pawn_push_direction(moving_color);
        m_nnue.disable_feature(captured_ep_square, Piece(opposite_color(moving_color), PT_PAWN));
    }
}

void Evaluation::on_undo_move(const Board& board, Move move) {
    m_ctm = opposite_color(m_ctm);
    m_nnue.pop_accumulator();
}

void Evaluation::on_make_null_move(const Board& board) {
    m_ctm = opposite_color(m_ctm);
}

void Evaluation::on_undo_null_move(const Board& board) {
    m_ctm = opposite_color(m_ctm);
}

static Score dampen_evaluation(Score score) {
    if (score < EVAL_DAMPENING_THRESHOLD) {
        return score;
    }
    return EVAL_DAMPENING_THRESHOLD
         + dampen_evaluation(((score - EVAL_DAMPENING_THRESHOLD) * EVAL_DAMPENING_FACTOR) / 1024);
}

Score Evaluation::get() const {
    Score score = std::clamp(m_nnue.forward(m_ctm), -KNOWN_WIN + 1, KNOWN_WIN - 1);
    return score > 0
         ?  dampen_evaluation(score)
         : -dampen_evaluation(-score);
}

} // illumina