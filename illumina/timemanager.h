#ifndef ILLUMINA_TIMEMANAGER_H
#define ILLUMINA_TIMEMANAGER_H

#include "clock.h"
#include "searchdefs.h"
#include "tunablevalues.h"
#include "types.h"

namespace illumina {

static constexpr int LAG_MARGIN = 3;

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
    ui64 elapsed() const;
    bool running() const;
    void stop();

    void on_new_pv(const PVResults& pv_results,
                   const Board& board);

    TimeManager();

private:
    TimePoint m_time_start {};
    ui64 m_soft_bound = 0;
    ui64 m_hard_bound = 0;
    ui64 m_last_search_duration = 1;
    bool m_running = false;
    bool m_tourney_time = false;

    ui64 m_last_depth_elapsed_time = 1;
    Move m_last_best_move = MOVE_NULL;

    void setup(bool tourney_time);
    void set_starting_bounds(ui64 soft, ui64 hard);
};

inline void TimeManager::setup(bool tourney_time) {
    m_time_start = now();
    m_running = true;
    m_tourney_time = tourney_time;
    m_last_depth_elapsed_time = 1; // Start with 1 to prevent divisions by zero.
}

inline bool TimeManager::running() const {
    return m_running;
}

inline ui64 TimeManager::elapsed() const {
    return m_running ? delta_ms(now(), m_time_start) : m_last_search_duration;
}

inline void TimeManager::stop() {
    if (!m_running) {
        return;
    }
    m_last_search_duration = elapsed();
    m_running = false;
}

inline void TimeManager::set_starting_bounds(ui64 soft, ui64 hard) {
    m_soft_bound = soft;
    m_hard_bound = hard;
}

inline void TimeManager::start_movetime(ui64 movetime_ms) {
    setup(false);
    ui64 bound = movetime_ms - std::min(movetime_ms, ui64(LAG_MARGIN));
    set_starting_bounds(bound, bound);
}

inline void TimeManager::start_tourney_time(ui64 our_time_ms,
                                            ui64 our_inc_ms,
                                            ui64 their_time_ms,
                                            ui64 their_inc_ms,
                                            int moves_to_go) {
    ui64 max_time = our_time_ms - std::min(our_time_ms, ui64(LAG_MARGIN));
    our_time_ms  += double(our_inc_ms) * TM_INCREMENT_FACTOR - LAG_MARGIN;
    setup(true);

    if (moves_to_go != 1) {
        set_starting_bounds(std::min(max_time,  ui64(double(our_time_ms) * TM_BASE_SOFT_BOUND_FACTOR)),
                            std::min(max_time, ui64(double(our_time_ms) * TM_BASE_HARD_BOUND_FACTOR)));
    }
    else {
        set_starting_bounds(max_time, max_time);
    }
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
