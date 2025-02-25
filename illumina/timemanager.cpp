#include "timemanager.h"

#include "search.h"

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

    m_best_move_stability = 0;
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

void TimeManager::on_pv_results(const PVResults& pv_results) {
    if (m_last_best_move == pv_results.best_move) {
        m_best_move_stability = std::min(m_best_move_stability + 1, 12);
    } else {
        m_best_move_stability = 0;
    }
    m_last_best_move = pv_results.best_move;

    m_best_move_node_pct = double(pv_results.best_move_nodes) / double(pv_results.nodes);
}

bool TimeManager::time_up_soft() const {
    if (m_mode != NORMAL) {
        return false;
    }

    i64 elapsed = delta_ms(Clock::now(), m_time_start);

    double bm_stability = TM_BM_STABILITY_BASE - TM_BM_STABILITY_SLOPE * m_best_move_stability;
    double nodes = std::max(TM_NODES_BASE, (1.0 - m_best_move_node_pct) * TM_NODES_SLOPE + TM_NODES_BIAS);
    double soft = double(m_soft) * bm_stability * (m_last_best_move != MOVE_NULL ? nodes : 1.0);

    return elapsed >= i64(soft);
}

} // illumina