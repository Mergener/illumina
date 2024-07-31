#ifndef ILLUMINA_EVALUATION_H
#define ILLUMINA_EVALUATION_H

#include <istream>
#include <ostream>

#include "board.h"
#include "searchdefs.h"
#include "nnue.h"

namespace illumina {

class Evaluation {
public:
    Score get() const;
    void on_new_board(const Board& board);
    void on_make_move(const Board& board, Move move);
    void on_undo_move(const Board& board, Move move);
    void on_make_null_move(const Board& board);
    void on_undo_null_move(const Board& board);

private:
    NNUE m_nnue;
    Color m_ctm;
};

} // illumina

#endif // ILLUMINA_EVALUATION_H
