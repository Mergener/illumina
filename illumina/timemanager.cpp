#include "timemanager.h"

#include "tunablevalues.h"

namespace illumina {

static constexpr double OVERHEAD = 10.0;
static constexpr double MIN_TIME = 1.0;

void TimeManager::start(Color us, const SearchLimits& limits) {
    m_time_start = now();

    calculate_bounds(us, limits);
}

void TimeManager::calculate_bounds(Color us, const SearchLimits& limits) {
    auto our_time = us == CL_WHITE ? limits.white_time : limits.black_time;
    if (!our_time.has_value() && !limits.move_time.has_value()) {
        m_infinite = true;
        return;
    }

    m_infinite = false;

    if (!our_time.has_value()) {
        m_hard_bound = m_soft_bound = std::max(*limits.move_time - OVERHEAD, 1.0);
        return;
    }

    const double total_time = *our_time;
    const double increment = (us == CL_WHITE ? limits.white_inc : limits.black_inc).value_or(0);

    double hard_bound = total_time * TM_HARD_BOUND_FACTOR;
    double soft_bound = total_time * TM_SOFT_BOUND_FACTOR + increment * TM_INC_FACTOR;

    if (limits.move_time.has_value()) {
        hard_bound = std::min(static_cast<double>(*limits.move_time) - OVERHEAD, hard_bound);
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