#include "search.h"

#include <atomic>
#include <limits.h>
#include <cmath>
#include <thread>
#include <sstream>
#include <utility>

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
    Move  skip_move = MOVE_NULL;
    Move  pv[MAX_DEPTH];
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
    const std::vector<std::unique_ptr<SearchWorker>>& helper_workers() const;

    /**
     * Time, in milliseconds, since this worker started searching.
     * Expensive method, don't call often.
     */
    ui64 elapsed() const;

    void stop_search() const;
    bool should_stop() const;

    SearchContext(TranspositionTable* tt,
                  std::atomic<bool>* should_stop,
                  const Searcher::Listeners* listeners,
                  const RootInfo* root_info,
                  const std::vector<std::unique_ptr<SearchWorker>>* helper_workers,
                  TimeManager* time_manager);

private:
    TranspositionTable*        m_tt;
    const Searcher::Listeners* m_listeners;
    const RootInfo*            m_root_info;
    std::atomic<bool>*         m_stop;
    TimeManager*               m_time_manager;
    TimePoint                  m_search_start;
    const std::vector<std::unique_ptr<SearchWorker>>* m_helper_workers;
};

TranspositionTable& SearchContext::tt() const {
    return *m_tt;
}

bool SearchContext::should_stop() const {
    return m_stop->load(std::memory_order_relaxed);
}

ui64 SearchContext::elapsed() const {
    return delta_ms(now(), m_search_start);
}

const Searcher::Listeners& SearchContext::listeners() const {
    return *m_listeners;
}

SearchContext::SearchContext(TranspositionTable* tt,
                             std::atomic<bool>* should_stop,
                             const Searcher::Listeners* listeners,
                             const RootInfo* root_info,
                             const std::vector<std::unique_ptr<SearchWorker>>* helper_workers,
                             TimeManager* time_manager)
        : m_tt(tt),
          m_listeners(listeners),
          m_root_info(root_info),
          m_stop(should_stop),
          m_time_manager(time_manager),
          m_search_start(now()),
          m_helper_workers(helper_workers)
{ }

void SearchContext::stop_search() const {
    m_stop->store(true, std::memory_order_seq_cst);
}

TimeManager& SearchContext::time_manager() const {
    return *m_time_manager;
}

const RootInfo& SearchContext::root_info() const {
    return *m_root_info;
}

const std::vector<std::unique_ptr<SearchWorker>>& SearchContext::helper_workers() const {
    return *m_helper_workers;
}

/** Whether we are at the root node or not. */
enum RootMode {
    NON_ROOT,
    ROOT,
};

/** Whether we're doing Principal Variation Search or a Zero-Window search. */
enum SearchType {
    PVS,
    ZWS
};

/** Whether a search should be traced or not. */
enum TraceMode {
    UNTRACED,
    TRACED,
};

/** Whether we should skip Null Move Pruning or not. */
enum SkipNmpMode {
    DONT_SKIP_NMP,
    SKIP_NMP
};

class SearchWorker {
public:
    void iterative_deepening();
    bool should_stop() const;

    ui64  nodes() const;
    Score score() const;
    Move  best_move() const;
    Move  ponder_move() const;

    SearchWorker(bool main,
                 const Board& board,
                 SearchContext* context,
                 const SearchSettings* settings);

private:
    const SearchSettings* m_settings;
    const SearchContext*  m_context;
    const bool m_main;
    int m_eval_random_margin = 0;
    int m_eval_random_seed = 0;
    std::vector<Move> m_search_moves;

    Board       m_board;
    MoveHistory m_hist;
    Evaluation  m_eval {};
    Depth       m_root_depth = 1;
    Move        m_curr_move  = MOVE_NULL;
    int         m_curr_move_number = 0;
    int         m_curr_pv_idx = 0;
    Depth       m_sel_depth = 0;

    Score m_score = 0;
    ui64  m_nodes = 0;
    Move  m_best_move = MOVE_NULL;
    Move  m_ponder_move = MOVE_NULL;

    template <TraceMode TRACE_MODE,
            SearchType SEARCH_TYPE,
            SkipNmpMode SKIP_NULL_MODE = DONT_SKIP_NMP,
            RootMode ROOT_MODE = NON_ROOT>
    Score negamax(Depth depth,
                  Score alpha,
                  Score beta,
                  SearchNode* stack_node);

    template <TraceMode TRACE_MODE, SearchType SEARCH_TYPE>
    Score quiescence_search(Depth ply, Score alpha, Score beta);

    void aspiration_windows();

    void update_pv_results(const SearchNode* search_stack,
                           Score alpha, Score beta,
                           bool notify_tm);

    Score evaluate();
    Score draw_score() const;

    template <bool TRACE>
    void on_make_move(const Board& board, Move move);

    template <bool TRACE>
    void on_undo_move(const Board& board, Move move);

    template <bool TRACE>
    void on_make_null_move(const Board& board);

    template <bool TRACE>
    void on_undo_null_move(const Board& board);

    template <bool TRACE>
    void on_piece_added(const Board& board, Piece p, Square s);

    template <bool TRACE>
    void on_piece_removed(const Board& board, Piece p, Square s);

    void check_limits();
    bool tracing() const;
};

void Searcher::stop() {
    m_stop.store(true, std::memory_order_relaxed);
}

TranspositionTable& Searcher::tt() {
    return m_tt;
}

