#include "evaluation.h"

#include "endgame.h"

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
    Color moved_color = m_ctm;
    m_ctm = opposite_color(m_ctm);

    switch (move.type()) {
        case MT_EN_PASSANT:
            m_nnue.update_features<1, 2>(
                    {move.destination()},
                    {move.source_piece()},
                    {move.source(), move.destination() - pawn_push_direction(moved_color)},
                    {move.source_piece(), Piece(m_ctm, PT_PAWN)});
            break;
        case MT_CASTLES:
            m_nnue.update_features<2, 2>(
                    {castled_rook_square(moved_color, move.castles_side()), move.destination()},
                    {Piece(moved_color, PT_ROOK), move.source_piece()},
                    {move.castles_rook_src_square(), move.source()},
                    {Piece(moved_color, PT_ROOK), move.source_piece()});
            break;
        case MT_PROMOTION_CAPTURE:
            m_nnue.update_features<1, 2>(
                    {move.destination()},
                    {Piece(moved_color, move.promotion_piece_type())},
                    {move.source(), move.destination()},
                    {move.source_piece(), move.captured_piece()});
            break;
        case MT_SIMPLE_CAPTURE:
            m_nnue.update_features<1, 2>(
                    {move.destination()},
                    {move.source_piece()},
                    {move.source(), move.destination()},
                    {move.source_piece(), move.captured_piece()});
            break;
        case MT_SIMPLE_PROMOTION:
            m_nnue.update_features<1, 1>(
                    {move.destination()},
                    {Piece(moved_color, move.promotion_piece_type())},
                    {move.source()},
                    {move.source_piece()});
            break;
        default:
            m_nnue.update_features<1, 1>(
                    {move.destination()},
                    {move.source_piece()},
                    {move.source()},
                    {move.source_piece()});
            break;
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

void Evaluation::on_piece_added(const Board &board, Piece p, Square s) {
}

void Evaluation::on_piece_removed(const Board &board, Piece p, Square s) {
}

Score Evaluation::get() const {
    return std::clamp(m_nnue.forward(m_ctm), -KNOWN_WIN + 1, KNOWN_WIN - 1);
}

} // illumina