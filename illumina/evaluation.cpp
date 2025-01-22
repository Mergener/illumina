#include "evaluation.h"

#include "endgame.h"

#include <cmath>

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

static std::pair<double, double> wdl_params(Score score, const Board& board) {
    // Stockfish WDL normalization model parameters.
    // Generated using https://github.com/official-stockfish/WDL_model.
    constexpr double AS[] = {-176.42805858, 569.77274253, -736.31599233, 531.80084725};
    constexpr double BS[] = {2.57937245, 4.53183327, -13.35043366, 91.14410685};

    int material = 1 * popcount(board.piece_type_bb(PT_PAWN))
                   + 3 * popcount(board.piece_type_bb(PT_KNIGHT))
                   + 3 * popcount(board.piece_type_bb(PT_BISHOP))
                   + 5 * popcount(board.piece_type_bb(PT_ROOK))
                   + 9 * popcount(board.piece_type_bb(PT_QUEEN));

    double x = std::clamp(material, 17, 78) / 58.0;

    double p_a = ((AS[0] * x / 58 + AS[1]) * x / 58 + AS[2]) * x / 58 + AS[3];
    double p_b = ((BS[0] * x / 58 + BS[1]) * x / 58 + BS[2]) * x / 58 + BS[3];

    return { p_a, p_b };
}

Score normalize_eval(Score score, const Board& board) {
    if (score == 0 || std::abs(score) >= KNOWN_WIN) {
        return score;
    }

    auto [a, _] = wdl_params(score, board);
    return Score(std::round(100.0 * double(score) / a));
}

} // illumina