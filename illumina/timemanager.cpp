#include "timemanager.h"

namespace illumina {

static constexpr i64 LAG_MARGIN = 10;

void TimeManager::start_normal(i64 our_time_ms, i64 our_inc_ms, i64 their_time_ms,
                               i64 their_inc_ms, int moves_to_go) {
    m_time_start = Clock::now();
    m_mode = NORMAL;

    double hard = double(our_time_ms) * TM_HARD_FACTOR;
    double soft = double(our_time_ms) * TM_SOFT_FACTOR
                + double(our_inc_ms) * TM_INC_FACTOR;

    m_hard = std::max(i64(1), i64(hard) - LAG_MARGIN);
    m_soft = std::max(i64(1), i64(soft));
}

void TimeManager::start_movetime(i64 movetime_ms) {
    m_time_start = Clock::now();
    m_mode = MOVETIME;

    m_hard = std::max(i64(1), movetime_ms - LAG_MARGIN);
    m_soft = m_hard;
}

void TimeManager::start_infinite() {
    m_time_start = Clock::now();
    m_mode = INFINITE;
}

bool TimeManager::time_up_hard() const {
    if (m_mode == INFINITE) {
        return false;
    }

    return delta_ms(Clock::now(), m_time_start) >= m_hard;
}

bool TimeManager::time_up_soft() const {
    if (m_mode == INFINITE) {
        return false;
    }

    return m_force_finish_soft || (delta_ms(Clock::now(), m_time_start) >= m_soft);
}

void TimeManager::pv_finished(const illumina::PVResults &pv_results) {
}

} // illumina