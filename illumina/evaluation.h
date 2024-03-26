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

} // illumina

#endif // ILLUMINA_EVALUATION_H
