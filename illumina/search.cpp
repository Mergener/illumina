#include "search.h"

#include <cmath>
#include <thread>

#include "endgame.h"
#include "timemanager.h"
#include "tunablevalues.h"
#include "movepicker.h"
#include "evaluation.h"
#include "utils.h"

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
    Move  pv[MAX_DEPTH];
    Move  skip_move = MOVE_NULL;
};

class SearchWorker;

/**
 * Search context shared among search workers.
 */
class SearchContext {
public:
    TimeManager&               time_manager() const;
    TranspositionTable&        tt() const;
    const Searcher::Listeners& listeners() const;
    const RootInfo&            root_info() const;
    const std::vector<SearchWorker>& helper_workers() const;

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
                  const std::vector<SearchWorker>* helper_workers,
                  TimeManager* time_manager);

private:
    TranspositionTable*        m_tt;
    const Searcher::Listeners* m_listeners;
    const RootInfo*            m_root_info;
    bool*                      m_stop;
    TimeManager*               m_time_manager;
    TimePoint                  m_search_start;
    const std::vector<SearchWorker>* m_helper_workers;
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
                             const std::vector<SearchWorker>* helper_workers,
                             TimeManager* time_manager)
    : m_stop(should_stop),
      m_tt(tt), m_listeners(listeners),
      m_time_manager(time_manager),
      m_root_info(root_info),
      m_helper_workers(helper_workers),
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

const std::vector<SearchWorker>& SearchContext::helper_workers() const {
    return *m_helper_workers;
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
    Depth searched_depth = 0;
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
    int         m_eval_random_margin = 0;
    int         m_eval_random_seed = 0;

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

    Score evaluate() const;
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

    // We start by generating the legal moves for two reasons:
    //  1. Determine which moves must be searched. This is the intersection
    //  between legal moves and requested searchmoves.
    //  2. Determine a initial (pseudo) best move, so that we can play it even
    //  without having searched anything.
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

    // If root moves are empty, we're either in a checkmate, stalemate or
    // requested searchmoves has no intersection with the position's legal
    // moves.
    // Don't even bother searching if that's the case.
    SearchResults results {};
    if (root_info.moves.empty()) {
        results.best_move   = MOVE_NULL;
        results.ponder_move = MOVE_NULL;
        results.score       = board.in_check() ? -MATE_SCORE : 0;
        return results;
    }
    results.best_move = root_info.moves[0];

    // Create search context.
    std::vector<SearchWorker> secondary_workers;
    m_stop = false;
    m_tt.new_search();
    SearchContext context(&m_tt, &m_stop, &m_listeners, &root_info, &secondary_workers, &m_tm);

    // Create main worker.
    SearchWorker main_worker(true, board, &context, &settings);

    // Kickstart our time manager.
    ui64 our_time = UINT64_MAX;
    if (settings.move_time.has_value()) {
        // 'movetime'
        our_time = settings.move_time.value();
        m_tm.start_movetime(our_time);
    }
    else if (settings.white_time.has_value() || settings.black_time.has_value()) {
        // 'wtime/winc/btime/binc'
        our_time = board.color_to_move() == CL_WHITE
                 ? settings.white_time.value_or(UINT64_MAX)
                 : settings.black_time.value_or(UINT64_MAX);

        m_tm.start_tourney_time(our_time, 0, 0, 0);
    }
    else {
        // 'infinite'
        m_tm.stop();
    }

    // Determine the number of helper threads to be used.
    int n_helper_threads = std::max(1, settings.n_threads) - 1;

    // Create secondary workers.
    for (int i = 0; i < n_helper_threads; ++i) {
        secondary_workers.emplace_back(false, board, &context, &settings);
    }

    // Fire secondary threads.
    std::vector<std::thread> helper_threads;
    for (int i = 0; i < n_helper_threads; ++i) {
        SearchWorker& worker = secondary_workers[i];
        helper_threads.emplace_back([&worker]() {
            worker.iterative_deepening();
        });
    }

    main_worker.iterative_deepening();

    // Wait for secondary threads to stop.
    for (std::thread& thread: helper_threads) {
        thread.join();
    }

    // Save obtained results in vector and vote for the best afterwards.
    std::vector<WorkerResults> all_results;
    all_results.push_back(main_worker.results());
    for (SearchWorker& worker: secondary_workers) {
        all_results.push_back(worker.results());
    }

    // Vote for the best results. Prioritize results
    // with higher scores that reached higher depths.
    WorkerResults* selected_results;
    int best_result_score = INT_MIN;
    for (WorkerResults& results: all_results) {
        if (results.pv_results[0].best_move == MOVE_NULL) {
            // Ignore threads that couldn't complete the search to a point
            // where they have a valid move.
            continue;
        }

        int result_score = results.searched_depth * 500 +
            results.pv_results[0].score +
            (results.pv_results[0].bound_type == BT_EXACT) * 400;

        if (result_score > best_result_score) {
            selected_results = &results;
            best_result_score = result_score;
        }
    }

    // Fill in the search results object to be returned.
    // We assume that the best move is the one at MultiPV 1.
    // Although this is not necessarily true, we don't want
    // to focus our efforts in improving MultiPV mode since
    // it is already not the one used for playing.
    const auto& main_line_results = selected_results->pv_results[0];
    results.score = main_line_results.score;

    // Only accept non-null best moves.
    if (main_line_results.best_move != MOVE_NULL) {
        results.best_move = main_line_results.best_move;
    }
    // Only accept non-null ponder moves.
    if (main_line_results.best_move != MOVE_NULL) {
        results.ponder_move = main_line_results.ponder_move;
    }

    return results;
}

