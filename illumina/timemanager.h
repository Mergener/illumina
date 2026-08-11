#ifndef ILLUMINA_TIMEMANAGER_H
#define ILLUMINA_TIMEMANAGER_H

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

private:
    TimePoint m_time_start = now();
    i64 m_soft_bound = 0;
    i64 m_hard_bound = 0;
    bool m_infinite = false;

    void calculate_bounds(Color us, const SearchLimits& limits);
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
