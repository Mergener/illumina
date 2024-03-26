#ifndef ILLUMINA_TIMEMANAGER_H
#define ILLUMINA_TIMEMANAGER_H

#include "clock.h"
#include "types.h"

namespace illumina {

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
    ui64 elapsed() const;
    void stop();

    TimeManager();

private:
    TimePoint m_time_start {};
    ui64 m_soft_bound = 0;
    ui64 m_hard_bound = 0;
    ui64 m_elapsed = 0;
    bool m_running = false;

    void setup();
};

inline void TimeManager::setup() {
    m_time_start = now();
    m_running = true;
}

inline ui64 TimeManager::elapsed() const {
    return m_running ? delta_ms(now(), m_time_start) : m_elapsed;
}

inline void TimeManager::stop() {
    if (!m_running) {
        return;
    }
    m_elapsed = elapsed();
    m_running = false;
}

inline void TimeManager::start_movetime(ui64 movetime_ms) {
    setup();
    m_soft_bound = movetime_ms - 50;
    m_hard_bound = m_soft_bound;
}

inline void TimeManager::start_tourney_time(ui64 our_time_ms,
                                            ui64 our_inc_ms,
                                            ui64 their_time_ms,
                                            ui64 their_inc_ms,
                                            int moves_to_go) {
    setup();
    m_soft_bound = our_time_ms / 20;
    m_hard_bound = m_soft_bound;
}

inline ui64 TimeManager::soft_bound() const {
    return m_soft_bound;
}

inline ui64 TimeManager::hard_bound() const {
    return m_hard_bound;
}

inline bool TimeManager::finished_soft() const {
    return delta_ms(now(), m_time_start) >= m_soft_bound;
}

inline bool TimeManager::finished_hard() const {
    return delta_ms(now(), m_time_start) >= m_hard_bound;
}

inline TimeManager::TimeManager() {
    m_time_start = now();
}

} // illumina

#endif // ILLUMINA_TIMEMANAGER_H
