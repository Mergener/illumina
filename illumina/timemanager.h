#ifndef ILLUMINA_TIMEMANAGER_H
#define ILLUMINA_TIMEMANAGER_H

#include "clock.h"
#include "searchdefs.h"
#include "tunablevalues.h"
#include "types.h"

namespace illumina {

static constexpr int LAG_MARGIN = 10;

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
    bool running() const;
    void stop();

    void on_new_pv(Depth depth, Move best_move, Score score);

    TimeManager();

private:
    TimePoint m_time_start {};
    ui64 m_orig_soft_bound = 0;
    ui64 m_orig_hard_bound = 0;
    ui64 m_soft_bound = 0;
    ui64 m_hard_bound = 0;
    ui64 m_elapsed = 0;
    bool m_running = false;
    bool m_tourney_time = false;

    Move m_last_best_move   = MOVE_NULL;
    Score m_last_best_score = 0;
    int m_stable_iterations = 0;

    void setup(bool tourney_time);
    void set_starting_bounds(ui64 soft, ui64 hard);
};

inline bool TimeManager::running() const {
    return m_running;
}

inline ui64 TimeManager::elapsed() const {
    return m_running ? delta_ms(now(), m_time_start) : m_elapsed;
}

inline ui64 TimeManager::soft_bound() const {
    return m_soft_bound;
}

inline ui64 TimeManager::hard_bound() const {
    return m_hard_bound;
}

inline bool TimeManager::finished_soft() const {
    return m_running && ui64(delta_ms(now(), m_time_start)) >= m_soft_bound;
}

inline bool TimeManager::finished_hard() const {
    return m_running && ui64(delta_ms(now(), m_time_start)) >= m_hard_bound;
}

inline TimeManager::TimeManager() {
    m_time_start = now();
}

} // illumina

#endif // ILLUMINA_TIMEMANAGER_H
