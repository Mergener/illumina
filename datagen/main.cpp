#include <iostream>
#include <sstream>
#include <random>

#include <illumina.h>

namespace illumina {

constexpr ui64 SEARCH_NODE_LIMIT    = 4096;
constexpr int  MIN_RANDOM_PLIES     = 6;
constexpr int  MAX_RANDOM_PLIES     = 12;
constexpr size_t MAX_BYTES          = 80ULL * 1024 * 1024 * 1024;
constexpr Score HI_SCORE            = 500;
constexpr int MAX_HI_SCORE_PLIES    = 6;
constexpr size_t POSITIONS_PER_GAME = 12;

struct GamePlyData {
    Score white_pov_score;
    Move  best_move;
};

struct Game {
    std::string outcome; // 0.5 for draw, 1.0 for white win, 0 for black win.
    Board       start_pos;
    std::vector<GamePlyData> ply_data;
};

struct OutputTuple {
    std::string fen;
    Score white_pov_score;
    std::string wdl;
};

Game simulate() {
    Game game;

    struct {
        Searcher searcher;
    } players[CL_COUNT];

    SearchSettings search_settings;
    search_settings.max_nodes = SEARCH_NODE_LIMIT;

    Board board = Board::standard_startpos();

    // Play some random moves to apply variety to the
    // starting positions.
    bool done_creating_startpos = false;
    size_t n_random_plies = random(MIN_RANDOM_PLIES, MAX_RANDOM_PLIES + 1);
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

        if (std::abs(search_results.score) > 120) {
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
        game.ply_data.push_back(ply_data);

        // Count how many successive times a position is
        // too good for either side. This will possibly
        // be used for win adjudications.
        if (ply_data.white_pov_score >= HI_SCORE) {
            hi_score_plies = hi_score_player == CL_WHITE
                ? hi_score_plies + 1
                : 1;
            hi_score_player = CL_WHITE;
        }
        else if (ply_data.white_pov_score <= -HI_SCORE) {
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
        if (hi_score_plies >= MAX_HI_SCORE_PLIES) {
            game.outcome = hi_score_player == CL_WHITE
                ? "1.0" : "0";
            break;
        }

        board.make_move(best_move);
        result = board.result();
    }

    // Set the WDL accordingly.
    // (1 for white wins, 0.5 for draws and 0 for black wins)
    if (result.is_draw()) {
        game.outcome = "0.5";
    }
    else if (result.is_finished()) {
        game.outcome = result.winner == CL_WHITE ? "1" : "0";
    }

    return game;
}

void generate_data() {
    // Initialize random number generator. Will be used
    // to shuffle data later.
    std::random_device rd;
    std::mt19937 g(rd());

    // Keep generating data until we reach the byte limit.
    size_t bytes = 0;
    while (bytes < MAX_BYTES) {
        // Vector to store and shuffle which data will be outputted from
        // each game.
        std::vector<OutputTuple> out_tuples;

        // Simulate the game and gather information about it.
        Game game = simulate();
        const std::vector<GamePlyData>& ply_data_vec = game.ply_data;

        // We'll recreate the positions from the game and filter out
        // some of them before we output them.
        Board board = game.start_pos;

        // Gather information from each ply from the simulated game.
        for (const GamePlyData& ply_data: ply_data_vec) {
            // We want to filter out some positions, so simply play out the moves
            // but don't add them to the out_tuples.
            if (board.in_check() ||                         // Checks shouldn't have a static eval.
                board.last_move().is_capture() ||           // Likely right before a recapture.
                is_mate_score(ply_data.white_pov_score)) {  // Mate scores behave differently than regular eval.
                board.make_move(ply_data.best_move);
                continue;
            }

            out_tuples.push_back({ board.fen(), ply_data.white_pov_score, game.outcome });

            board.make_move(ply_data.best_move);
        }

        // Since the tuples will be sliced, shuffling them allows us
        // to not only save data from the beginning of the game.
        std::shuffle(out_tuples.begin(), out_tuples.end(), g);

        for (size_t i = 0; i < std::min(out_tuples.size(), POSITIONS_PER_GAME); ++i) {
            const OutputTuple& out_tuple = out_tuples[i];

            std::stringstream ss;
            ss << out_tuple.fen             << " | "
               << out_tuple.white_pov_score << " | "
               << out_tuple.wdl             << '\n';

            std::string s = ss.str();
            bytes += s.size();
            std::cout << s;
        }
    }
}

}

int main(int argc, char* argv[]) {
    std::ios_base::sync_with_stdio(false);

    illumina::init();

    illumina::generate_data();
    return 0;
}