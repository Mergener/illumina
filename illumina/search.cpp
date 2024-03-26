#include "search.h"

#include "timemanager.h"
#include "movepicker.h"
#include "evaluation.h"

namespace illumina {

class SearchContext {
public:
    TimeManager&               time_manager() const;
    TranspositionTable&        tt() const;
    const Searcher::Listeners& listeners() const;

    void stop_search() const;
    bool should_stop() const;

    SearchContext(TranspositionTable* tt,
                  bool* should_stop,
                  const Searcher::Listeners* listeners,
                  TimeManager* time_manager);

private:
    TranspositionTable* m_tt;
    const Searcher::Listeners* m_listeners;
    bool* m_stop;
    TimeManager* m_time_manager;
};

inline TranspositionTable& SearchContext::tt() const {
    return *m_tt;
}

inline bool SearchContext::should_stop() const {
    return *m_stop;
}

inline const Searcher::Listeners& SearchContext::listeners() const {
    return *m_listeners;
}

SearchContext::SearchContext(TranspositionTable* tt,
                             bool* should_stop,
                             const Searcher::Listeners* listeners,
                             TimeManager* time_manager)
                             : m_stop(should_stop),
                             m_tt(tt), m_listeners(listeners),
                             m_time_manager(time_manager) { }

void SearchContext::stop_search() const {
    *m_stop = true;
}

TimeManager& SearchContext::time_manager() const {
    return *m_time_manager;
}

struct WorkerResults {
    Move  best_move = MOVE_NULL;
    Score score     = 0;
    ui64  nodes     = 0;
};

class SearchWorker {
public:
    void iterative_deepening();
    bool should_stop() const;
    void check_time_bounds();

    const WorkerResults& results() const;

    SearchWorker(bool main,
                 Board board,
                 SearchContext* context,
                 const SearchSettings* settings);

private:
    const SearchSettings* m_settings;
    const SearchContext*  m_context;
    WorkerResults         m_results;

    Evaluation m_eval {};
    Board      m_board;
    bool       m_main;

    Score quiescence_search(Depth ply, Score alpha, Score beta);

    template <NodeType NODE, bool ROOT = false>
    Score pvs(Depth depth,
              Depth ply,
              Score alpha,
              Score beta);

    void aspiration_windows(Depth depth);

    void make_move(Move move);
    void undo_move();
};

inline void SearchWorker::make_move(Move move) {
    m_board.make_move(move);
    m_eval.on_make_move(m_board);
}

inline void SearchWorker::undo_move() {
    m_board.undo_move();
    m_eval.on_undo_move(m_board);
}

inline bool SearchWorker::should_stop() const {
    return m_context->should_stop();
}

inline SearchWorker::SearchWorker(bool main,
                                  Board board,
                                  SearchContext* context,
                                  const SearchSettings* settings)
    : m_main(main), m_board(std::move(board)),
    m_context(context), m_settings(settings) {
    m_eval.on_new_board(m_board);
}

inline const WorkerResults& SearchWorker::results() const {
    return m_results;
}

Score SearchWorker::quiescence_search(Depth ply, Score alpha, Score beta) {
    m_results.nodes++;

    Score stand_pat = m_eval.get();

    if (stand_pat >= beta) {
        return beta;
    }
    if (stand_pat > alpha) {
        alpha = stand_pat;
    }

    check_time_bounds();
    if (should_stop()) {
        return alpha;
    }

    MovePicker<true> move_picker(m_board);
    SearchMove move;
    while ((move = move_picker.next()) != MOVE_NULL) {
        make_move(move);
        Score score = -quiescence_search(ply + 1, -beta, -alpha);
        undo_move();

        if (score >= beta) {
            alpha = beta;
            break;
        }
        if (score > alpha) {
            alpha = score;
        }
    }

    return alpha;
}

template<NodeType NODE, bool ROOT>
Score SearchWorker::pvs(Depth depth, Depth ply, Score alpha, Score beta) {
    m_results.nodes++;

    if (should_stop()) {
        return alpha;
    }

    if (m_board.is_repetition_draw(2)) {
        return 0;
    }

    if (depth <= 0) {
        return quiescence_search(ply, alpha, beta);
    }

    int n_searched_moves = 0;

    MovePicker move_picker(m_board);
    Move move;
    Move best_move;
    while ((move = move_picker.next()) != MOVE_NULL) {
        if constexpr (ROOT) {
            if (m_results.best_move == MOVE_NULL) {
                m_results.best_move = move;
            }
        }

        n_searched_moves++;


        make_move(move);
        Score score = -pvs<NT_PV>(depth - 1, ply + 1, -beta, -alpha);
        undo_move();

        if (score >= beta) {
            alpha = beta;
            break;
        }
        if (score > alpha) {
            alpha = score;
            best_move = move;
        }
    }

    if constexpr (ROOT) {
        if (!should_stop()) {
            m_results.score = alpha;
            m_results.best_move = best_move;
        }
    }

    if (n_searched_moves == 0) {
        return m_board.in_check() ? (-MATE_SCORE + ply) : 0;
    }

    return alpha;
}

void SearchWorker::aspiration_windows(Depth depth) {
    pvs<NT_PV, true>(depth, 0, -MAX_SCORE, MAX_SCORE);

    if (m_main) {
        PVResults results;
        results.depth     = depth;
        results.best_move = m_results.best_move;
        results.score     = m_results.score;
        results.nodes     = m_results.nodes;
        results.time      = m_context->time_manager().elapsed();

        m_context->listeners().pv_finish(results);
    }
}

void SearchWorker::iterative_deepening() {
    Move moves[MAX_GENERATED_MOVES];
    generate_moves(m_board, moves);
    m_results.best_move = moves[0];

    Depth max_depth = m_settings->max_depth.value_or(MAX_DEPTH);
    for (Depth d = 1; d <= max_depth; ++d) {
        check_time_bounds();

        aspiration_windows(d);
        if (should_stop()) {
            break;
        }
    }
}

void SearchWorker::check_time_bounds() {
    if (!m_main) {
        return;
    }

    if (m_results.nodes % 1024 != 0) {
        return;
    }

    if (m_context->time_manager().finished_hard()) {
        m_context->stop_search();
    }
}

SearchResults Searcher::search(const Board& board,
                               const SearchSettings& settings) {
    // Create search context.
    m_stop = false;
    SearchContext context(&m_tt, &m_stop, &m_listeners, &m_tm);

    SearchWorker worker(true, board, &context, &settings);

    if (settings.move_time.has_value()) {
        m_tm.start_movetime(settings.move_time.value());
    }
    else if (board.color_to_move() == CL_WHITE && settings.white_time.has_value()) {
        m_tm.start_tourney_time(settings.white_time.value(), 0, 0, 0);
    }
    else if (board.color_to_move() == CL_BLACK && settings.black_time.has_value()) {
        m_tm.start_tourney_time(settings.black_time.value(), 0, 0, 0);
    }

    worker.iterative_deepening();

    const WorkerResults& worker_results = worker.results();

    SearchResults results {};
    results.score     = worker_results.score;
    results.best_move = worker_results.best_move;

    return results;
}

void Searcher::stop() {
    m_stop = true;
}

}