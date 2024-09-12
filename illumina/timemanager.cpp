#include "timemanager.h"

#include "search.h"

namespace illumina {

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
        if (spent_effort >= (TM_NODETM_THRESHOLD - positive_score / 64)) {
            multiplier += TM_NODETM_MULTIPLIER;

            // We have spent considerable time in this node. As a plus, recaptures
            // are very often singular moves. When we find ourselves in a position
            // with only a single good recapture and some certain stability, we might
            // save time by not searching it for too long.
            Move last_move = board.last_move();
            if (pv_results.best_move == m_last_best_move &&
                last_move.is_capture() &&
                pv_results.best_move.destination() == last_move.destination()) {
                // Check if this is the only possible recapture.
                Move moves[MAX_GENERATED_MOVES];
                Move* end = generate_moves<BIT(MT_SIMPLE_CAPTURE)>(board, moves);

                bool has_multiple_recaptures = false;
                for (Move* it = &moves[0]; it != end; ++it) {
                    Move capture = *it;
                    if (capture.destination() != last_move.destination()) {
                        // Not a recapture.
                        continue;
                    }

                    if (capture.destination() == last_move.destination() &&
                        capture.source() != last_move.source()) {
                        // We have a distinct recapture.
                        has_multiple_recaptures = true;
                        continue;
                    }
                }
                multiplier += !has_multiple_recaptures * TM_RECAPTURE_MULTIPLIER;
            }
        }
    }

    // If current best move is the same as last move,
    // quicken our soft bound a little bit. Conversely,
    // extend it if we changed the mind on the best move.
    if (pv_results.depth >= TM_MOVE_STABILITY_MIN_DEPTH) {
        multiplier += (pv_results.best_move == m_last_best_move)
                      ? TM_MOVE_STABILITY_MULTIPLIER
                      : TM_MOVE_INSTABILITY_MULTIPLIER;
    }
    m_last_best_move = pv_results.best_move;

    m_soft_bound = double(m_soft_bound) * std::max(0.0, 1.0 - multiplier);
}

} // illumina