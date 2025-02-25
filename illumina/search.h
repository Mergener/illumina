#ifndef ILLUMINA_SEARCH_H
#define ILLUMINA_SEARCH_H

#include <functional>
#include <optional>
#include <memory>
#include <atomic>

#include "board.h"
#include "timemanager.h"
#include "tracing.h"
#include "types.h"
#include "transpositiontable.h"

namespace illumina {

struct SearchSettings {
    Score contempt = 0;
    int n_pvs = 1;
    int n_threads = 1;
    int eval_random_margin = 0;
    ui64 eval_rand_seed = 0;
    ui64 max_nodes = UINT64_MAX;
    std::optional<Depth> max_depth;
    std::optional<i64>   white_time;
    std::optional<i64>   white_inc;
    std::optional<i64>   black_time;
    std::optional<i64>   black_inc;
    std::optional<i64>   move_time;
    std::optional<std::vector<Move>> search_moves;
    ISearchTracer* tracer = nullptr;
};

struct PVResults {
    Depth depth;
    int pv_idx;
    Depth sel_depth;
    Move  best_move;
    Score score;
    ui64  nodes;
    ui64  best_move_nodes;
    ui64  time;
    BoundType bound_type;
    std::vector<Move> line;
};

struct SearchResults {
    Move  best_move = MOVE_NULL;
    Move  ponder_move = MOVE_NULL;
    Score score = 0;
    ui64 total_nodes = 0;
};

class Searcher {
    friend class SearchContext;
    friend class SearchWorker;

public:
    using PVFinishListener = std::function<void(PVResults&)>;
    using CurrentMoveListener = std::function<void(Depth depth, Move move, int move_num)>;

    TranspositionTable& tt();

    SearchResults search(const Board& board,
                         const SearchSettings& settings);
    void stop();

    void set_pv_finish_listener(const PVFinishListener& listener);
    void set_currmove_listener(const CurrentMoveListener& listener);

    Searcher() = default;
    Searcher(const Searcher& other) = delete;
    explicit Searcher(TranspositionTable&& tt);
    ~Searcher() = default;

private:
    bool m_searching = false;

    std::atomic_bool m_stop = false;
    TranspositionTable m_tt;

    TimeManager m_tm;

    struct Listeners {
        PVFinishListener    pv_finish = [](PVResults&) {};
        CurrentMoveListener curr_move_listener = [](Depth,Move,int) {};
    } m_listeners;
};

inline void Searcher::set_pv_finish_listener(const PVFinishListener& listener) {
    m_listeners.pv_finish = listener;
}

inline void Searcher::set_currmove_listener(const CurrentMoveListener& listener) {
    m_listeners.curr_move_listener = listener;
}

inline Searcher::Searcher(TranspositionTable&& tt)
    : m_tt(std::move(tt)) { }

void recompute_search_constants();

} // illumina

#endif // ILLUMINA_SEARCH_H