void SearchWorker::iterative_deepening() {
    Depth max_depth = m_settings->max_depth.value_or(MAX_DEPTH);
    for (m_curr_depth = 1; m_curr_depth <= max_depth; ++m_curr_depth) {
        if (should_stop() || (m_curr_depth > 2 && m_context->time_manager().finished_soft())) {
            // If we finished soft, we don't want to start a new iteration.
            if (m_main) {
                m_context->stop_search();
            }
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
            m_results.searched_depth = m_curr_depth;
            check_limits();
            if (should_stop() || (m_curr_depth > 2 && m_context->time_manager().finished_soft())) {
                // If we finished soft, we don't want to start a new iteration.
                if (m_main) {
                    m_context->stop_search();
                }
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
    Score window     = ASP_WIN_WINDOW;
    Depth depth      = m_curr_depth;

    // Don't use aspiration windows in lower depths since
    // their results is still too unstable.
    if (depth >= ASP_WIN_MIN_DEPTH) {
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

    // Don't search nodes with closed bounds.
    if (!ROOT && alpha >= beta) {
        return beta;
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

    // Probe from transposition table. This will allow us
    // to use information gathered in other searches (or transpositions)
    // to improve the current search.
    TranspositionTableEntry tt_entry {};
    bool found_in_tt = tt.probe(board_key, tt_entry, node->ply);
    if (found_in_tt) {
        hash_move = tt_entry.move();

        if (!PV && node->skip_move == MOVE_NULL && tt_entry.depth() >= depth) {
            if (!ROOT && tt_entry.bound_type() == BT_EXACT) {
                // TT Cuttoff.
                return tt_entry.score();
            }
            else if (tt_entry.bound_type() == BT_LOWERBOUND) {
                alpha = std::max(alpha, tt_entry.score());
            }
            else {
                beta = std::min(beta, tt_entry.score());
            }

            if (alpha >= beta) {
                return alpha;
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
    static_eval    = !in_check ? evaluate() : 0;
    bool improving = ply > 2 && !in_check && ((node - 2)->static_eval < static_eval);

    // Reverse futility pruning.
    // If our position is too good, by a safe margin and low depth, prune.
    Score rfp_margin = RFP_MARGIN_BASE + RFP_DEPTH_MULT * depth;
    if (!PV        &&
        !in_check  &&
        node->skip_move == MOVE_NULL &&
        depth <= RFP_MAX_DEPTH &&
        alpha < MATE_THRESHOLD &&
        static_eval - rfp_margin > beta) {
        return static_eval - rfp_margin;
    }

    // Null move pruning.
    if (!PV        &&
        !SKIP_NULL &&
        !in_check  &&
        popcount(m_board.color_bb(us)) >= NMP_MIN_PIECES &&
        static_eval >= beta &&
        depth >= NMP_MIN_DEPTH &&
        node->skip_move == MOVE_NULL) {
        Depth reduction = std::max(depth, std::min(NMP_BASE_DEPTH_RED, NMP_BASE_DEPTH_RED + (static_eval - beta) / NMP_EVAL_DELTA_DIVISOR));

        make_null_move();
        Score score = -pvs<false, true>(depth - 1 - reduction, -beta, -beta + 1, node + 1);
        undo_null_move();

        if (score >= beta) {
            return beta;
        }
    }

    // Internal iterative reductions.
    if (depth >= IIR_MIN_DEPTH && !found_in_tt && node->skip_move == MOVE_NULL) {
        depth -= IIR_DEPTH_RED;
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
    if constexpr (ROOT) {
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
        if (move == node->skip_move) {
            continue;
        }

        move_idx++;

        // Skip unrequested moves.
        if (ROOT &&
            std::find(m_search_moves.begin(), m_search_moves.end(), move) == m_search_moves.end()) {
            continue;
        }

        // Report 'currmove' and 'currmovenumber'.
        if constexpr (ROOT) {
            m_curr_move_number++;
            m_curr_move = move;

            if (m_main) {
                m_context->listeners().curr_move_listener(m_curr_depth,
                                                          m_curr_move,
                                                          m_curr_move_number);
            }
        }

        // Late move pruning.
        if (alpha > -MATE_THRESHOLD &&
            depth <= (LMP_BASE_MAX_DEPTH + m_board.gives_check(move)) &&
            move_idx >= s_lmp_count_table[improving][depth] &&
            move_picker.stage() > MPS_KILLER_MOVES &&
            !in_check &&
            move.is_quiet()) {
            continue;
        }

        // SEE pruning.
        if (depth <= SEE_PRUNING_MAX_DEPTH &&
            !m_board.in_check()            &&
            move_picker.stage() > MPS_GOOD_CAPTURES &&
            !has_good_see(m_board, move.source(), move.destination(), SEE_PRUNING_THRESHOLD)) {
            continue;
        }

        // Futility pruning.
        if (depth <= FP_MAX_DEPTH      &&
            !in_check                  &&
            move.is_quiet()            &&
            (static_eval + FP_MARGIN) < alpha &&
            !m_board.gives_check(move)) {
            continue;
        }

        // Singular extensions.
        Depth extensions = 0;
        if (!ROOT     &&
            !in_check &&
            hash_move != MOVE_NULL       &&
            node->skip_move == MOVE_NULL &&
            tt_entry.bound_type() != BT_UPPERBOUND &&
            depth >= 8        &&
            move == hash_move &&
            tt_entry.depth() >= (depth - 3) &&
            std::abs(tt_entry.score()) < MATE_THRESHOLD) {
            Score se_beta = std::min(beta, tt_entry.score() - depth * 2);

            node->skip_move = move;
            Score score = pvs<false>(depth / 2, se_beta - 1, se_beta, node);
            node->skip_move = MOVE_NULL;

            if (score < se_beta) {
                extensions++;
            }
        }

        make_move(move);

        // Late move reductions.
        Depth reductions = 0;
        if (n_searched_moves >= LMR_MIN_MOVE_IDX &&
            depth >= LMR_MIN_DEPTH &&
            !in_check              &&
            !m_board.in_check()) {
            reductions = s_lmr_table[n_searched_moves - 1][depth];
            if (move.is_quiet()) {
                // Further reduce moves that are not improving the static evaluation.
                reductions += !improving;

                // Further reduce moves that have been historically very bad.
                reductions += m_hist.quiet_history(move,
                                                   m_board.last_move(),
                                                   m_board.gives_check(move)) <= LMR_BAD_HISTORY_THRESHOLD;
            }
            else if (move_picker.stage() == MPS_BAD_CAPTURES) {
                // Further reduce bad captures when we're in a very good position
                // and probably don't need unsound sacrifices.
                bool stable = alpha >= LMR_STABLE_ALPHA_THRESHOLD;
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
                    m_hist.update_quiet_history(quiet,
                                                m_board.last_move(),
                                                depth,
                                                m_board.gives_check(quiet),
                                                quiet == best_move);
                }
            }

            // Due to aspiration windows, our search may have failed high in the root.
            // Make sure we always have a best_move and a best_score.
            if (ROOT && (!should_stop() || depth <= 2)) {
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
            if (ROOT && (!should_stop() || depth <= 2)) {
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
    // Don't store in singular searches.
    if (node->skip_move == MOVE_NULL) {
        if (alpha >= beta) {
            // Beta-Cutoff, lowerbound score.
            tt.try_store(board_key, ply, best_move, alpha, depth, BT_LOWERBOUND);
        } else if (alpha <= original_alpha) {
            // Couldn't raise alpha, score is an upperbound.
            tt.try_store(board_key, ply, best_move, alpha, depth, BT_UPPERBOUND);
        } else {
            // We have an exact score.
            tt.try_store(board_key, ply, best_move, alpha, depth, BT_EXACT);
        }
    }

    return alpha;
}

Score SearchWorker::quiescence_search(Depth ply, Score alpha, Score beta) {
    Score stand_pat = evaluate();

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
            !has_good_see(m_board, move.source(), move.destination(), QSEE_PRUNING_THRESHOLD)) {
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

Score SearchWorker::evaluate() const {
    // Check if we're in a known endgame.
    Endgame eg = identify_endgame(m_board);

    // We're on a known endgame. Use its evaluation.
    if (eg.type != EG_UNKNOWN) {
        return eg.evaluation;
    }

    // If we're not in a known endgame, use our regular
    // static evaluation function.
    Score score = m_eval.get();
    if (m_eval_random_margin != 0) {
        // User has requested evaluation randomness, apply the noise.
        i32 seed   = Score((m_eval_random_seed * m_board.hash_key()) & BITMASK(15));
        i32 margin = m_eval_random_margin;
        i32 noise  = (seed % (margin * 2)) - margin;
        score += Score(noise);
    }
    return score;
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
    pv_results.time       = m_context->elapsed();
    pv_results.bound_type = m_results.pv_results[m_curr_pv_idx].bound_type;

    ui64 total_nodes = m_results.nodes;
    for (const SearchWorker& worker: m_context->helper_workers()) {
        total_nodes += worker.results().nodes;
    }
    pv_results.nodes = total_nodes;

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
    m_results.nodes++;
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
      m_context(context), m_settings(settings),
      m_eval_random_margin(m_main
                           ? settings->eval_random_margin
                           : std::max(SMP_EVAL_RANDOM_MARGIN, settings->eval_random_margin)),
      m_eval_random_seed(m_main
                         ? settings->eval_rand_seed
                         : random(ui64(1), UINT64_MAX)) {
    m_eval.on_new_board(m_board);
}

const WorkerResults& SearchWorker::results() const {
    return m_results;
}

static void init_search_constants() {
    for (size_t m = 0; m < MAX_GENERATED_MOVES; ++m) {
        for (Depth d = 0; d < MAX_DEPTH; ++d) {
            s_lmr_table[m][d] = Depth(LMR_REDUCTIONS_BASE + std::log(d) * std::log(m) * 100.0 / LMR_REDUCTIONS_DIVISOR);
        }
    }
    for (Depth d = 0; d < MAX_DEPTH; ++d) {
        s_lmp_count_table[false][d] = int(LMP_BASE_IDX_NON_IMPROVING + LMP_DEPTH_FACTOR_NON_IMPROVING * d * d);
        s_lmp_count_table[true][d]  = int(LMP_BASE_IDX_IMPROVING + LMP_DEPTH_FACTOR_IMPROVING * d * d);
    }
}

void recompute_search_constants() {
    init_search_constants();
}

void init_search() {
    init_search_constants();
}

}