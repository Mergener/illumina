#include "search.h"

#include <cmath>

#include "timemanager.h"
#include "movepicker.h"
#include "evaluation.h"

namespace illumina {

struct RootInfo {
    /**
     * Intersection between root's legal moves and moves passed through the
     * searchmoves argument (if specified).
     */
    std::vector<Move> moves;

    /** The color to move in the root board. */
    Color color;
};

/**
 * Search node on the search stack.
 */
struct SearchNode {
    Depth ply = 0;
    Score static_eval = 0;
    Move skip_move = MOVE_NULL;
    Move  pv[MAX_DEPTH];
};

/**
 * Search context shared among search workers.
 */
class SearchContext {
public:
    TimeManager&               time_manager() const;
    TranspositionTable&        tt() const;
    const Searcher::Listeners& listeners() const;
    const RootInfo&            root_info() const;

    /**
     * Time, in milliseconds, since this worker started searching.
     * Expensive method, don't call often.
     */
    ui64 elapsed() const;

    void stop_search() const;
    bool should_stop() const;

    SearchContext(TranspositionTable* tt,
                  bool* should_stop,
                  const Searcher::Listeners* listeners,
                  const RootInfo* root_info,
                  TimeManager* time_manager);

private:
    TranspositionTable*        m_tt;
    const Searcher::Listeners* m_listeners;
    const RootInfo*            m_root_info;
    bool*                      m_stop;
    TimeManager*               m_time_manager;
    TimePoint                  m_search_start;
};

TranspositionTable& SearchContext::tt() const {
    return *m_tt;
}

bool SearchContext::should_stop() const {
    return *m_stop;
}

ui64 SearchContext::elapsed() const {
    return delta_ms(now(), m_search_start);
}

const Searcher::Listeners& SearchContext::listeners() const {
    return *m_listeners;
}

SearchContext::SearchContext(TranspositionTable* tt,
                             bool* should_stop,
                             const Searcher::Listeners* listeners,
                             const RootInfo* root_info,
                             TimeManager* time_manager)
    : m_stop(should_stop),
      m_tt(tt), m_listeners(listeners),
      m_time_manager(time_manager),
      m_root_info(root_info),
      m_search_start(now()) { }

void SearchContext::stop_search() const {
    *m_stop = true;
}

TimeManager& SearchContext::time_manager() const {
    return *m_time_manager;
}

const RootInfo& SearchContext::root_info() const {
    return *m_root_info;
}

struct WorkerResults {
    struct {
        Move best_move = MOVE_NULL;
        Move ponder_move = MOVE_NULL;
        Score score = 0;
        BoundType bound_type = BT_EXACT;
    } pv_results[MAX_PVS];
    Depth sel_depth   = 0;
    ui64  nodes       = 0;
};

class SearchWorker {
public:
    void iterative_deepening();
    bool should_stop() const;
    void check_limits();

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
    Depth       m_curr_depth = 1;
    Move        m_curr_move  = MOVE_NULL;
    int         m_curr_move_number = 0;
    int         m_curr_pv_idx = 0;

    std::vector<Move> m_search_moves;

    Score quiescence_search(Depth ply, Score alpha, Score beta);

    template <bool PV, bool SKIP_NULL = false, bool ROOT = false>
    Score pvs(Depth depth,
              Score alpha,
              Score beta,
              SearchNode* stack_node);

    void aspiration_windows();

    void make_move(Move move);
    void undo_move();
    void make_null_move();
    void undo_null_move();

    void report_pv_results(const SearchNode* search_stack);