// Some tracing related macros to prevent cumbersome 'if constexprs'

#define TRACE_PUSH(move)                  \
do {                                      \
    if constexpr (TRACING) {           \
        auto tracer = m_settings->tracer; \
        tracer->push_node();              \
    }                                     \
} while(false)

#define TRACE_PUSH_SIBLING()              \
do {                                      \
    if constexpr (TRACING) {           \
        auto tracer = m_settings->tracer; \
        tracer->push_sibling_node();      \
    }                                     \
} while(false)

#define TRACE_POP()                       \
do {                                      \
    if constexpr (TRACING) {           \
        auto tracer = m_settings->tracer; \
        tracer->pop_node();               \
    }                                     \
} while(false)

#define TRACE_SET(which, val)             \
do {                                      \
    if constexpr (TRACING) {                \
        auto tracer = m_settings->tracer; \
        tracer->set(which, val);          \
    }                                     \
} while (false)

SearchResults Searcher::search(const Board& board,
                               const SearchSettings& settings) {
    // Create root info data.
    RootInfo root_info;
    root_info.color = board.color_to_move();

    // We start by generating the legal moves for two reasons:
    //  1. Determine which moves must be searched. This is the intersection
    //  between legal moves and requested searchmoves.
    //  2. Determine an initial (pseudo) best move, so that we can play it even
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
        results.total_nodes = 1;
        return results;
    }
    results.best_move = root_info.moves[0];

    // Create search context.
    std::vector<std::unique_ptr<SearchWorker>> secondary_workers;
    m_stop.store(false, std::memory_order::memory_order_seq_cst);
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

    // Fire secondary threads.
    secondary_workers.clear();
    std::vector<std::thread> helper_threads;
    secondary_workers.resize(n_helper_threads);
    for (int i = 0; i < n_helper_threads; ++i) {
        helper_threads.emplace_back([&secondary_workers, &board, &context, &settings, i]() {
            auto worker = std::make_unique<SearchWorker>(false, board, &context, &settings);
            worker->iterative_deepening();
            secondary_workers[i] = std::move(worker);
        });
    }

    // Initialize tracer search.
    if (settings.tracer != nullptr) {
        settings.tracer->new_search(board, tt().size() / (1024 * 1024), settings);
    }

    main_worker.iterative_deepening();

    // Force search to be stopped.
    context.stop_search();

    // Finish tracing search.
    if (settings.tracer != nullptr) {
        settings.tracer->finish_search();
    }

    // Wait for secondary threads to stop.
    for (std::thread& thread: helper_threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    helper_threads.clear();

    // Fill in the search results object to be returned.
    // We assume that the best move is the one at MultiPV 1.
    // Although this is not necessarily true, we don't want
    // to focus our efforts in improving MultiPV mode since
    // it is already not the one used for playing.
    results.score = main_worker.score();
    results.total_nodes = main_worker.nodes();

    // Only accept non-null best moves.
    Move best_move = main_worker.best_move();
    if (best_move != MOVE_NULL) {
        results.best_move = best_move;
    }
    // Only accept non-null ponder moves.
    Move ponder_move = main_worker.ponder_move();
    if (ponder_move != MOVE_NULL) {
        results.ponder_move = ponder_move;
    }

    return results;
}

