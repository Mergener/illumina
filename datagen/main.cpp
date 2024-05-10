#include <iostream>
#include <sstream>
#include <random>

#include <illumina.h>

namespace illumina {

constexpr ui64 SEARCH_NODE_LIMIT    = 4096;
constexpr int  N_RANDOM_PLIES       = 16;
constexpr size_t MAX_BYTES          = 80ULL * 1024 * 1024 * 1024;
constexpr Score HI_SCORE            = 6000;
constexpr int MAX_HI_SCORE_PLIES    = 6;
constexpr size_t POSITIONS_PER_GAME = 15;
//constexpr size_t MAX_BYTES       = 2000;

struct GamePlyData {
    Score white_pov_score;
    Move  best_move;
};

struct Game {
    std::string outcome; // 0.5 for draw, 1.0 for white win, 0 for black win.
    Board start_pos;
    std::vector<GamePlyData> ply_data;
};

Game simulate() {
    Game game;

    struct {
        Searcher searcher;
    } players[CL_COUNT];

    SearchSettings search_settings;
    search_settings.max_nodes = SEARCH_NODE_LIMIT;

    Board board = Board::standard_startpos();

    // Play some random moves.
    for (int i = 0; i < N_RANDOM_PLIES; ++i) {
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

        Move move = legal_moves[random(size_t(0), n_moves)];
        board.make_move(move);
    }

    game.start_pos = board;
    int hi_score_plies = 0;
    Color hi_score_player = CL_WHITE;

    // Finally, play the game.
    BoardResult result = board.result();
    while (!result.is_finished()) {
        GamePlyData ply_data {};
        auto& player = players[board.color_to_move()];

        SearchResults search_results = player.searcher.search(board, search_settings);
        Move best_move = search_results.best_move;

        ply_data.best_move = best_move;
        ply_data.white_pov_score = board.color_to_move() == CL_WHITE
            ? search_results.score
            : -search_results.score;
        game.ply_data.push_back(ply_data);

        if (ply_data.white_pov_score > HI_SCORE) {
            hi_score_plies = hi_score_player == CL_WHITE
                ? hi_score_plies + 1
                : 0;
            hi_score_player = CL_WHITE;
        }
        else if (ply_data.white_pov_score < -HI_SCORE) {
            hi_score_plies = hi_score_player == CL_BLACK
                             ? hi_score_plies + 1
                             : 0;
            hi_score_player = CL_BLACK;
        }
        else {
            hi_score_plies = 0;
        }

        if (hi_score_plies >= MAX_HI_SCORE_PLIES) {
            game.outcome = hi_score_player == CL_WHITE
                ? "1.0" : "0";
            break;
        }


        board.make_move(best_move);

        result = board.result();
    }

    if (result.is_draw()) {
        game.outcome = "0.5";
    }
    else if (result.is_finished()) {
        game.outcome = result.winner == CL_WHITE ? "1" : "0";
    }

    return game;
}

void generate_data() {
    std::random_device rd;
    std::mt19937 g(rd());

    size_t bytes = 0;
    while (bytes < MAX_BYTES) {
        Game game = simulate();
        Board board = game.start_pos;
        std::vector<GamePlyData> ply_data_vec = game.ply_data;
        std::shuffle(ply_data_vec.begin(), ply_data_vec.end(), g);

        size_t n = 0;
        for (const GamePlyData& ply_data: ply_data_vec) {
            if (n >= POSITIONS_PER_GAME) {
                break;
            }

            if (board.in_check() ||
                board.last_move().is_capture() ||
                is_mate_score(ply_data.white_pov_score)) {
                board.make_move(ply_data.best_move);
                continue;
            }

            std::stringstream ss;
            ss << ply_data.best_move.to_uci() << ','
               << board.fen()              << ','
               << ply_data.white_pov_score << ','
               << game.outcome << std::endl;

            std::string s = ss.str();
            bytes += s.size();
            std::cout << s;

            board.make_move(ply_data.best_move);
            n++;
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