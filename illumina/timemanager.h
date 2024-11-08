#ifndef ILLUMINA_TIMEMANAGER_H
#define ILLUMINA_TIMEMANAGER_H

#include "clock.h"
#include "searchdefs.h"
#include "tunablevalues.h"
#include "types.h"

namespace illumina {

static constexpr int LAG_MARGIN = 10;

class PVResults;

class TimeManager {
public:
    void start_movetime(ui64 movetime_ms);
    void start_tourney_time(ui64 our_time_ms,
                            ui64 our_inc_ms,
                            ui64 their_time_ms,
                            ui64 their_inc_ms,
                            int moves_to_go = 0);
    ui64 hard_bound() const;
    ui64 soft_bound() const;
    bool finished_soft() const;
    bool finished_hard() const;
    bool running() const;
    void stop();

    void on_new_pv(const PVResults& results);

    TimeManager();

private:
    TimePoint m_time_start {};
    ui64 m_soft_bound = 0;
    ui64 m_hard_bound = 0;
    bool m_running = false;
    bool m_tourney_time = false;

    ui64 m_last_total_nodes {};

    void force_timeout();

    void setup(bool tourney_time);
    void set_starting_bounds(ui64 soft, ui64 hard);
};

inline bool TimeManager::running() const {
    return m_running;
}

inline ui64 TimeManager::soft_bound() const {
    return m_soft_bound;
}

inline ui64 TimeManager::hard_bound() const {
    return m_hard_bound;
}

inline bool TimeManager::finished_soft() const {
    return m_running && delta_ms(now(), m_time_start) >= m_soft_bound;
}

inline bool TimeManager::finished_hard() const {
    return m_running && delta_ms(now(), m_time_start) >= m_hard_bound;
}

inline TimeManager::TimeManager() {
    m_time_start = now();
}

} // illumina

#endif // ILLUMINA_TIMEMANAGER_H
