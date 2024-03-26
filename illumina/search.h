#ifndef ILLUMINA_SEARCH_H
#define ILLUMINA_SEARCH_H

#include <functional>
#include <optional>
#include <memory>

#include "board.h"
#include "timemanager.h"
#include "types.h"
#include "transpositiontable.h"

namespace illumina {

struct PVResults {
    Depth depth;
    Move  best_move;
    Score score;
    ui64  nodes;
    ui64  time;
    std::vector<Move> line;
};

struct SearchSettings {
    std::optional<Depth> max_depth;
    std::optional<i64>   white_time;
    std::optional<i64>   white_inc;
    std::optional<i64>   black_time;
    std::optional<i64>   black_inc;
    std::optional<i64>   move_time;
    std::optional<std::vector<Move>> search_moves;
};

struct SearchResults {
    Move  best_move;
    Score score;
};

class Searcher {
    friend class SearchContext;
    friend class SearchWorker;

public:
    using PVFinishListener = std::function<void(PVResults&)>;

    SearchResults search(const Board& board,
                         const SearchSettings& settings);
    void stop();

    void set_pv_finish_listener(const PVFinishListener& listener);

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
        PVFinishListener pv_finish = [](PVResults&) {};
    } m_listeners;
};

inline void Searcher::set_pv_finish_listener(const PVFinishListener& listener) {
    m_listeners.pv_finish = listener;
}

inline Searcher::Searcher(TranspositionTable&& tt)
    : m_tt(std::move(tt)) { }

} // illumina

#endif // ILLUMINA_SEARCH_H
