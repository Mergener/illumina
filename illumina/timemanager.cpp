#include "timemanager.h"

#include "search.h"

namespace illumina {

constexpr int LAG_MARGIN = 10;

bool TimeManager::finished_soft() const {
    return m_running && delta_ms(now(), m_time_start) >= m_soft_bound;
}

bool TimeManager::finished_hard() const {
    return m_running && delta_ms(now(), m_time_start) >= m_hard_bound;
}

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
    // max_time is the total time we have subtracted by the lag margin.
    // Also, we cap max time so that it is never less than 1ms.
    ui64 max_time = our_time_ms - std::min(ui64(LAG_MARGIN), our_time_ms - 1);
    m_expected_branch_factor = 1.5;

    setup(true);

    if (moves_to_go != 1) {
        set_starting_bounds(std::min(double(max_time), double(our_time_ms) * 0.05 + double(our_inc_ms) * 0.5),
                            max_time);
    }
    else {
        // If moves_to_go is 1, we are regaining our time in the next move.
        // We can safely use all our remaining time here.
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

    ui64 elapsed = delta_ms(now(), m_time_start);

    // Branch factor timeout.
    // If we think we're going to take too long to finish the next iteration,
    // stop searching.
    double branch_factor = double(results.nodes) / std::max(double(m_last_total_nodes), 1.0);
    m_expected_branch_factor = (branch_factor + m_expected_branch_factor) / 2;
    m_last_total_nodes   = results.nodes;
    if (   results.depth >= 6
        && results.bound_type == BT_EXACT
        && elapsed * m_expected_branch_factor > m_hard_bound) {
        force_timeout();
        return;
    }
}

} // illumina