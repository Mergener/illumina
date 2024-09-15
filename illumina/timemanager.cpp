#include "timemanager.h"

#include "search.h"

namespace illumina {

void TimeManager::setup(bool tourney_time) {
    m_time_start   = now();
    m_running      = true;
    m_tourney_time = tourney_time;
    m_last_depth_elapsed_time = 1; // Start with 1 to prevent divisions by zero.
}

void TimeManager::stop() {
    if (!m_running) {
        return;
    }
    m_last_search_duration = elapsed();
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
    ui64 max_time = our_time_ms - std::min(our_time_ms, ui64(LAG_MARGIN));
    our_time_ms  += double(our_inc_ms) * TM_INCREMENT_FACTOR - LAG_MARGIN;
    setup(true);

    if (moves_to_go != 1) {
        set_starting_bounds(std::min(max_time, ui64(double(our_time_ms) * TM_BASE_SOFT_BOUND_FACTOR)),
                            std::min(max_time, ui64(double(our_time_ms) * TM_BASE_HARD_BOUND_FACTOR)));
    }
    else {
        set_starting_bounds(max_time, max_time);
    }
}

void TimeManager::on_new_pv(const PVResults& pv_results,
                            const Board& board) {
    // If not on tourney time, new pvs shouldn't affect
    // our thinking time.
    if (!m_tourney_time) {
        return;
    }

    // Compute the branching factor. We can use this value to determine whether
    // the search will be able to finish a new iteration.
    double total_elapsed = std::max(elapsed(), ui64(1));
    double branch_factor = total_elapsed / double(m_last_depth_elapsed_time);
    if (ui64(total_elapsed * branch_factor) > hard_bound()) {
        // New iteration would take too long. Halt.
        m_soft_bound = 0;
        return;
    }

    // Save last depth time point for future calculations.
    double multiplier = 0;
    m_last_depth_elapsed_time = total_elapsed;

    if (pv_results.depth >= TM_NODETM_MIN_DEPTH) {
        // We already know that the next iteration can be calculated successfully.
        // However, we might still decide that it is pointless spending too much
        // time if we seem to have an obvious move.
        double spent_effort = double(pv_results.best_move_nodes) / double(pv_results.pv_nodes);
        double positive_score = std::max(0, pv_results.score);
        if (spent_effort >= (TM_NODETM_THRESHOLD - positive_score / 64) &&
            (pv_results.best_move == m_last_best_move)) {
            multiplier += TM_NODETM_MULTIPLIER;
        }
    }

    // If current best move is the same as last move,
    // quicken our soft bound a little. Conversely,
    // extend it if we changed the mind on the best move.
    if (pv_results.depth >= TM_MOVE_STABILITY_MIN_DEPTH) {
        multiplier += (pv_results.best_move == m_last_best_move)
                   ?  TM_MOVE_STABILITY_MULTIPLIER
                   :  TM_MOVE_INSTABILITY_MULTIPLIER;
    }
    m_last_best_move = pv_results.best_move;

    m_soft_bound = double(m_soft_bound) * std::max(0.0, 1.0 - multiplier);
}

} // illumina