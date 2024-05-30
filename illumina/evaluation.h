#ifndef ILLUMINA_EVALUATION_H
#define ILLUMINA_EVALUATION_H

#include "board.h"
#include "searchdefs.h"

namespace illumina {

class Evaluation {
public:
    Score get() const;
    void on_new_board(const Board& board);
    void on_make_move(const Board& board);
    void on_undo_move(const Board& board);

private:
    Score m_score_white = 0;
    Color m_ctm = CL_WHITE;
};

inline Score Evaluation::get() const {
    return m_ctm == CL_WHITE ? m_score_white : -m_score_white;
}

inline void Evaluation::on_make_move(const Board& board) {
    on_new_board(board);
}

inline void Evaluation::on_undo_move(const Board& board) {
    on_new_board(board);
}


inline int evaluation_phase(const Board& board) {
    int phase_pawns   = int(popcount(board.piece_type_bb(PT_PAWN)) * 1);
    int phase_knights = int(popcount(board.piece_type_bb(PT_KNIGHT)) * 3);
    int phase_bishops = int(popcount(board.piece_type_bb(PT_BISHOP)) * 3);
    int phase_rooks   = int(popcount(board.piece_type_bb(PT_ROOK)) * 5);
    int phase_queens  = int(popcount(board.piece_type_bb(PT_QUEEN)) * 9);

    return phase_pawns  + phase_knights
           + phase_bishops + phase_rooks
           + phase_queens;
}

inline Score transition_eval_score(int phase, Score mg, Score eg) {
    constexpr int START_PHASE = 9 * 2  +
                                5 * 4 +
                                3 * 4 +
                                3 * 4 +
                                1 * 16;

    int mg_phase = phase;
    int eg_phase = START_PHASE - phase;

    return (mg * mg_phase + eg * eg_phase) / START_PHASE;
}

Score mg_psqt_score(Piece p, Square square);
Score eg_psqt_score(Piece p, Square square);

} // illumina

#endif // ILLUMINA_EVALUATION_H
