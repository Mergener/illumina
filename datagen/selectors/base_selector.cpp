#include "base_selector.h"

namespace illumina {

std::vector<DataPoint> BaseSelector::select(ThreadContext& ctx,
                                            const Game& game) {
    std::random_device rnd;
    std::mt19937 rng(rnd());

    // We'll recreate the positions from the game and filter out
    // some of them before we output them.
    std::vector<DataPoint> extracted_data;
    Board board = game.start_pos;
    const std::vector<GamePlyData>& ply_data_vec = game.ply_data;

    // Gather information from each ply from the simulated game.
    for (const GamePlyData& ply_data: ply_data_vec) {
        board.make_move(ply_data.best_move);

        // Apply filters.
        if (   board.in_check()                               // Checks shouldn't have a static eval.
               || board.last_move().is_capture()              // Likely right before a recapture.
               || is_mate_score(ply_data.white_pov_score)) {  // Mate scores behave differently than regular eval.
            continue;
        }

        extracted_data.push_back({ board.fen(false), ply_data });
    }

    // Since the tuples will be sliced, shuffling them allows us
    // to not only save data from the beginning of the game.
    std::shuffle(extracted_data.begin(), extracted_data.end(), rng);

    // We want longer games to skew towards the upper bound of extracted data points.
    size_t n_data_points = std::min(size_t(m_min_positions_per_game) + ply_data_vec.size() / 32,
                                    size_t(m_max_positions_per_game));
    n_data_points = std::min(n_data_points, extracted_data.size());
    extracted_data.erase(extracted_data.begin() + n_data_points, extracted_data.end());

    return extracted_data;
}

BaseSelector::~BaseSelector() noexcept {
}

} // illumina
