#ifndef ILLUMINA_SEARCH_H
#define ILLUMINA_SEARCH_H

#include <functional>
#include <optional>
#include <memory>
#include <nlohmann/json/json.hpp>

#include "board.h"
#include "timemanager.h"
#include "types.h"
#include "transpositiontable.h"

namespace illumina {

struct PVResults {
    Depth depth;
    int pv_idx;
    Depth sel_depth;
    Move  best_move;
    Score score;
    ui64  nodes;
    ui64  time;
    BoundType bound_type;
    std::vector<Move> line;
};

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
};

struct SearchStats {
    ui64 main_search_nodes {};
    ui64 qsearch_nodes {};
    ui64 all_nodes {};
    ui64 cut_nodes {};
    ui64 pv_nodes {};
    ui64 qsearch_best_move_idx_summation {};
    ui64 null_move_prunes {};
    ui64 fp_prunes {};
    ui64 lmp_prunes {};
    ui64 see_prunes {};
    ui64 qsee_prunes {};
    ui64 rfp_prunes {};
    ui64 mdp_prunes {};
    std::map<int, ui64> best_move_indexes;

    struct StageData {
        ui64 reached;
        ui64 best_move;
    };
    std::map<int, StageData> stages_data;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SearchStats::StageData, reached, best_move);

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SearchStats,
                                   main_search_nodes,
                                   qsearch_nodes,
                                   all_nodes,
                                   cut_nodes,
                                   pv_nodes,
                                   qsearch_best_move_idx_summation,
                                   null_move_prunes,
                                   fp_prunes,
                                   lmp_prunes,
                                   see_prunes,
                                   qsee_prunes,
                                   rfp_prunes,
                                   mdp_prunes,
                                   best_move_indexes,
                                   stages_data);
struct SearchResults {
    Move  best_move;
    Move  ponder_move;
    Score score;

    // Contains stats from all threads. First index is main thread.
    std::vector<SearchStats> search_stats;
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
    Searcher(Searcher&& other) = default;
    explicit Searcher(TranspositionTable&& tt);
    ~Searcher() = default;

private:
    bool m_searching = false;

    bool m_stop = false;
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