void SearchWorker::iterative_deepening() {
    Depth max_depth = m_settings->max_depth.value_or(MAX_DEPTH);
    for (m_root_depth = 1; m_root_depth <= max_depth; ++m_root_depth) {
        // If we finished soft, we don't want to start a new iteration.
        if (   m_main
               && m_root_depth > 2
               && m_context->time_manager().finished_soft()) {
            m_context->stop_search();
        }

        // Check if we need to interrupt the search.
        if (should_stop()) {
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
            if (m_best_move == MOVE_NULL) {
                m_best_move = m_search_moves[0];
            }

            check_limits();
            aspiration_windows();
            check_limits();

            // If we finished soft, we don't want to start a new iteration.
            if (   m_main
                   && m_root_depth > 2
                   && m_context->time_manager().finished_soft()) {
                m_context->stop_search();
            }

            // Check if we need to interrupt the search.
            if (should_stop()) {
                break;
            }

            // When using MultiPV, erase the best searched move and look for a new
            // pv without it.
            Move pv_move = best_move();
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
    for (Depth ply = 0; ply < Depth(STACK_SIZE); ++ply) {
        SearchNode& node = search_stack[ply];
        node.ply = ply;
    }

    // Prepare aspiration windows.
    Score prev_score = score();
    Score alpha      = -MAX_SCORE;
    Score beta       = MAX_SCORE;
    Score window     = ASP_WIN_WINDOW;
    Depth depth      = m_root_depth;

    // Don't use aspiration windows in lower depths since
    // their results is still too unstable.
    if (depth >= ASP_WIN_MIN_DEPTH) {
        alpha = std::max(-MAX_SCORE, prev_score - window);
        beta  = std::min(MAX_SCORE,  prev_score + window);
    }

    Move best_move = m_best_move;

    // Perform search with aspiration windows.
    while (!should_stop()) {
        Score score;
        if (tracing()) {
            ISearchTracer* tracer = m_settings->tracer;
            tracer->new_tree(m_root_depth,
                             m_curr_pv_idx + 1,
                             alpha, beta);
            score = negamax<TRACED, PVS, SKIP_NMP, ROOT>(depth, alpha, beta, &search_stack[0]);
            tracer->set(Traceable::SCORE, score);
            tracer->finish_tree();
        }
        else {
            score = negamax<UNTRACED, PVS, SKIP_NMP, ROOT>(depth, alpha, beta, &search_stack[0]);
        }

        // Update ponder move if and only if we have both
        // a best move and a ponder move.
        if (   search_stack->pv[0] != MOVE_NULL
               && search_stack->pv[1] != MOVE_NULL) {
            m_ponder_move = search_stack->pv[1];
        }

        if (score > alpha && score < beta) {
            // We found an exact score within our bounds, finish
            // the current depth search.
            update_pv_results(search_stack, alpha, beta, true);
            break;
        }

        if (score <= alpha) {
            beta  = (alpha + beta) / 2;
            alpha = std::max(-MAX_SCORE, alpha - window);
            depth = m_root_depth;

            m_score = prev_score;
            m_best_move = best_move;
            update_pv_results(search_stack, alpha, beta, false);
        }
        else if (score >= beta) {
            beta = std::min(MAX_SCORE, beta + window);

            prev_score = score;
            best_move  = m_best_move;
            update_pv_results(search_stack, alpha, beta, true);
        }

        check_limits();

        window += window / 2;
    }
}

static std::array<std::array<Depth, MAX_DEPTH>, MAX_GENERATED_MOVES> s_lmr_table;
static std::array<std::array<int, MAX_DEPTH>, 2> s_lmp_count_table;

template<TraceMode TRACE_MODE,
        SearchType SEARCH_TYPE,
        SkipNmpMode SKIP_NMP_MODE,
        RootMode ROOT_MODE>
Score SearchWorker::negamax(Depth depth, Score alpha, Score beta, SearchNode* stack_node) {
    constexpr bool PV_NODE      = SEARCH_TYPE   == PVS;
    constexpr bool ROOT_NODE    = ROOT_MODE     == ROOT;
    constexpr bool TRACING      = TRACE_MODE    == TRACED;
    constexpr bool SKIPPING_NMP = SKIP_NMP_MODE == SKIP_NMP;

    ILLUMINA_ASSERT(!TRACING || tracing());

    TRACE_SET(Traceable::PV, PV_NODE);
    TRACE_SET(Traceable::ALPHA, alpha);
    TRACE_SET(Traceable::BETA, beta);
    TRACE_SET(Traceable::LAST_MOVE, m_board.last_move());
    TRACE_SET(Traceable::LAST_MOVE_RAW, m_board.last_move().raw());
    TRACE_SET(Traceable::ZOB_KEY, i64(m_board.hash_key()));
    TRACE_SET(Traceable::DEPTH, depth);
    TRACE_SET(Traceable::IN_CHECK, m_board.in_check());

    // Initialize the PV line with a null move. Specially useful for all-nodes.
    if constexpr (PV_NODE) {
        stack_node->pv[0] = MOVE_NULL;
    }

    // Don't search nodes with closed bounds.
    if (!ROOT_NODE && alpha >= beta) {
        return alpha;
    }

    // Keep track on some stats to report or use later.
    m_sel_depth = std::max(stack_node->ply, m_sel_depth);

    // Make sure we count the root node.
    if constexpr (ROOT_NODE) {
        m_nodes++;
    }

    // Check if we must stop our search.
    // If so, return the best score we've found so far.
    check_limits();
    if (should_stop()) {
        return alpha;
    }

    // Check for draws and return the draw score, taking contempt into consideration.
    if (!ROOT_NODE && (m_board.is_repetition_draw(2) || (m_board.rule50() >= 100) || m_board.is_insufficient_material_draw())) {
        return draw_score();
    }

    // Setup some important values.
    TranspositionTable& tt = m_context->tt();
    ui64 board_key         = m_board.hash_key();
    Score original_alpha   = alpha;
    int n_searched_moves   = 0;
    Move hash_move         = MOVE_NULL;
    bool in_check          = m_board.in_check();
    Color us               = m_board.color_to_move();
    Depth ply              = stack_node->ply;
    Score& static_eval     = stack_node->static_eval;

    // Probe from transposition table. This will allow us
    // to use information gathered in other searches (or transpositions)
    // to improve the current search.
    TranspositionTableEntry tt_entry {};
    bool found_in_tt = tt.probe(board_key, tt_entry, stack_node->ply);

    // On multithreaded searches, race conditions might make it so
    // that the TT move is invalid. The following check loses elo
    // on single threaded searches (~-2), so we only do it if we're
    // running SMP.
    if (   found_in_tt
           && m_settings->n_threads > 1
           && tt_entry.move() != MOVE_NULL
           && (   !m_board.is_move_pseudo_legal(tt_entry.move())
                  || !m_board.is_move_legal(tt_entry.move()))) {
        found_in_tt = false;
    }

    if (found_in_tt) {
        hash_move = tt_entry.move();

        TRACE_SET(Traceable::FOUND_IN_TT, true);
        TRACE_SET(Traceable::TT_MOVE, hash_move);
        TRACE_SET(Traceable::TT_BOUND, tt_entry.bound_type());
        TRACE_SET(Traceable::TT_DEPTH, tt_entry.depth());
        TRACE_SET(Traceable::TT_SCORE, tt_entry.score());

        // TT Cutoff.
        if (!PV_NODE
            && stack_node->skip_move == MOVE_NULL
            && tt_entry.depth() >= depth) {
            BoundType bt = tt_entry.bound_type();
            Score score  = tt_entry.score();
            if (   bt == BT_EXACT
                   || (bt == BT_UPPERBOUND && score <= alpha)
                   || (bt == BT_LOWERBOUND && score >= beta)) {
                TRACE_SET(Traceable::TT_CUTOFF, true);
                return score;
            }
        }
    }

    bool ttpv = PV_NODE || (found_in_tt && tt_entry.ttpv());

    // Check extensions.
    // Extend positions in check.
    depth += in_check;

    // Clamp excessive depths.
    depth = std::min(std::min(MAX_DEPTH, depth), m_root_depth * 2 - ply);

    // Compute the static eval. Useful for many heuristics.
    Score raw_eval;
    if (!in_check) {
        raw_eval    = !found_in_tt ? evaluate() : tt_entry.static_eval();
        static_eval = m_hist.correct_eval_with_corrhist(m_board, raw_eval);
        TRACE_SET(Traceable::PAWN_CORRHIST, m_hist.pawn_corrhist(m_board) / CORRHIST_GRAIN);
        TRACE_SET(Traceable::NON_PAWN_CORRHIST, m_hist.non_pawn_corrhist(m_board) / CORRHIST_GRAIN);
    }
    else {
        raw_eval    = 0;
        static_eval = 0;
    }
    TRACE_SET(Traceable::STATIC_EVAL, static_eval);

    bool improving = ply > 2 && !in_check && ((stack_node - 2)->static_eval < static_eval);
    TRACE_SET(Traceable::IMPROVING, improving);

    // Internal iterative reductions.
    if (   depth >= IIR_MIN_DEPTH
           && !found_in_tt
           && stack_node->skip_move == MOVE_NULL) {
        depth -= IIR_DEPTH_RED;
    }

    // Reverse futility pruning.
    // If our position is too good, by a safe margin and low depth, prune.
    Score rfp_margin = RFP_MARGIN_BASE + RFP_DEPTH_MULT * depth;
    if (!PV_NODE
        && !in_check
        && stack_node->skip_move == MOVE_NULL
        && depth <= RFP_MAX_DEPTH
        && alpha < MATE_THRESHOLD
        && static_eval - rfp_margin > beta) {
        return static_eval - rfp_margin;
    }

    // Mate distance pruning.
    Score expected_mate_score = MATE_SCORE - ply;
    if (expected_mate_score < beta) {
        beta = expected_mate_score;
        if (alpha >= beta) {
            return beta;
        }
    }

    // Null move pruning.
    if (   !PV_NODE
           && !SKIPPING_NMP
           && !in_check
           && non_pawn_bb(m_board) != 0
           && popcount(m_board.color_bb(us)) >= NMP_MIN_PIECES
           && static_eval >= beta
           && depth >= NMP_MIN_DEPTH
           && stack_node->skip_move == MOVE_NULL) {
        Depth reduction = depth / 3 + 4;

        m_board.make_null_move();
        Score score = -negamax<TRACE_MODE, ZWS, SKIP_NMP>(depth - reduction, -beta, -beta + 1, stack_node + 1);
        TRACE_SET(Traceable::SCORE, -score);
        m_board.undo_null_move();

        if (score >= beta) {
            tt.try_store(board_key, ply, MOVE_NULL, score, depth, static_eval, BT_LOWERBOUND, ttpv);
            return score;
        }
    }

    // ProbCut.
    Score pc_beta = beta + PROBCUT_BETA_MARGIN;
    if (   depth >= PROBCUT_DEPTH
        && (!found_in_tt || tt_entry.depth() < (depth - 3) || tt_entry.score() >= pc_beta)
        && !in_check
        && stack_node->skip_move == MOVE_NULL
        && std::abs(beta) < KNOWN_WIN) {
        // TT moves may be used if we're pretty sure they can cause a cutoff.
        // We approximate a pawn value to 200 internal units, hence the division by 200.
        Move pc_tt_move = MOVE_NULL;
        if (   found_in_tt
            && tt_entry.move() != MOVE_NULL
            && has_good_see(m_board, hash_move.source(), hash_move.destination(), (pc_beta - static_eval) / 200)) {
            pc_tt_move = hash_move;
        }

        MovePicker<false> pc_move_picker(m_board, ply, m_hist, pc_tt_move);
        Score pc_score = -MAX_SCORE;

        int pc_searched_moves = 0;
        SearchMove move;
        while ((move = pc_move_picker.next()) != MOVE_NULL) {
            if (   pc_searched_moves > 0
                && pc_move_picker.stage() > MPS_GOOD_CAPTURES) {
                break;
            }

            m_board.make_move(move);
            pc_score = -quiescence_search<TRACE_MODE, SEARCH_TYPE>(ply + 1, -pc_beta, -pc_beta + 1);
            if (pc_score >= pc_beta) {
                pc_score = -negamax<TRACE_MODE, ZWS>(PROBCUT_DEPTH, -pc_beta, -pc_beta + 1, stack_node + 1);
            }
            m_board.undo_move();
            if (pc_score >= pc_beta) {
                break;
            }
            pc_searched_moves++;
        }

        if (pc_score >= pc_beta) {
            return pc_score;
        }
    }

    // Dive into the quiescence search when depth becomes zero.
    if (depth <= 0) {
        Score score = quiescence_search<TRACE_MODE, SEARCH_TYPE>(ply, alpha, beta);
        TRACE_SET(Traceable::SCORE, score);
        return score;
    }

    // Kickstart our curr move counter for later reporting.
    if constexpr (ROOT_NODE) {
        m_curr_move_number = 0;
    }

    // Store played quiet moves in this list.
    // Useful for history updates later on.
    StaticList<Move, MAX_GENERATED_MOVES> quiets_played;

    int move_idx = -1;

    MovePicker move_picker(m_board, ply, m_hist, hash_move);
    SearchMove move {};
    Move best_move = found_in_tt ? tt_entry.move() : MOVE_NULL;
    bool has_legal_moves = false;
    Score best_score = -MAX_SCORE;
    while ((move = move_picker.next()) != MOVE_NULL) {
        has_legal_moves = true;
        if (move == stack_node->skip_move) {
            continue;
        }

        move_idx++;

        // Skip unrequested moves.
        if (ROOT_NODE
            && std::find(m_search_moves.begin(), m_search_moves.end(), move) == m_search_moves.end()) {
            continue;
        }

        // Report 'currmove' and 'currmovenumber'.
        if constexpr (ROOT_NODE) {
            m_curr_move_number++;
            m_curr_move = move;

            if (m_main) {
                m_context->listeners().curr_move_listener(m_root_depth,
                                                          m_curr_move,
                                                          m_curr_move_number);
            }
        }

        // Low depth pruning.
        if (   non_pawn_bb(m_board)
               && alpha > -KNOWN_WIN) {
            // Late move pruning.
            if (!ROOT_NODE
                && alpha > -MATE_THRESHOLD
                && depth <= (LMP_BASE_MAX_DEPTH + m_board.gives_check(move))
                && move_idx >= s_lmp_count_table[improving][depth]
                && move_picker.stage() > MPS_KILLER_MOVES
                && !in_check) {
                move_picker.skip_quiets();
                if (move.is_quiet()) {
                    continue;
                }
            }

            Color them = opposite_color(m_board.color_to_move());
            Bitboard discovered_atks = discovered_attacks(m_board, move.source(), move.destination());
            Bitboard their_valuable_pieces = m_board.piece_bb(Piece(them, PT_KING))
                                             | m_board.piece_bb(Piece(them, PT_QUEEN))
                                             | m_board.piece_bb(Piece(them, PT_ROOK));

            // SEE pruning.
            if ((!PV_NODE || m_root_depth > SEE_PRUNING_MAX_DEPTH)
                && (discovered_atks & their_valuable_pieces) == 0
                && depth <= SEE_PRUNING_MAX_DEPTH
                && !m_board.in_check()
                && move_picker.stage() > MPS_GOOD_CAPTURES
                && !has_good_see(m_board, move.source(), move.destination(), SEE_PRUNING_THRESHOLD)) {
                continue;
            }

            // Futility pruning.
            if ((!PV_NODE || m_root_depth > FP_MAX_DEPTH)
                && depth <= FP_MAX_DEPTH
                && !in_check
                && move != hash_move
                && (static_eval + FP_MARGIN) < alpha
                && !m_board.gives_check(move)) {
                move_picker.skip_quiets();
                if (move.is_quiet()) {
                    continue;
                }
            }
        }

        // Singular extensions.
        Depth extensions = 0;
        if (!ROOT_NODE
            && !in_check
            && hash_move != MOVE_NULL
            && stack_node->skip_move == MOVE_NULL
            && tt_entry.bound_type() != BT_UPPERBOUND
            && depth >= 8
            && move == hash_move
            && tt_entry.depth() >= (depth - 3)
            && std::abs(tt_entry.score()) < MATE_THRESHOLD
            && !m_board.gives_check(move)) {
            Score se_beta = tt_entry.score() - depth * 3;

            stack_node->skip_move = move;

            TRACE_PUSH_SIBLING();
            TRACE_SET(Traceable::SKIP_MOVE, stack_node->skip_move);

            Score score = negamax<TRACE_MODE, ZWS>(depth / 2, se_beta - 1, se_beta, stack_node);
            TRACE_SET(Traceable::SCORE, score);

            TRACE_POP();

            stack_node->skip_move = MOVE_NULL;

            if (score < se_beta) {
                extensions++;
                if (  !PV_NODE
                      && score < (se_beta - SE_DOUBLE_EXT_MARGIN)) {
                    extensions++;
                }
            }
                // Multi-cut pruning.
            else if (score >= beta) {
                return score;
            }
        }

        m_board.make_move(move);
        TRACE_SET(Traceable::LAST_MOVE_SCORE, move.value());

        // Late move reductions.
        Depth reductions = 0;
        if (   n_searched_moves >= LMR_MIN_MOVE_IDX
               && depth >= LMR_MIN_DEPTH
               && !in_check
               && !m_board.in_check()) {
            reductions = s_lmr_table[n_searched_moves - 1][depth];
            if (move.is_quiet()) {
                // Further reduce moves that are not improving the static evaluation.
                reductions += !improving;

                // Further reduce moves that have been historically very bad.
                reductions += m_hist.quiet_history(move,
                                                   m_board.last_move(),
                                                   m_board.gives_check(move)) <= LMR_BAD_HISTORY_THRESHOLD;

                // Don't reduce nodes that have been on the PV as much.
                reductions -= ttpv;
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
            score = -negamax<TRACE_MODE, SEARCH_TYPE>(depth - 1 + extensions, -beta, -alpha, stack_node + 1);
            TRACE_SET(Traceable::SCORE, -score);
        }
        else {
            // Perform a null window search. Searches after the first move are
            // performed with a null window. If the search fails high, do a
            // re-search with the full window.
            score = -negamax<TRACE_MODE, ZWS>(depth - 1 - reductions + extensions, -alpha - 1, -alpha, stack_node + 1);
            TRACE_SET(Traceable::SCORE, -score);

            if (score > alpha && reductions > 1) {
                TRACE_PUSH_SIBLING();
                score = -negamax<TRACE_MODE, ZWS>(depth - 1 + extensions, -alpha - 1, -alpha, stack_node + 1);
                TRACE_SET(Traceable::SCORE, -score);
                TRACE_POP();
            }

            if (score > alpha && score < beta) {
                TRACE_PUSH_SIBLING();
                score = -negamax<TRACE_MODE, SEARCH_TYPE>(depth - 1 + extensions, -beta, -alpha, stack_node + 1);
                TRACE_SET(Traceable::SCORE, -score);
                TRACE_POP();
            }
        }

        m_board.undo_move();

        if (move.is_quiet()) {
            quiets_played.push_back(move);
        }

        n_searched_moves++;

        if (score >= best_score) {
            best_score = score;
        }

        if (score >= beta) {
            // Our search failed high.
            alpha     = score;
            best_move = move;
            TRACE_SET(Traceable::BEST_MOVE, best_move);
            TRACE_SET(Traceable::BEST_MOVE_RAW, best_move.raw());

            // Update our history scores and refutation moves.
            if (move.is_quiet()) {
                m_hist.set_killer(ply, move);

                for (Move quiet: quiets_played) {
                    m_hist.update_quiet_history(quiet,
                                                m_board.last_move(),
                                                depth,
                                                quiet == best_move);
                }
            }

            // Due to aspiration windows, our search may have failed high in the root.
            // Make sure we always have a best_move and a best_score.
            if (ROOT_NODE && (!should_stop() || depth <= 2)) {
                m_best_move = move;
                m_score = alpha;
            }

            if constexpr (PV_NODE && !ROOT_NODE) {
                stack_node->pv[0] = MOVE_NULL;
            }
            break;
        }
        if (score > alpha) {
            // We've got a new best move.
            alpha     = score;
            best_move = move;
            TRACE_SET(Traceable::BEST_MOVE, move);
            TRACE_SET(Traceable::BEST_MOVE_RAW, move.raw());

            // Make sure we update our best_move in the root.
            if (ROOT_NODE && (!should_stop() || depth <= 2)) {
                m_best_move = move;
                m_score = alpha;
            }

            // Update the PV table.
            if constexpr (PV_NODE) {
                stack_node->pv[0] = best_move;
                size_t i;
                for (i = 0; i < MAX_DEPTH - 2; ++i) {
                    Move pv_move = (stack_node + 1)->pv[i];
                    if (pv_move == MOVE_NULL) {
                        break;
                    }
                    stack_node->pv[i + 1] = pv_move;
                }
                stack_node->pv[i + 1] = MOVE_NULL;
            }
        }
    }

    // Check if we have a checkmate or stalemate.
    if (!has_legal_moves) {
        if (stack_node->skip_move != MOVE_NULL) {
            return alpha;
        }

        Score score = m_board.in_check() ? (-MATE_SCORE + ply) : 0;
        return score;
    }

    best_score = n_searched_moves > 0 ? best_score : alpha;

    // Store in transposition table.
    // Don't store in singular searches.
    if (stack_node->skip_move == MOVE_NULL) {
        if (alpha >= beta) {
            // Beta-Cutoff, lowerbound score.
            tt.try_store(board_key,
                         ply, best_move,
                         best_score,
                         depth, raw_eval,
                         BT_LOWERBOUND,
                         ttpv);

            // Update corrhist.
            if (   !in_check
                   && (best_move == MOVE_NULL || best_move.is_quiet())
                   && alpha >= static_eval) {
                m_hist.update_corrhist(m_board, depth, alpha - static_eval);
            }
        } else if (alpha <= original_alpha) {
            // Couldn't raise alpha, score is an upperbound.
            tt.try_store(board_key,
                         ply, best_move,
                         best_score,
                         depth, raw_eval,
                         BT_UPPERBOUND,
                         ttpv);

            // Update corrhist.
            if (   !in_check
                   && (best_move == MOVE_NULL || best_move.is_quiet())
                   && alpha <= static_eval) {
                m_hist.update_corrhist(m_board, depth, alpha - static_eval);
            }
        } else {
            // We have an exact score.
            tt.try_store(board_key,
                         ply, best_move,
                         best_score,
                         depth, raw_eval,
                         BT_EXACT,
                         ttpv);

            // Update corrhist.
            if (   !in_check
                   && (best_move == MOVE_NULL || best_move.is_quiet())
                   && alpha >= static_eval) {
                m_hist.update_corrhist(m_board, depth, alpha - static_eval);
            }
        }
    }

    return best_score;
}

template <TraceMode TRACE_MODE, SearchType SEARCH_TYPE>
Score SearchWorker::quiescence_search(Depth ply, Score alpha, Score beta) {
    constexpr bool PV_NODE = SEARCH_TYPE == PVS;
    constexpr bool TRACING = TRACE_MODE == TRACED;

    TRACE_SET(Traceable::QSEARCH, true);
    TRACE_SET(Traceable::PV, PV_NODE);
    TRACE_SET(Traceable::ALPHA, alpha);
    TRACE_SET(Traceable::BETA, beta);
    TRACE_SET(Traceable::LAST_MOVE, m_board.last_move());
    TRACE_SET(Traceable::LAST_MOVE_RAW, m_board.last_move().raw());
    TRACE_SET(Traceable::ZOB_KEY, i64(m_board.hash_key()));
    TRACE_SET(Traceable::DEPTH, 0);
    TRACE_SET(Traceable::IN_CHECK, m_board.in_check());

    Score original_alpha = alpha;

    TranspositionTable& tt = m_context->tt();
    TranspositionTableEntry tt_entry;
    Move tt_move = MOVE_NULL;
    bool found_in_tt = tt.probe(m_board.hash_key(), tt_entry);
    if (   found_in_tt
           && tt_entry.move() != MOVE_NULL) {
        // On multithreaded searches, race conditions might make it so
        // that the TT move is invalid. The following check loses elo
        // on single threaded searches (~-2), so we only do it if we're
        // running SMP.
        if (   m_settings->n_threads > 1
               && (   !m_board.is_move_pseudo_legal(tt_entry.move())
                      || !m_board.is_move_legal(tt_entry.move()))) {
            found_in_tt = false;
        }
        else {
            // We're in qsearch, never search non capture moves.
            tt_move = tt_entry.move().is_capture()
                      ? tt_entry.move()
                      : MOVE_NULL;
        }
    }

    if (   found_in_tt
           && tt_entry.move() != MOVE_NULL
           && (   !m_board.is_move_pseudo_legal(tt_entry.move())
                  || !m_board.is_move_legal(tt_entry.move()))) {
        found_in_tt = false;
    }
    else {
        tt_move = found_in_tt && tt_entry.move().is_capture()
                  ? tt_entry.move()
                  : MOVE_NULL;
    }

    m_sel_depth = std::max(m_sel_depth, ply);

    Score raw_eval = found_in_tt ? tt_entry.static_eval() : evaluate();
    Score stand_pat = raw_eval;
    if (!m_board.in_check()) {
        stand_pat = m_hist.correct_eval_with_corrhist(m_board, stand_pat);
    }
    TRACE_SET(Traceable::STATIC_EVAL, stand_pat);

    if (stand_pat >= beta) {
        return stand_pat;
    }
    if (stand_pat > alpha) {
        alpha = stand_pat;
    }

    check_limits();
    if (should_stop()) {
        return alpha;
    }

    // Finally, start looping over available noisy moves.
    MovePicker<true> move_picker(m_board, ply, m_hist, tt_move);
    SearchMove move;
    SearchMove best_move;
    Score best_score = stand_pat;
    while ((move = move_picker.next()) != MOVE_NULL) {
        // SEE pruning.
        if (   move_picker.stage() >= MPS_BAD_CAPTURES
               && !has_good_see(m_board, move.source(), move.destination(), QSEE_PRUNING_THRESHOLD)) {
            continue;
        }

        m_board.make_move(move);
        TRACE_SET(Traceable::LAST_MOVE_SCORE, move.value());
        Score score = -quiescence_search<TRACE_MODE, SEARCH_TYPE>(ply + 1, -beta, -alpha);
        TRACE_SET(Traceable::SCORE, -score);
        m_board.undo_move();

        if (score > best_score) {
            best_score = score;
        }

        if (score >= beta) {
            best_move = move;
            TRACE_SET(Traceable::BEST_MOVE, move);
            TRACE_SET(Traceable::BEST_MOVE_RAW, move.raw());
            alpha = score;
            break;
        }
        if (score > alpha) {
            best_move = move;
            TRACE_SET(Traceable::BEST_MOVE, move);
            TRACE_SET(Traceable::BEST_MOVE_RAW, move.raw());
            alpha = score;
        }
    }

    if (best_score <= original_alpha) {
        tt.try_store(m_board.hash_key(),
                     ply, MOVE_NULL,
                     best_score,
                     0, raw_eval,
                     BT_UPPERBOUND,
                     false);
    }
    else if (best_score >= beta) {
        tt.try_store(m_board.hash_key(),
                     ply, best_move,
                     best_score,
                     0, raw_eval,
                     BT_LOWERBOUND,
                     false);
    }
    else {
        tt.try_store(m_board.hash_key(),
                     ply, best_move,
                     best_score,
                     0, raw_eval,
                     BT_EXACT,
                     false);
    }

    return best_score;
}

Score SearchWorker::evaluate() {
    // Check if we're in a known endgame.
    Endgame eg = identify_endgame(m_board);

    // We're on a known endgame. Use its evaluation.
    if (eg.type != EG_UNKNOWN) {
        return eg.evaluation;
    }

    // If we're not in a known endgame, use our regular
    // static evaluation function.
    Score score = m_eval.compute();
    if (m_eval_random_margin != 0) {
        // User has requested evaluation randomness, apply the noise.
        i32 seed   = Score((m_eval_random_seed * m_board.hash_key()) & BITMASK(15));
        i32 margin = m_eval_random_margin;
        i32 noise  = (seed % (margin * 2)) - margin;
        score += Score(noise);
    }
    return score;
}

void SearchWorker::update_pv_results(const SearchNode* search_stack,
                                     Score alpha, Score beta,
                                     bool notify_tm) {
    // We only want the main thread to update results, the others
    // should just assist on the search.
    if (!m_main) {
        return;
    }

    Score score  = this->score();
    BoundType bt = score >= beta
                   ? BT_LOWERBOUND
                   : score <= alpha
                     ? BT_UPPERBOUND
                     : BT_EXACT;

    PVResults pv_results;
    pv_results.pv_idx     = m_curr_pv_idx;
    pv_results.depth      = m_root_depth;
    pv_results.sel_depth  = m_sel_depth;
    pv_results.score      = score;
    pv_results.time       = m_context->elapsed();
    pv_results.bound_type = bt;

    ui64 total_nodes = nodes();
    for (const std::unique_ptr<SearchWorker>& worker: m_context->helper_workers()) {
        if (worker == nullptr) {
            continue;
        }

        total_nodes += worker->nodes();
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

    // Notify the time manager that we finished a pv iteration.
    if (notify_tm) {
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

    ui64 nodes = this->nodes();
    if (nodes >= m_settings->max_nodes) {
        m_context->stop_search();
        return;
    }

    if (nodes % 1024 != 0) {
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

template <bool TRACING>
void SearchWorker::on_make_move(const illumina::Board& board, illumina::Move move) {
    TRACE_PUSH();
    m_nodes++;
    m_context->tt().prefetch(board.estimate_hash_key_after(move));
    m_eval.on_make_move(board, move);
}

template <bool TRACING>
void SearchWorker::on_undo_move(const illumina::Board& board, illumina::Move move) {
    TRACE_POP();
    m_eval.on_undo_move(board, move);
}

template <bool TRACING>
void SearchWorker::on_make_null_move(const illumina::Board& board) {
    TRACE_PUSH();
    m_nodes++;
    m_eval.on_make_null_move(board);

    TRACE_SET(Traceable::LAST_MOVE, MOVE_NULL);
    TRACE_SET(Traceable::LAST_MOVE_RAW, MOVE_NULL.raw());
}

template <bool TRACING>
void SearchWorker::on_undo_null_move(const illumina::Board& board) {
    TRACE_POP();
    m_eval.on_undo_null_move(board);
}

template <bool TRACING>
void SearchWorker::on_piece_added(const Board& board, Piece p, Square s) {
    m_eval.on_piece_added(board, p , s);
}

template <bool TRACING>
void SearchWorker::on_piece_removed(const Board& board, Piece p, Square s) {
    m_eval.on_piece_removed(board, p , s);
}

bool SearchWorker::should_stop() const {
    return m_context->should_stop();
}

Score SearchWorker::score() const {
    return m_score;
}

ui64 SearchWorker::nodes() const {
    return m_nodes;
}

Move SearchWorker::best_move() const {
    return m_best_move;
}

Move SearchWorker::ponder_move() const {
    return m_ponder_move;
}

SearchWorker::SearchWorker(bool main,
                           const Board& board,
                           SearchContext* context,
                           const SearchSettings* settings)
        : m_settings(settings),
          m_context(context),
          m_main(main),
          m_eval_random_margin(settings->eval_random_margin),
          m_eval_random_seed(settings->eval_rand_seed),
          m_board(board) {
    m_eval.on_new_board(m_board);

    // Dispatch board callbacks to Worker's methods.
    BoardListener board_listener {};
    if (!main || settings->tracer == nullptr) {
        board_listener.on_make_null_move = [this](const Board& b) { on_make_null_move<false>(b); };
        board_listener.on_undo_null_move = [this](const Board& b) { on_undo_null_move<false>(b); };
        board_listener.on_make_move = [this](const Board& b, Move m) { on_make_move<false>(b, m); };
        board_listener.on_undo_move = [this](const Board& b, Move m) { on_undo_move<false>(b, m); };
        board_listener.on_add_piece = [this](const Board& b, Piece p, Square s) { on_piece_added<false>(b, p, s); };
        board_listener.on_remove_piece = [this](const Board& b, Piece p, Square s) { on_piece_removed<false>(b, p, s); };
    }
    else {
        board_listener.on_make_null_move = [this](const Board& b) { on_make_null_move<true>(b); };
        board_listener.on_undo_null_move = [this](const Board& b) { on_undo_null_move<true>(b); };
        board_listener.on_make_move = [this](const Board& b, Move m) { on_make_move<true>(b, m); };
        board_listener.on_undo_move = [this](const Board& b, Move m) { on_undo_move<true>(b, m); };
        board_listener.on_add_piece = [this](const Board& b, Piece p, Square s) { on_piece_added<true>(b, p, s); };
        board_listener.on_remove_piece = [this](const Board& b, Piece p, Square s) { on_piece_removed<true>(b, p, s); };
    }
    m_board.set_listener(board_listener);
}

bool SearchWorker::tracing() const {
    return m_main && m_settings->tracer != nullptr;
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