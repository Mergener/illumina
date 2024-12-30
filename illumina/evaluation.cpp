#include "evaluation.h"

#include "endgame.h"

namespace illumina {

void Evaluation::on_new_board(const Board& board) {
    m_undoing_move = false;
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
    m_undoing_move = false;
    m_nnue.push_accumulator();
    m_ctm = opposite_color(m_ctm);
}

void Evaluation::on_undo_move(const Board& board, Move move) {
    m_undoing_move = true;
    m_ctm = opposite_color(m_ctm);
    m_nnue.pop_accumulator();
}

void Evaluation::on_make_null_move(const Board& board) {
    m_ctm = opposite_color(m_ctm);
}

void Evaluation::on_undo_null_move(const Board& board) {
    m_ctm = opposite_color(m_ctm);
}

void Evaluation::on_piece_added(const Board &board, Piece p, Square s) {
    if (m_undoing_move) {
        return;
    }

    m_nnue.enable_feature(s, p);
}

void Evaluation::on_piece_removed(const Board &board, Piece p, Square s) {
    if (m_undoing_move) {
        return;
    }

    m_nnue.disable_feature(s, p);
}

Score Evaluation::get() const {
    return std::clamp(m_nnue.forward(m_ctm), -KNOWN_WIN + 1, KNOWN_WIN - 1);
}

} // illumina