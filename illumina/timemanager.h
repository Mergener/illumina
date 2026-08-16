#ifndef ILLUMINA_TIMEMANAGER_H
#define ILLUMINA_TIMEMANAGER_H

#include <memory>

#include "clock.h"
#include "searchdefs.h"
#include "types.h"

namespace illumina {

class TimeManager {
public:
    void start(Color us, const SearchLimits& limits);

    /** If true, search should not start a new iteration. */
    bool finished_soft() const;

    /** If true, search must be interrupted immediately. */
    bool finished_hard() const;
    i64 elapsed() const;

    void on_iteration_complete(Depth depth, Move best_move, ui64 nodes);
    void add_spent_effort(Move move, ui64 nodes);

private:
    TimePoint m_time_start = now();
    i64 m_soft_bound = 0;
    i64 m_hard_bound = 0;
    bool m_infinite = false;

    struct {
        std::optional<i64> move_time;
        std::optional<i64> our_time;
        i64 our_inc;
    } m_limits {};

    Move m_last_best_move = MOVE_NULL;
    int m_move_stability_count = 0;
    Depth m_curr_depth = 1;

    struct SpentEffortTable {
        std::array<std::array<ui64, SQ_COUNT>, SQ_COUNT> by_move {};
    };
    std::unique_ptr<SpentEffortTable> m_spent_effort = std::make_unique<SpentEffortTable>();
    ui64 m_nodes = 0;

    void calculate_bounds();
};

inline i64 TimeManager::elapsed() const {
    return delta_ms(now(), m_time_start);
}

inline bool TimeManager::finished_soft() const {
    return !m_infinite && elapsed() >= m_soft_bound;
}

inline bool TimeManager::finished_hard() const {
    return !m_infinite && elapsed() >= m_hard_bound;
}

} // illumina

#endif // ILLUMINA_TIMEMANAGER_H
