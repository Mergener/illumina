#include "base_extractor.h"

namespace illumina {

BaseExtractor::BaseExtractor(int min_positions_per_game,
              int max_positions_per_game)
    : m_min_positions_per_game(min_positions_per_game),
      m_max_positions_per_game(max_positions_per_game) { }

std::vector<ExtractedData> BaseExtractor::extract_data(ThreadContext& ctx,
                                                       const Game& game) {
    std::random_device rnd;
    std::mt19937 rng(rnd());

    // We'll recreate the positions from the game and filter out
    // some of them before we output them.
    std::vector<ExtractedData> extracted_data;
    Board board = game.start_pos;
    const std::vector<GamePlyData>& ply_data_vec = game.ply_data;

    // Gather information from each ply from the simulated game.
    for (const GamePlyData& ply_data: ply_data_vec) {
        // We want to filter out some positions, so simply play out the moves
        // but don't add them to the out_tuples.
        if (   board.in_check()                               // Checks shouldn't have a static eval.
               || board.last_move().is_capture()              // Likely right before a recapture.
               || is_mate_score(ply_data.white_pov_score)) {  // Mate scores behave differently than regular eval.
            board.make_move(ply_data.best_move);
            continue;
        }

        extracted_data.push_back({ board.fen(false), ply_data });

        board.make_move(ply_data.best_move);
    }

    // Since the tuples will be sliced, shuffling them allows us
    // to not only save data from the beginning of the game.
    std::shuffle(extracted_data.begin(), extracted_data.end(), rng);

    size_t n_pos = std::min(size_t(m_min_positions_per_game) + ply_data_vec.size() / 30,
                            size_t(m_max_positions_per_game));
    n_pos = std::min(n_pos, extracted_data.size());

    extracted_data.erase(extracted_data.begin(), extracted_data.begin() + n_pos);

    return extracted_data;
}

BaseExtractor::~BaseExtractor() noexcept {
}

} // illumina
