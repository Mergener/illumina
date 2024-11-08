#include "timemanager.h"

#include "search.h"

namespace illumina {

void TimeManager::setup(bool tourney_time) {
    m_time_start = now();
    m_running = true;
    m_tourney_time = tourney_time;
}

void TimeManager::stop() {
    if (!m_running) {
        return;
    }
    m_running = false;
}

void TimeManager::set_starting_bounds(ui64 soft, ui64 hard) {
    m_soft_bound = soft;
    m_hard_bound = hard;
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
    ui64 max_time = our_time_ms - LAG_MARGIN;
    max_time = std::max(max_time, ui64(1));

    setup(true);

    if (moves_to_go != 1) {
        set_starting_bounds(std::min(double(max_time), double(our_time_ms) * 0.05),
                            std::min(double(max_time), double(our_time_ms) * 0.33));
    }
    else {
        set_starting_bounds(max_time, max_time);
    }
}

void TimeManager::force_timeout() {
    m_soft_bound = 0;
    m_hard_bound = 0;
}

void TimeManager::on_new_pv(const PVResults& results) {
    // If not on tourney time, new pvs shouldn't affect
    // our thinking time.
    if (!m_tourney_time) {
        return;
    }

//    ui64 elapsed = delta_ms(now(), m_time_start);
//
//    // Branch factor timeout.
//    // If we think we're going to take too long to finish the next iteration,
//    // stop searching.
//    if (   results.depth >= 6
//        && results.bound_type == BT_EXACT) {
//        double branch_factor = double(results.nodes) / std::max(double(m_last_total_nodes), 1.0);
//        m_last_total_nodes   = results.nodes;
//
//        if (elapsed * branch_factor > m_hard_bound) {
//            force_timeout();
//            return;
//        }
//    }
}

} // illumina