    Score draw_score() const;
};

void Searcher::stop() {
    m_stop = true;
}

TranspositionTable& Searcher::tt() {
    return m_tt;
}

SearchResults Searcher::search(const Board& board,
                               const SearchSettings& settings) {
    // Create root info data.
    RootInfo root_info;
    root_info.color = board.color_to_move();

    Move legal_moves[MAX_GENERATED_MOVES];
    Move* end = generate_moves(board, &legal_moves[0]);
    for (Move* it = &legal_moves[0]; it != end; ++it) {
        Move move = *it;
        if (settings.search_moves.has_value()) {
            const std::vector<Move>& search_moves = settings.search_moves.value();
            // Skip move if not included in search moves.
            if (std::find(search_moves.begin(), search_moves.end(), move) == search_moves.end()) {
                continue;
            }
        }
        root_info.moves.push_back(move);
    }

    // Create search context.
    m_stop = false;
    m_tt.new_search();
    SearchContext context(&m_tt, &m_stop, &m_listeners, &root_info, &m_tm);

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

    // Fill in the search results object to be returned.
    // We assume that the best move is the one at MultiPV 1.
    // Although this is not necessarily true, we don't want
    // to focus our efforts in improving MultiPV mode since
    // it is already not the one used for playing.
    SearchResults results {};
    results.score       = worker_results.pv_results[0].score;
    results.best_move   = worker_results.pv_results[0].best_move;
    results.ponder_move = worker_results.pv_results[0].ponder_move;

    return results;
}

void SearchWorker::iterative_deepening() {
    Depth max_depth = m_settings->max_depth.value_or(MAX_DEPTH);
    for (m_curr_depth = 1; m_curr_depth <= max_depth; ++m_curr_depth) {
        if (should_stop() || m_context->time_manager().finished_soft()) {
            break;
        }

        // Make sure this thread's search moves list is full once
        // each new iteration starts.
        m_search_moves = m_context->root_info().moves;

        int n_pvs = std::clamp(m_settings->n_pvs, 1, MAX_PVS);
        for (m_curr_pv_idx = 0; m_curr_pv_idx < n_pvs; ++m_curr_pv_idx) {
            // If there are no moves to search, abort.
            if (m_search_moves.empty()) {
                break;
            }

            // Make sure we set a valid move as the best move so that
            // we can return it if our search is interrupted early.
            if (m_results.pv_results[m_curr_pv_idx].best_move == MOVE_NULL) {
                m_results.pv_results[m_curr_pv_idx].best_move = m_search_moves[0];
            }

            check_limits();
            aspiration_windows();
            check_limits();
            if (should_stop() || m_context->time_manager().finished_soft()) {
                // If we finished soft, we don't want to start a new iteration.
                break;
            }

            // When using MultiPV, erase the best searched move and look for a new
            // pv without it.
            Move pv_move = m_results.pv_results[m_curr_pv_idx].best_move;
            auto best_move_it = std::find(m_search_moves.begin(), m_search_moves.end(), pv_move);
            if (best_move_it != m_search_moves.end()) {
                m_search_moves.erase(best_move_it);
            }
        }
        m_curr_pv_idx = 0;
    }
}

void SearchWorker::aspiration_windows() {
    // Prepare the search stack.
    constexpr size_t STACK_SIZE = MAX_DEPTH + 64;
    SearchNode search_stack[STACK_SIZE];
    for (Depth ply = 0; ply < STACK_SIZE; ++ply) {
        SearchNode& node = search_stack[ply];
        node.ply = ply;
    }

    // Prepare aspiration windows.
    Score prev_score = m_results.pv_results[m_curr_pv_idx].score;
    Score alpha      = -MAX_SCORE;
    Score beta       = MAX_SCORE;
    Score window     = 10;
    Depth depth      = m_curr_depth;

    // Don't use aspiration windows in lower depths since
    // their results is still too unstable.
    if (depth >= 6) {
        alpha = std::max(-MAX_SCORE, prev_score - window);
        beta  = std::min(MAX_SCORE,  prev_score + window);
    }

    Move best_move = m_results.pv_results[m_curr_pv_idx].best_move;

    // Perform search with aspiration windows.
    while (!should_stop()) {
        Score score = pvs<true, false, true>(depth, alpha, beta, &search_stack[0]);

        if (score > alpha && score < beta) {
            // We found an exact score within our bounds, finish
            // the current depth search.
            m_results.pv_results[m_curr_pv_idx].bound_type = BT_EXACT;
            report_pv_results(search_stack);
            break;
        }

        if (score <= alpha) {
            beta  = (alpha + beta) / 2;
            alpha = std::max(-MAX_SCORE, alpha - window);
            depth = m_curr_depth;

            m_results.pv_results[m_curr_pv_idx].score     = prev_score;
            m_results.pv_results[m_curr_pv_idx].best_move = best_move;
        }
        else if (score >= beta) {
            beta = std::min(MAX_SCORE, beta + window);

            prev_score = score;
            best_move  = m_results.pv_results[m_curr_pv_idx].best_move;
            m_results.pv_results[m_curr_pv_idx].bound_type = BT_LOWERBOUND;
            report_pv_results(search_stack);
        }

        check_limits();

        window += window / 2;
    }
}

static std::array<std::array<Depth, MAX_DEPTH>, MAX_GENERATED_MOVES> s_lmr_table;
static std::array<std::array<int, MAX_DEPTH>, 2> s_lmp_count_table;

template<bool PV, bool SKIP_NULL, bool ROOT>
Score SearchWorker::pvs(Depth depth, Score alpha, Score beta, SearchNode* node) {
    // Initialize the PV line with a null move. Specially useful for all-nodes.
    if constexpr (PV) {
        node->pv[0] = MOVE_NULL;
    }

    // Keep track on some stats to report or use later.
    m_results.sel_depth = std::max(m_results.sel_depth, node->ply);

    // Make sure we count the root node.
    if constexpr (ROOT) {
        m_results.nodes++;
    }

    // Check if we must stop our search.
    // If so, return the best score we've found so far.
    check_limits();
    if (should_stop()) {
        return alpha;
    }

    // Check for draws and return the draw score, taking contempt into consideration.
    if (!ROOT && (m_board.is_repetition_draw(2) || (m_board.rule50() >= 100) || m_board.is_insufficient_material_draw())) {
        return draw_score();
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
    Depth ply              = node->ply;
    Score& static_eval     = node->static_eval;
    Move skip_move         = node->skip_move;

    // Probe from transposition table. This will allow us
    // to use information gathered in other searches (or transpositions)
    // to improve the current search.
    TranspositionTableEntry tt_entry {};
    bool found_in_tt = tt.probe(board_key, tt_entry, node->ply);
    if (found_in_tt) {
        hash_move = tt_entry.move();

        if (!PV && tt_entry.depth() >= depth) {
            if (!ROOT && tt_entry.bound_type() == BT_EXACT) {
                // TT Cuttoff.
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

    // Check extensions.
    // Extend positions in check.
    if (in_check && ply < MAX_DEPTH && depth < MAX_DEPTH) {
        depth++;
    }

    // Dive into the quiescence search when depth becomes zero.
    if (depth <= 0) {
        return quiescence_search(ply, alpha, beta);
    }

    // Compute the static eval. Useful for many heuristics.
    // If we're on a singular search, assume that the static eval
    // was already calculated.
    if (skip_move == MOVE_NULL) {
        static_eval = !in_check ? m_eval.get() : 0;
    }
    bool improving = ply > 2 && !in_check && ((node - 2)->static_eval < static_eval);

    // Reverse futility pruning.
    // If our position is too good, by a safe margin and low depth, prune.
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
        Score score = -pvs<false, true>(depth - 1 - reduction, -beta, -beta + 1, node + 1);
        undo_null_move();

        if (score >= beta) {
            return beta;
        }
    }

    // Internal iterative reductions.
    if (depth >= 4 && !found_in_tt) {
        depth--;
    }

    // Mate distance pruning.
    Score expected_mate_score = MATE_SCORE - ply;
    if (expected_mate_score < beta) {
        beta = expected_mate_score;
        if (alpha >= beta) {
            return beta;
        }
    }

    // Kickstart our curr move counter for later reporting.
    if (ROOT && m_main) {
        m_curr_move_number = 0;
    }

    // Store played quiet moves in this list.
    // Useful for history updates later on.
    StaticList<Move, MAX_GENERATED_MOVES> quiets_played;

    int move_idx = -1;

    MovePicker move_picker(m_board, ply, m_hist, hash_move);
    Move move {};
    bool has_legal_moves = false;
    while ((move = move_picker.next()) != MOVE_NULL) {
        has_legal_moves = true;
        move_idx++;

        check_limits();
        if (should_stop()) {
            break;
        }

        // Skip unrequested moves.
        if (ROOT &&
            std::find(m_search_moves.begin(), m_search_moves.end(), move) == m_search_moves.end()) {
            continue;
        }

        // Report 'currmove' and 'currmovenumber'.
        if (ROOT && m_main) {
            m_curr_move_number++;
            m_curr_move = move;
            m_context->listeners().curr_move_listener(m_curr_depth,
                                                      m_curr_move,
                                                      m_curr_move_number);
        }

        // Late move pruning.
        if (alpha > -MATE_THRESHOLD &&
            depth <= (8 + m_board.gives_check(move)) &&
            move_idx >= s_lmp_count_table[improving][depth] &&
            move_picker.stage() > MPS_KILLER_MOVES &&
            !in_check &&
            move.is_quiet()) {
            continue;
        }

        // SEE pruning.
        if (depth <= 8          &&
            !m_board.in_check() &&
            move_picker.stage() > MPS_GOOD_CAPTURES &&
            !has_good_see(m_board, move.source(), move.destination(), -2)) {
            continue;
        }

        // Singular extensions.
        Depth extensions = 0;
        if (!ROOT             &&
            move == hash_move &&
            !in_check         &&
            depth >= 8        &&
            skip_move == MOVE_NULL            &&
            tt_entry.depth() >= (depth - 3)   && // Limits excessive extensions
            tt_entry.score() < MATE_THRESHOLD &&
            (tt_entry.bound_type() != BT_UPPERBOUND)) {
            Score se_beta = std::min(beta, tt_entry.score() - 4 - depth * 2);

            node->skip_move = move;
            Score score = pvs<true>((depth - 1) / 2, se_beta - 1, se_beta, node);
            node->skip_move = MOVE_NULL;

            if (score < se_beta) {
                // We have a singular move. Search it with extended depth.
                extensions++;
            }
            else if (score >= beta) {
                return se_beta;
            }
        }

        make_move(move);

        // Futility pruning.
        if (depth <= 6 &&
            !in_check  &&
            !m_board.in_check() &&
            move.is_quiet() &&
            (static_eval + 80) < alpha) {
            undo_move();
            continue;
        }

        // Late move reductions.
        Depth reductions = 0;
        if (n_searched_moves >= 1 &&
            depth >= 2            &&
            !in_check             &&
            !m_board.in_check()) {
            reductions = s_lmr_table[n_searched_moves - 1][depth];
            if (move.is_quiet()) {
                // Further reduce moves that are not improving the static evaluation.
                reductions += !improving;

                // Further reduce moves that have been historically very bad.
                reductions += m_hist.quiet_history(move) <= -400;
            }
            else if (move_picker.stage() == MPS_BAD_CAPTURES) {
                // Further reduce bad captures when we're in a very good position
                // and probably don't need unsound sacrifices.
                bool stable = alpha >= 250;
                reductions -= !stable * (reductions / 2);
            }

            // Prevent too high or below zero reductions.
            reductions = std::clamp(reductions, 0, depth);
        }

        Score score;
        if (n_searched_moves == 0) {
            // Perform PVS. First move of the list is always PVS.
            score = -pvs<true>(depth - 1 + extensions, -beta, -alpha, node + 1);
        }
        else {
            // Perform a null window search. Searches after the first move are
            // performed with a null window. If the search fails high, do a
            // re-search with the full window.
            score = -pvs<false>(depth - 1 - reductions + extensions, -alpha - 1, -alpha, node + 1);
            if (score > alpha && score < beta) {
                score = -pvs<true>(depth - 1 + extensions, -beta, -alpha, node + 1);
            }
        }
        undo_move();

        if (move.is_quiet()) {
            quiets_played.push_back(move);
        }

        n_searched_moves++;

        if (score >= beta) {
            // Our search failed high.
            alpha     = beta;
            best_move = move;

            // Update our history scores and refutation moves.
            if (move.is_quiet()) {
                m_hist.set_killer(ply, move);

                for (Move quiet: quiets_played) {
                    m_hist.update_quiet_history(quiet, depth, quiet == best_move);
                }
            }

            // Due to aspiration windows, our search may have failed high in the root.
            // Make sure we always have a best_move and a best_score.
            if (ROOT && m_main && (!should_stop() || depth <= 2)) {
                m_results.pv_results[m_curr_pv_idx].best_move = move;
                m_results.pv_results[m_curr_pv_idx].score     = alpha;
            }

            if constexpr (PV && !ROOT) {
                node->pv[0] = MOVE_NULL;
            }
            break;
        }
        if (score > alpha) {
            // We've got a new best move.
            alpha     = score;
            best_move = move;

            // Make sure we update our best_move in the root ASAP.
            if (ROOT && m_main && (!should_stop() || depth <= 2)) {
                m_results.pv_results[m_curr_pv_idx].best_move = move;
                m_results.pv_results[m_curr_pv_idx].score     = alpha;
            }

            // Update the PV table.
            if constexpr (PV) {
                node->pv[0] = best_move;
                size_t i;
                for (i = 0; i < MAX_DEPTH - 2; ++i) {
                    Move pv_move = (node + 1)->pv[i];
                    if (pv_move == MOVE_NULL) {
                        break;
                    }
                    node->pv[i + 1] = pv_move;
                }
                node->pv[i + 1] = MOVE_NULL;
            }
        }
    }

    // Check if we have a checkmate or stalemate.
    if (!has_legal_moves) {
        return m_board.in_check() ? (-MATE_SCORE + ply) : 0;
    }

    if (should_stop()) {
        return alpha;
    }

    // Store in transposition table.
    if (alpha >= beta) {
        // Beta-Cutoff, lowerbound score.
        tt.try_store(board_key, ply, best_move, alpha, depth, BT_LOWERBOUND);
    }
    else if (alpha <= original_alpha) {
        // Couldn't raise alpha, score is an upperbound.
        tt.try_store(board_key, ply, best_move, alpha, depth, BT_UPPERBOUND);
    }
    else {
        // We have an exact score.
        tt.try_store(board_key, ply, best_move, alpha, depth, BT_EXACT);
    }

    return alpha;
}

Score SearchWorker::quiescence_search(Depth ply, Score alpha, Score beta) {
    Score stand_pat = m_eval.get();

    if (stand_pat >= beta) {
        return beta;
    }
    if (stand_pat > alpha) {
        alpha = stand_pat;
    }

    check_limits();
    if (should_stop()) {
        return alpha;
    }

    // Finally, start looping over available noisy moves.
    MovePicker<true> move_picker(m_board, ply, m_hist);
    SearchMove move;
    SearchMove best_move;
    while ((move = move_picker.next()) != MOVE_NULL) {
        // SEE pruning.
        if (move_picker.stage() >= MPS_BAD_CAPTURES &&
            !has_good_see(m_board, move.source(), move.destination(), -2)) {
            continue;
        }

        make_move(move);
        Score score = -quiescence_search(ply + 1, -beta, -alpha);
        undo_move();

        if (score >= beta) {
            best_move = move;
            alpha = beta;
            break;
        }
        if (score > alpha) {
            best_move = move;
            alpha = score;
        }
    }

    return alpha;
}

void SearchWorker::report_pv_results(const SearchNode* search_stack) {
    // We only want the main thread to report results, the others
    // should just assist on the search.
    if (!m_main) {
        return;
    }

    PVResults pv_results;
    pv_results.pv_idx     = m_curr_pv_idx;
    pv_results.depth      = m_curr_depth;
    pv_results.sel_depth  = m_results.sel_depth;
    pv_results.score      = m_results.pv_results[m_curr_pv_idx].score;
    pv_results.nodes      = m_results.nodes;
    pv_results.time       = m_context->elapsed();
    pv_results.bound_type = m_results.pv_results[m_curr_pv_idx].bound_type;

    // Extract the PV line.
    pv_results.line.clear();
    for (Move pv_move: search_stack->pv) {
        if (pv_move == MOVE_NULL) {
            break;
        }
        pv_results.line.push_back(pv_move);
    }

    // The best move for a PV result is the first move of the line.
    // To not be confused with the SearchWorker's best move, as this
    // would be the best move of the entire search.
    if (!pv_results.line.empty()) {
        pv_results.best_move = pv_results.line[0];
    }

    // Ponder move is the move we would consider pondering about if we could.
    // We assume the best response from our opponent to be that move.
    if (pv_results.line.size() >= 2) {
        m_results.pv_results[m_curr_pv_idx].ponder_move = pv_results.line[1];
    }

    // Notify the time manager that we finished a pv iteration.
    if (m_main) {
        m_context->time_manager().on_new_pv(pv_results.depth,
                                            pv_results.best_move,
                                            pv_results.score);
    }

    // Notify whoever else needs to know about it.
    m_context->listeners().pv_finish(pv_results);
}

void SearchWorker::check_limits() {
    if (!m_main) {
        return;
    }

    if (m_results.nodes >= m_settings->max_nodes) {
        m_context->stop_search();
        return;
    }

    if (m_results.nodes % 1024 != 0) {
        return;
    }

    if (m_context->time_manager().finished_hard()) {
        m_context->stop_search();
    }
}

Score SearchWorker::draw_score() const {
    return m_board.color_to_move() == m_context->root_info().color
           ? -m_settings->contempt
           :  m_settings->contempt;
}

void SearchWorker::make_move(Move move) {
    m_eval.on_make_move(m_board, move);
    m_board.make_move(move);
    m_results.nodes++;
}

void SearchWorker::undo_move() {
    m_eval.on_undo_move(m_board, m_board.last_move());
    m_board.undo_move();
}

void SearchWorker::make_null_move() {
    m_eval.on_make_null_move(m_board);
    m_board.make_null_move();
}

void SearchWorker::undo_null_move() {
    m_eval.on_undo_null_move(m_board);
    m_board.undo_null_move();
}

bool SearchWorker::should_stop() const {
    return m_context->should_stop();
}

SearchWorker::SearchWorker(bool main,
                           Board board,
                           SearchContext* context,
                           const SearchSettings* settings)
    : m_main(main), m_board(std::move(board)),
      m_context(context), m_settings(settings) {
    m_eval.on_new_board(m_board);
}

const WorkerResults& SearchWorker::results() const {
    return m_results;
}

static void init_search_constants() {
    for (size_t m = 0; m < MAX_GENERATED_MOVES; ++m) {
        for (Depth d = 0; d < MAX_DEPTH; ++d) {
            s_lmr_table[m][d] = Depth(1.25 + std::log(d) * std::log(m) * 100.0 / 267.0);
        }
    }
    for (Depth d = 0; d < MAX_DEPTH; ++d) {
        s_lmp_count_table[false][d] = int(3 + 0.6 * d * d);
        s_lmp_count_table[true][d]  = int(4 + 1.2 * d * d);
    }
}

void init_search() {
    init_search_constants();
}

}