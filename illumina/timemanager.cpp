#include "timemanager.h"

namespace illumina {

void TimeManager::setup(bool tourney_time) {
    m_time_start = now();
    m_running.store(std::memory_order_seq_cst);
    m_tourney_time = tourney_time;
}

void TimeManager::stop() {
    if (!m_running.load(std::memory_order_relaxed)) {
        return;
    }
    m_running.store(std::memory_order_seq_cst);
    m_elapsed = elapsed();
}

void TimeManager::set_starting_bounds(ui64 soft, ui64 hard) {
    m_soft_bound = soft;
    m_hard_bound = hard;
    m_orig_soft_bound = soft;
    m_orig_hard_bound = hard;
}

void TimeManager::start_movetime(ui64 movetime_ms) {
    setup(false);
    ui64 bound = movetime_ms - std::min(movetime_ms, ui64(LAG_MARGIN));
    set_starting_bounds(bound, bound);
}

void TimeManager::start_tourney_time(ui64 our_time_ms,
                                     ui64 our_inc_ms,
                                     ui64 their_time_ms,
                                     ui64 their_inc_ms,
                                     int moves_to_go) {
    ui64 max_time = our_time_ms - std::min(our_time_ms, ui64(LAG_MARGIN));
    our_time_ms  += our_inc_ms * 45 - LAG_MARGIN;
    setup(true);

    if (moves_to_go != 1) {
        set_starting_bounds(std::min(max_time, ui64(double(our_time_ms) * 0.083)),
                            std::min(max_time, ui64(double(our_time_ms) * 0.333)));
    }
    else {
        set_starting_bounds(max_time, max_time);
    }
}

void TimeManager::on_new_pv(Depth depth,
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
        && elapsed() > ui64(double(m_hard_bound) * TM_CUTOFF_HARD_BOUND_FACTOR)) {
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
            m_soft_bound = ui64(double(m_soft_bound) * TM_STABILITY_SB_RED_FACTOR);
        }
    }
    else {
        // Our search deviated a bit from what we were expecting.
        // Give it some more thought.
        m_soft_bound = (m_soft_bound + m_orig_soft_bound * TM_STABILITY_SB_EXT_FACTOR) / 128;
        m_last_best_move = best_move;
        m_last_best_score = score;
    }
}

} // illumina