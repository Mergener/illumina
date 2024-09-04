#include "timemanager.h"

#include "search.h"

namespace illumina {

void TimeManager::on_new_pv(const PVResults& pv_results) {
    // If not on tourney time, new pvs shouldn't affect
    // our thinking time.
    if (!m_tourney_time) {
        return;
    }

    // Compute the branching factor. We can use this value to determine whether
    // the search will be able to finish a new iteration.
    double total_elapsed = elapsed();
    double branch_factor = total_elapsed / double(m_last_depth_elapsed_time);
    if (ui64(total_elapsed * branch_factor) > hard_bound()) {
        // New iteration would take too long. Halt.
        stop();
        return;
    }

    // Save last depth time point for future calculations.
    m_last_depth_elapsed_time = total_elapsed;

    if (pv_results.depth >= TM_NODETM_MIN_DEPTH) {
        // We already know that the next iteration can be calculated successfully.
        // However, we might still decide that it is pointless spending too much
        // time if we seem to have an obvious move.
        double spent_effort = double(pv_results.best_move_nodes) / double(pv_results.pv_nodes);
        if (spent_effort < TM_SPENT_EFFORT_THRESHOLD) {
            spent_effort = 0;
        }
        double factor = (1 - spent_effort * spent_effort);
        m_soft_bound = double(m_soft_bound) * factor;
    }
}

} // illumina