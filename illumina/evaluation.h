#ifndef ILLUMINA_EVALUATION_H
#define ILLUMINA_EVALUATION_H

#include <istream>
#include <ostream>
#include <vector>

#include "board.h"
#include "searchdefs.h"
#include "nnue.h"

namespace illumina {

class Evaluation {
public:
    Score compute(const Board& board);
    int complexity(const Board& board);
    void on_new_board(const Board& board);
    void on_make_move(const Board& board, Move move);
    void on_undo_move(const Board& board, Move move);
    void on_make_null_move(const Board& board);
    void on_undo_null_move(const Board& board);

private:
    EvaluationNNUE m_eval_nnue { default_eval_network(), 400 };
    ComplexityNNUE m_complexity_nnue { default_complexity_network(), 16384 };
    std::array<Move, MAX_DEPTH> m_eval_lazy_updates;
    std::array<Move, MAX_DEPTH> m_complexity_lazy_updates;
    size_t m_n_eval_lazy_updates = 0;
    size_t m_n_complexity_lazy_updates = 0;

    void apply_eval_lazy_updates();
    void apply_complexity_lazy_updates();
};

Score normalize_score(Score score, const Board& board);

struct WDL {
    int w;
    int d;
    int l;
};

WDL wdl_from_score(Score score, const Board& board);

} // illumina

#endif // ILLUMINA_EVALUATION_H
