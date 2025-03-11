#include "simulation.h"

namespace illumina {

Game simulate(const GameOptions& options) {
    Game game;

    struct {
        Searcher searcher;
    } players[CL_COUNT];

    SearchSettings search_settings;
    search_settings.max_nodes = options.search_node_limit;
    search_settings.move_time = 10000;

    Board board = Board::standard_startpos();

    // Play some random moves to apply variety to the
    // starting positions, but don't add positions that
    // are too one-sided.
    bool done_creating_startpos = false;
    int n_random_plies = random(options.min_random_plies, options.max_random_plies + 1);
    while (!done_creating_startpos) {
        for (int i = 0; i < n_random_plies; ++i) {
            // Generate all legal moves.
            Move legal_moves[MAX_GENERATED_MOVES];
            Move* begin = legal_moves;
            Move* end = generate_moves(board, begin);
            size_t n_moves = end - begin;

            if (n_moves == 0) {
                // Oops, we entered a stalemate or checkmate position.
                // Rewind to the start.
                board = Board::standard_startpos();
                i = -1;
                continue;
            }

            // Play a random move.
            Move move = legal_moves[random(size_t(0), n_moves)];
            board.make_move(move);
        }

        // Evaluate the resulting start position. If the evaluation is too
        // skewed towards one side, re-create.
        auto& player = players[board.color_to_move()];

        SearchSettings validation_search_settings = search_settings;
        validation_search_settings.max_nodes = UINT64_MAX;
        validation_search_settings.max_depth = 2;

        SearchResults search_results = player.searcher.search(board, validation_search_settings);

        if (std::abs(search_results.score) >= 200) {
            // Position is excessively imbalanced.
            // Rewind to the start.
            board = Board::standard_startpos();
            continue;
        }

        done_creating_startpos = true;
    }

    game.start_pos = board;
    int hi_score_plies = 0;
    Color hi_score_player = CL_WHITE;

    // Finally, play the game.
    BoardResult result = board.result();
    while (!result.is_finished()) {
        GamePlyData ply_data {};
        auto& player = players[board.color_to_move()];

        // Perform a shallow search to simulate a move.
        SearchResults search_results = player.searcher.search(board, search_settings);
        Move best_move = search_results.best_move;

        // Save position data.
        ply_data.best_move = best_move;
        ply_data.white_pov_score = board.color_to_move() == CL_WHITE
                                   ? search_results.score
                                   : -search_results.score;

        ply_data.white_pov_score = std::clamp(ply_data.white_pov_score, -3000, 3000);

        game.ply_data.push_back(ply_data);

        // Count how many successive times a position is
        // too good for either side. This will possibly
        // be used for win adjudications.
        if (ply_data.white_pov_score >= options.win_adjudication_score) {
            hi_score_plies = hi_score_player == CL_WHITE
                             ? hi_score_plies + 1
                             : 1;
            hi_score_player = CL_WHITE;
        }
        else if (ply_data.white_pov_score <= -options.win_adjudication_score) {
            hi_score_plies = hi_score_player == CL_BLACK
                             ? hi_score_plies + 1
                             : 1;
            hi_score_player = CL_BLACK;
        }
        else {
            hi_score_plies = 0;
        }

        // Check for adjudications.
        // If we get successive very high scores for either side,
        // we can adjudicate a win for the better side.
        if (hi_score_plies >= options.win_adjudication_plies) {
            result = { hi_score_player, BoardOutcome::CHECKMATE };
            break;
        }

        board.make_move(best_move);
        result = board.result();
    }

    game.result = result;
    return game;
}

} // illumina
