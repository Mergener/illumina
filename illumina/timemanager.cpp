#include "timemanager.h"

#include "tunablevalues.h"

namespace illumina {

static constexpr double OVERHEAD = 10.0;
static constexpr double MIN_TIME = 1.0;

void TimeManager::start(Color us, const SearchLimits& limits) {
    *this = {}; // make sure everything is properly reset on every search

    auto our_time = us == CL_WHITE ? limits.white_time : limits.black_time;
    if (!our_time.has_value() && !limits.move_time.has_value()) {
        m_infinite = true;
    }
    else {
        m_limits.move_time = limits.move_time;
        m_limits.our_time = our_time;
        m_limits.our_inc = us == CL_WHITE ? limits.white_inc.value_or(0) : limits.black_inc.value_or(0);
    }

    calculate_bounds();
}

void TimeManager::on_iteration_complete(Move best_move, ui64 nodes) {
    if (best_move == m_last_best_move) {
        m_move_stability_count++;
    }
    else {
        m_move_stability_count = 0;
    }
    m_last_best_move = best_move;
    m_nodes = nodes;

    calculate_bounds();
}

void TimeManager::add_spent_effort(Move move, ui64 nodes) {
    m_spent_effort->by_move[move.source()][move.destination()] += nodes;
}

void TimeManager::calculate_bounds() {
    if (m_infinite) {
        return;
    }

    if (!m_limits.our_time.has_value()) {
        m_hard_bound = m_soft_bound = std::max(*m_limits.move_time - OVERHEAD, 1.0);
        return;
    }

    const double total_time = *m_limits.our_time;
    const double increment = m_limits.our_inc;

    double hard_bound = total_time * TM_HARD_BOUND_FACTOR;
    double soft_bound = total_time * TM_SOFT_BOUND_FACTOR + increment * TM_INC_FACTOR;

    soft_bound *= TM_MOVE_STABILITY_BASE - m_move_stability_count * TM_MOVE_STABILITY_SLOPE;

    double spent_effort = static_cast<double>(m_spent_effort->by_move[m_last_best_move.source()][m_last_best_move.destination()]);
    double inverse_effort_ratio = 1 - spent_effort / static_cast<double>(m_nodes);
    soft_bound *= std::max(TM_NODES_BASE, inverse_effort_ratio * TM_NODES_SLOPE + TM_NODES_BIAS);

    if (m_limits.move_time.has_value()) {
        hard_bound = std::min(static_cast<double>(*m_limits.move_time) - OVERHEAD, hard_bound);
    }

    // Safeguard against latency...
    hard_bound = std::min(hard_bound, total_time - OVERHEAD);

    // ...except in extremely low time controls, where unpredictable behaviour
    // might arise from trying to play too fast.
    hard_bound = std::max(hard_bound, MIN_TIME);

    soft_bound = std::min(soft_bound, hard_bound);

    m_hard_bound = hard_bound;
    m_soft_bound = soft_bound;
}

} // illumina