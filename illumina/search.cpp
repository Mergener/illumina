#include "search.h"

#include <cmath>

#include "timemanager.h"
#include "movepicker.h"
#include "evaluation.h"

namespace illumina {

static std::array<std::array<Depth, MAX_DEPTH>, MAX_GENERATED_MOVES> s_lmr_table;

static void init_lmr_table() {
    for (size_t m = 0; m < MAX_GENERATED_MOVES; ++m) {
        for (Depth d = 0; d < MAX_DEPTH; ++d) {
            s_lmr_table[m][d] = Depth(1.25 + std::log(d) * std::log(m) * 100.0 / 267.0);
        }
    }
}

void init_search() {
    init_lmr_table();
}

class SearchContext {
public:
    TimeManager&               time_manager() const;
    TranspositionTable&        tt() const;
    const Searcher::Listeners& listeners() const;

    ui64 elapsed() const;

    void stop_search() const;
    bool should_stop() const;

    SearchContext(TranspositionTable* tt,
                  bool* should_stop,
                  const Searcher::Listeners* listeners,
                  TimeManager* time_manager);

private:
    TranspositionTable*        m_tt;
    const Searcher::Listeners* m_listeners;
    bool*                      m_stop;
    TimeManager*               m_time_manager;
    TimePoint                  m_search_start;
};

inline TranspositionTable& SearchContext::tt() const {
    return *m_tt;
}

inline bool SearchContext::should_stop() const {
    return *m_stop;
}

inline ui64 SearchContext::elapsed() const {
    return delta_ms(now(), m_search_start);
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
      m_time_manager(time_manager),
      m_search_start(now()) { }

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
    BoundType bound_type = BT_EXACT;
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

    MoveHistory m_hist;
    Evaluation  m_eval {};
    Board       m_board;
    bool        m_main;
    Depth       m_curr_depth;

    Score quiescence_search(Depth ply, Score alpha, Score beta);

    template <bool PV, bool SKIP_NULL = false, bool ROOT = false>
    Score pvs(Depth depth,
              Depth ply,
              Score alpha,
              Score beta);

    void aspiration_windows();

    void make_move(Move move);
    void undo_move();
    void make_null_move();
    void undo_null_move();

    void update_results();
};

inline void SearchWorker::make_move(Move move) {
    m_board.make_move(move);
    m_eval.on_make_move(m_board);
}

inline void SearchWorker::undo_move() {
    m_board.undo_move();
    m_eval.on_undo_move(m_board);
}

inline void SearchWorker::make_null_move() {
    m_board.make_null_move();
    m_eval.on_new_board(m_board);
}

inline void SearchWorker::undo_null_move() {
    m_board.undo_null_move();
    m_eval.on_new_board(m_board);
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

    MovePicker<true> move_picker(m_board, ply, m_hist);
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

template<bool PV, bool SKIP_NULL, bool ROOT>
Score SearchWorker::pvs(Depth depth, Depth ply, Score alpha, Score beta) {
    m_results.nodes++;

    if (should_stop()) {
        return alpha;
    }

    if (!ROOT && (m_board.is_repetition_draw(2) || (m_board.rule50() >= 100) || m_board.is_insufficient_material_draw())) {
        return 0;
    }

    // Setup some important values.
    TranspositionTable& tt = m_context->tt();
    Score original_alpha   = alpha;
    int n_searched_moves   = 0;
    Move best_move         = MOVE_NULL;
    Move hash_move         = MOVE_NULL;
    ui64  board_key        = m_board.hash_key();
    bool in_check          = m_board.in_check();
    Color us               = m_board.color_to_move();

    // Probe from transposition table. This will allow us
    // to use information gathered in other searches (or transpositions
    // to improve the current search.
    TranspositionTableEntry tt_entry {};
    bool found_in_tt = tt.probe(board_key, tt_entry);
    if (found_in_tt) {
        hash_move = tt_entry.move();

        if (tt_entry.depth() >= depth) {
            if (!ROOT && tt_entry.bound_type() == BT_EXACT) {
                // TT Cuttoff. We found a TT entry that was searched in a depth higher or
                // equal to ours, so we have an accurate score of this position and don't
                // need to search further here.
                return tt_entry.score();
            }
            else if (tt_entry.bound_type() == BT_LOWERBOUND) {
                alpha = tt_entry.score();
            }
            else {
                beta = tt_entry.score();
            }
        }
    }

    if (depth <= 0) {
        return quiescence_search(ply, alpha, beta);
    }

    Score static_eval = m_eval.get();

    // Reverse futility pruning.
    Score rfp_margin = 50 + 70 * depth;
    if (!PV        &&
        !in_check  &&
        depth <= 7 &&
        alpha < MATE_THRESHOLD &&
        static_eval - rfp_margin > beta) {
        return static_eval - rfp_margin;
    }

    // Null move pruning.
    if (!PV        &&
        !SKIP_NULL &&
        !in_check  &&
        popcount(m_board.color_bb(us)) >= 4 &&
        static_eval >= beta &&
        depth >= 3) {
        Depth reduction = std::max(depth, std::min(2, 2 + (static_eval - beta) / 200));

        make_null_move();
        Score score = -pvs<false, true>(depth - 1 - reduction, ply + 1, -beta, -beta + 1);
        undo_null_move();

        if (score >= beta) {
            return beta;
        }
    }

    MovePicker move_picker(m_board, ply, m_hist, hash_move);
    Move move {};
    while ((move = move_picker.next()) != MOVE_NULL) {
        // SEE pruning.
        if (depth <= 8          &&
            !m_board.in_check() &&
            move_picker.stage() > MPS_GOOD_CAPTURES &&
            !has_good_see(m_board, move.source(), move.destination(), -2)) {
            continue;
        }

        make_move(move);

        // Late move reductions.
        Depth reductions = 0;
        if (n_searched_moves >= 1 &&
            depth >= 2            &&
            move.is_quiet()       &&
            !m_board.in_check()) {
            reductions += s_lmr_table[n_searched_moves - 1][depth];
        }

        Score score;
        if (n_searched_moves == 0) {
            // Perform PVS. First move of the list is always PVS.
            score = -pvs<true>(depth - 1, ply + 1, -beta, -alpha);
        }
        else {
            // Perform a null window search. Searches after the first move are
            // performed with a null window. If the search fails high, do a
            // re-search with the full window.
            score = -pvs<false>(depth - 1 - reductions, ply + 1, -alpha - 1, -alpha);
            if (score > alpha && score < beta) {
                score = -pvs<true>(depth - 1, ply + 1, -beta, -alpha);
            }
        }
        undo_move();

        n_searched_moves++;

        if (score >= beta) {
            alpha     = beta;
            best_move = move;

            if (move.is_quiet()) {
                m_hist.set_killer(ply, move);
                m_hist.increment_history_score(move, depth);
            }

            if (ROOT && m_main && (!should_stop() || depth <= 2)) {
                m_results.best_move = move;
                m_results.score     = alpha;
            }
            break;
        }
        if (score > alpha) {
            alpha     = score;
            best_move = move;

            if (ROOT && m_main && (!should_stop() || depth <= 2)) {
                m_results.best_move = move;
                m_results.score     = alpha;
            }
        }
    }

    // Check if we have a checkmate or stalemate.
    if (n_searched_moves == 0) {
        return m_board.in_check() ? (-MATE_SCORE + ply) : 0;
    }

    if (should_stop()) {
        return alpha;
    }

    // Store in transposition table.
    if (alpha >= beta) {
        // Beta-Cutoff, lowerbound score.
        tt.try_store(board_key, best_move, alpha, depth, BT_LOWERBOUND);
    }
    else if (alpha <= original_alpha) {
        // Couldn't raise alpha, score is an upperbound.
        tt.try_store(board_key, best_move, alpha, depth, BT_UPPERBOUND);
    }
    else {
        // We have an exact score.
        tt.try_store(board_key, best_move, alpha, depth, BT_EXACT);
    }

    return alpha;
}

void SearchWorker::aspiration_windows() {
    Score prev_score = m_results.score;
    Score alpha      = -MAX_SCORE;
    Score beta       = MAX_SCORE;
    Score window     = 50;
    Depth depth      = m_curr_depth;

    if (depth >= 6) {
        alpha = std::max(-MAX_SCORE, prev_score - window);
        beta  = std::min(MAX_SCORE,  prev_score + window);
    }

    Move best_move = m_results.best_move;

    while (!should_stop()) {
        Score score = pvs<true, false, true>(depth, 0, alpha, beta);

        if (score > alpha && score < beta) {
            m_results.bound_type = BT_EXACT;
            update_results();
            break;
        }

        if (score <= alpha) {
            beta  = (alpha + beta) / 2;
            alpha = std::max(-MAX_SCORE, alpha - window);
            depth = m_curr_depth;

            m_results.score     = prev_score;
            m_results.best_move = best_move;
        }
        else if (score >= beta) {
            beta = std::min(MAX_SCORE, beta + window);

            prev_score = score;
            best_move  = m_results.best_move;
            m_results.bound_type = BT_LOWERBOUND;
            update_results();
        }

        check_time_bounds();

        window += window / 2;
    }
}

void SearchWorker::update_results() {
    if (!m_main) {
        return;
    }

    PVResults results;

    results.depth      = m_curr_depth;
    results.best_move  = m_results.best_move;
    results.score      = m_results.score;
    results.nodes      = m_results.nodes;
    results.time       = m_context->elapsed();
    results.bound_type = m_results.bound_type;

    // Extract the PV line.
    results.line.push_back(m_results.best_move);
    m_board.make_move(m_results.best_move);

    TranspositionTableEntry tt_entry;
    while (m_context->tt().probe(m_board.hash_key(), tt_entry)) {
        Move move = tt_entry.move();
        if (move == MOVE_NULL) {
            break;
        }

        results.line.push_back(tt_entry.move());
        m_board.make_move(tt_entry.move());

        if (m_board.is_repetition_draw()) {
            break;
        }
    }

    for (Move _: results.line) {
        m_board.undo_move();
    }

    m_context->listeners().pv_finish(results);
}

void SearchWorker::iterative_deepening() {
    // Make sure we set a valid move as the best move so that
    // we can return it if our search is interrupted early.
    Move moves[MAX_GENERATED_MOVES];
    generate_moves(m_board, moves);
    m_results.best_move = moves[0];

    Depth max_depth = m_settings->max_depth.value_or(MAX_DEPTH);
    for (m_curr_depth = 1; m_curr_depth <= max_depth; ++m_curr_depth) {
        check_time_bounds();
        aspiration_windows();
        if (should_stop() || m_context->time_manager().finished_soft()) {
            // If we finished soft, we don't want to start a new iteration.
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
    m_tt.new_search();
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
    else {
        m_tm.stop();
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

TranspositionTable& Searcher::tt() {
    return m_tt;
}

}