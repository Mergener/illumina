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

inline void TimeManager::setup(bool tourney_time) {
    m_time_start = now();
    m_running = true;
    m_tourney_time = tourney_time;
}

inline bool TimeManager::running() const {
    return m_running;
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

inline void TimeManager::set_starting_bounds(ui64 soft, ui64 hard) {
    m_soft_bound = soft;
    m_hard_bound = hard;
    m_orig_soft_bound = soft;
    m_orig_hard_bound = hard;
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
    our_time_ms  += our_inc_ms * 45 - LAG_MARGIN;
    setup(true);

    if (moves_to_go != 1) {
        set_starting_bounds(std::min(max_time, our_time_ms / 12),
                            std::min(max_time, our_time_ms / 3));
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

inline void TimeManager::on_new_pv(Depth depth,
                                   Move best_move,
                                   Score score) {
    // If not on tourney time, new pvs shouldn't affect
    // our thinking time.
    if (!m_tourney_time) {
        return;
    }

    // If we think that our next search won't be finished
    // before the next depth ends, interrupt the search.
    if (depth >= TM_CUTOFF_MIN_DEPTH
        && elapsed() > (m_hard_bound * TM_CUTOFF_HARD_BOUND_FACTOR / TM_CUTOFF_HARD_BOUND_DIVISOR)) {
        m_soft_bound = 0;
        m_hard_bound = 0;
        return;
    }

    if (depth <= TM_STABILITY_MIN_DEPTH) {
        m_last_best_score = score;
        m_last_best_move  = best_move;
        return;
    }

    // If the last few plies had stable results, quicken
    // the search softly.
    Score cp_delta = score - m_last_best_score;
    if (   best_move == m_last_best_move
        && (cp_delta > TM_STABILITY_MIN_CP_DELTA && cp_delta < TM_STABILITY_MAX_CP_DELTA)) {
        // We have the same last move and a close score to the previous
        // iteration.
        m_stable_iterations++;

        if (m_stable_iterations >= TM_STABILITY_SB_RED_MIN_ITER) {
            // Search has been stable for a while, decrease our soft bound.
            m_soft_bound = m_soft_bound * TM_STABILITY_SB_RED_FACTOR / TM_STABILITY_SB_RED_DIVISOR;
        }
    }
    else {
        // Our search deviated a bit from what we were expecting.
        // Give it some more thought.
        m_soft_bound = (m_soft_bound + m_orig_soft_bound * TM_STABILITY_SB_EXT_FACTOR) / TM_STABILITY_SB_EXT_DIVISOR;
        m_last_best_move = best_move;
        m_last_best_score = score;
    }
}

} // illumina

#endif // ILLUMINA_TIMEMANAGER_H
