#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <argparse/argparse.hpp>

#include <illumina.h>

#include "logger.h"

namespace illumina {

namespace {

struct GamePlyData {
    Score white_pov_score;
    Move  best_move;
};

struct Game {
    BoardResult result;
    Board       start_pos;
    std::vector<GamePlyData> ply_data;
};

struct NormalOptions {
    int threads = 1;
    std::string out_file_name {};
    ui64 search_node_limit = 6000;
    int min_random_plies = 8;
    int max_random_plies = 16;
    int win_adjudication_score = 1000;
    int win_adjudication_plies = 6;
    int min_positions_per_game = 12;
    int max_positions_per_game = 16;
    bool exclude_checks = true;
    bool exclude_mate_scores = true;
    bool exclude_last_move_captures = true;
};

struct DataPoint {
    std::string fen;
    GamePlyData ply_data;
};

NormalOptions parse_args(int argc, char* argv[]) {
    auto options = NormalOptions{};
    auto include_checks = false;
    auto include_mate_scores = false;
    auto include_last_move_captures = false;

    auto args = argparse::ArgumentParser(argv[0],
                                         "",
                                         argparse::default_arguments::none);

    args.add_argument("-h", "--help")
        .action([&args](const auto&) {
            std::cout << args;
            std::exit(0);
        })
        .default_value(false)
        .implicit_value(true)
        .nargs(0)
        .help("shows help message and exits");

    args.add_argument("-t", "--threads")
        .default_value(options.threads)
        .store_into(options.threads)
        .help("number of threads. Helper threads will write to files postfixed by the thread index.");

    args.add_argument("-o", "--output")
        .required()
        .store_into(options.out_file_name)
        .help("output file name.");

    args.add_argument("--search-node-limit")
        .default_value(options.search_node_limit)
        .store_into(options.search_node_limit)
        .help("node limit for each simulated move search.");

    args.add_argument("--min-random-plies")
        .default_value(options.min_random_plies)
        .store_into(options.min_random_plies)
        .help("minimum number of random opening plies before the game starts.");

    args.add_argument("--max-random-plies")
        .default_value(options.max_random_plies)
        .store_into(options.max_random_plies)
        .help("maximum number of random opening plies before the game starts.");

    args.add_argument("--win-adjudication-score")
        .default_value(options.win_adjudication_score)
        .store_into(options.win_adjudication_score)
        .help("absolute score threshold used for win adjudication.");

    args.add_argument("--win-adjudication-plies")
        .default_value(options.win_adjudication_plies)
        .store_into(options.win_adjudication_plies)
        .help("number of consecutive high-score plies needed for win adjudication.");

    args.add_argument("--min-positions-per-game")
        .default_value(options.min_positions_per_game)
        .store_into(options.min_positions_per_game)
        .help("minimum number of positions kept from each game.");

    args.add_argument("--max-positions-per-game")
        .default_value(options.max_positions_per_game)
        .store_into(options.max_positions_per_game)
        .help("maximum number of positions kept from each game.");

    args.add_argument("--include-checks")
        .default_value(false)
        .implicit_value(true)
        .store_into(include_checks)
        .help("keep positions where the side to move is in check.");

    args.add_argument("--include-mate-scores")
        .default_value(false)
        .implicit_value(true)
        .store_into(include_mate_scores)
        .help("keep positions whose search score is a mate score.");

    args.add_argument("--include-last-move-captures")
        .default_value(false)
        .implicit_value(true)
        .store_into(include_last_move_captures)
        .help("keep positions whose previous move was a capture.");

    try {
        args.parse_args(argc, argv);
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        std::cerr << args;
        std::exit(EXIT_FAILURE);
    }

    if (options.threads < 1) {
        throw std::invalid_argument("threads must be at least 1");
    }
    if (options.min_random_plies < 0) {
        throw std::invalid_argument("min-random-plies must be non-negative");
    }
    if (options.max_random_plies < options.min_random_plies) {
        throw std::invalid_argument("max-random-plies must be >= min-random-plies");
    }
    if (options.min_positions_per_game < 0) {
        throw std::invalid_argument("min-positions-per-game must be non-negative");
    }
    if (options.max_positions_per_game < options.min_positions_per_game) {
        throw std::invalid_argument("max-positions-per-game must be >= min-positions-per-game");
    }
    if (options.win_adjudication_plies < 1) {
        throw std::invalid_argument("win-adjudication-plies must be at least 1");
    }

    options.exclude_checks = !include_checks;
    options.exclude_mate_scores = !include_mate_scores;
    options.exclude_last_move_captures = !include_last_move_captures;
    return options;
}

std::string output_file_name(const NormalOptions& options, int thread_index) {
    if (thread_index == 0) {
        return options.out_file_name;
    }

    auto output_path = std::filesystem::path(options.out_file_name);
    const auto extension = output_path.extension();

    if (extension.empty()) {
        output_path += "_" + std::to_string(thread_index);
        return output_path.string();
    }

    output_path.replace_filename(output_path.stem().string()
                                 + "_"
                                 + std::to_string(thread_index)
                                 + extension.string());
    return output_path.string();
}

Game simulate_game(Searcher& white_searcher,
                   Searcher& black_searcher,
                   const NormalOptions& options) {
    Game game;

    struct {
        Searcher* searcher;
    } players[CL_COUNT] = {
        { &white_searcher },
        { &black_searcher }
    };

    SearchSettings search_settings;
    search_settings.max_nodes = options.search_node_limit;
    search_settings.move_time = 10000;

    auto board = Board::standard_startpos();

    auto done_creating_startpos = false;
    auto n_random_plies = random(options.min_random_plies, options.max_random_plies + 1);
    while (!done_creating_startpos) {
        for (auto i = 0; i < n_random_plies; ++i) {
            Move legal_moves[MAX_GENERATED_MOVES];
            auto* begin = legal_moves;
            auto* end = generate_moves(board, begin);
            auto n_moves = size_t(end - begin);

            if (n_moves == 0) {
                board = Board::standard_startpos();
                i = -1;
                continue;
            }

            auto move = legal_moves[random(size_t(0), n_moves)];
            board.make_move(move);
        }

        auto& player = players[board.color_to_move()];

        auto validation_search_settings = search_settings;
        validation_search_settings.max_nodes = UINT64_MAX;
        validation_search_settings.max_depth = 2;

        auto search_results = player.searcher->search(board, validation_search_settings);

        if (std::abs(search_results.score) >= 200) {
            board = Board::standard_startpos();
            continue;
        }

        done_creating_startpos = true;
    }

    game.start_pos = board;
    auto hi_score_plies = 0;
    auto hi_score_player = CL_WHITE;

    auto result = board.result();
    while (!result.is_finished()) {
        auto ply_data = GamePlyData{};
        auto& player = players[board.color_to_move()];

        auto search_results = player.searcher->search(board, search_settings);
        auto best_move = search_results.best_move;

        ply_data.best_move = best_move;
        ply_data.white_pov_score = board.color_to_move() == CL_WHITE
                                   ? search_results.score
                                   : -search_results.score;
        ply_data.white_pov_score = std::clamp(ply_data.white_pov_score, -3000, 3000);

        game.ply_data.push_back(ply_data);

        if (ply_data.white_pov_score >= options.win_adjudication_score) {
            hi_score_plies = hi_score_player == CL_WHITE ? hi_score_plies + 1 : 1;
            hi_score_player = CL_WHITE;
        }
        else if (ply_data.white_pov_score <= -options.win_adjudication_score) {
            hi_score_plies = hi_score_player == CL_BLACK ? hi_score_plies + 1 : 1;
            hi_score_player = CL_BLACK;
        }
        else {
            hi_score_plies = 0;
        }

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

std::vector<DataPoint> select_data_points(const Game& game,
                                          const NormalOptions& options) {
    static thread_local auto rng = std::mt19937(std::random_device{}());

    std::vector<DataPoint> extracted_data;
    auto board = game.start_pos;

    for (const GamePlyData& ply_data : game.ply_data) {
        board.make_move(ply_data.best_move);

        if (options.exclude_checks && board.in_check()) {
            continue;
        }
        if (options.exclude_last_move_captures && board.last_move().is_capture()) {
            continue;
        }
        if (options.exclude_mate_scores && is_mate_score(ply_data.white_pov_score)) {
            continue;
        }

        extracted_data.push_back({ board.fen(false), ply_data });
    }

    std::shuffle(extracted_data.begin(), extracted_data.end(), rng);

    auto n_data_points = std::min(size_t(options.min_positions_per_game) + game.ply_data.size() / 32,
                                    size_t(options.max_positions_per_game));
    n_data_points = std::min(n_data_points, extracted_data.size());
    extracted_data.erase(extracted_data.begin() + n_data_points, extracted_data.end());

    return extracted_data;
}

std::string_view wdl_value(const Game& game) {
    switch (game.result.outcome) {
    case BoardOutcome::CHECKMATE:
        return game.result.winner.value_or(CL_WHITE) == CL_WHITE ? "1.0" : "0.0";
    case BoardOutcome::UNFINISHED:
    case BoardOutcome::STALEMATE:
    case BoardOutcome::DRAW_BY_REPETITION:
    case BoardOutcome::DRAW_BY_50_MOVES_RULE:
    case BoardOutcome::DRAW_BY_INSUFFICIENT_MATERIAL:
        return "0.5";
    }

    return "0.5";
}

ui64 write_marlinflow(std::ostream& stream,
                      const Game& game,
                      const std::vector<DataPoint>& extracted_data) {
    const auto wdl = wdl_value(game);

    for (const DataPoint& data : extracted_data) {
        stream << data.fen << " | "
               << data.ply_data.white_pov_score << " | "
               << wdl << '\n';
    }

    return extracted_data.size();
}

std::string bytes_str(ui64 bytes) {
    constexpr auto KiB = ui64(1024ull);
    constexpr auto MiB = KiB * 1024ull;
    constexpr auto GiB = MiB * 1024ull;
    constexpr auto TiB = GiB * 1024ull;

    if (bytes < KiB) {
        return std::to_string(bytes) + " B";
    }

    std::string unit;
    auto divisor = KiB;

    if (bytes < MiB) {
        unit = " KiB";
    }
    else if (bytes < GiB) {
        divisor = MiB;
        unit = " MiB";
    }
    else if (bytes < TiB) {
        divisor = GiB;
        unit = " GiB";
    }
    else {
        divisor = TiB;
        unit = " TiB";
    }

    return std::to_string(bytes / divisor)
         + "."
         + std::to_string(int(double(bytes % divisor) / divisor * 10))
         + unit;
}

std::string time_str(ui64 elapsed_ms) {
    constexpr auto one_minute = ui64(60);
    constexpr auto one_hour = one_minute * 60;

    auto elapsed_seconds = elapsed_ms / 1000;

    if (elapsed_seconds < one_minute) {
        return std::to_string(elapsed_seconds) + "s";
    }
    if (elapsed_seconds < one_hour) {
        return std::to_string(elapsed_seconds / one_minute) + "m"
             + std::to_string(elapsed_seconds % one_minute) + "s";
    }
    return std::to_string(elapsed_seconds / one_hour) + "h"
         + std::to_string((elapsed_seconds % one_hour) / one_minute) + "m"
         + std::to_string(elapsed_seconds % one_minute);
}

void log_configuration(const NormalOptions& options) {
    sync_cout() << "Using normal datagen settings:"
                << "\n  threads: " << options.threads
                << "\n  output: " << options.out_file_name
                << "\n  search_node_limit: " << options.search_node_limit
                << "\n  min_random_plies: " << options.min_random_plies
                << "\n  max_random_plies: " << options.max_random_plies
                << "\n  win_adjudication_score: " << options.win_adjudication_score
                << "\n  win_adjudication_plies: " << options.win_adjudication_plies
                << "\n  min_positions_per_game: " << options.min_positions_per_game
                << "\n  max_positions_per_game: " << options.max_positions_per_game
                << "\n  exclude_checks: " << options.exclude_checks
                << "\n  exclude_mate_scores: " << options.exclude_mate_scores
                << "\n  exclude_last_move_captures: " << options.exclude_last_move_captures
                << sync_endl;
}

[[noreturn]] void thread_main(int thread_index, const NormalOptions& options) {
    ThreadContext ctx {};
    ctx.thread_index = thread_index;

    const auto out_file = output_file_name(options, thread_index);
    auto fstream = std::ofstream(out_file, std::ios_base::app);
    if (!fstream) {
        throw std::runtime_error("failed to open output file " + out_file);
    }

    auto start = Clock::now();
    auto total_data_points = ui64(0);
    auto unlogged_data_points = ui64(0);
    auto total_bytes = ui64(0);
    auto total_games = ui64(0);

    Searcher white_searcher {};
    Searcher black_searcher {};

    sync_cout(ctx) << "Starting and saving data to " << out_file << "." << sync_endl;

    while (true) {
        white_searcher.tt().new_search();
        black_searcher.tt().new_search();

        auto game = simulate_game(white_searcher, black_searcher, options);
        auto data = select_data_points(game, options);

        std::stringstream sstream;
        auto data_points = write_marlinflow(sstream, game, data);

        const auto data_str = sstream.str();
        fstream << data_str << std::flush;

        total_games++;
        total_bytes += data_str.size();
        total_data_points += data_points;
        unlogged_data_points += data_points;

        if (unlogged_data_points >= 1000) {
            unlogged_data_points = 0;

            auto elapsed_ms = delta_ms(Clock::now(), start);
            auto bytes_per_data = double(total_bytes) / double(total_data_points);
            auto data_per_sec = double(total_data_points) / (elapsed_ms / 1000.0);
            auto games_per_sec = double(total_games) / (elapsed_ms / 1000.0);

            sync_cout(ctx) << std::setprecision(2)
                           << total_data_points << " data points generated in "
                           << time_str(elapsed_ms) << " ("
                           << bytes_str(total_bytes) << ", "
                           << bytes_per_data << " bytes/data, "
                           << data_per_sec << " data/sec, "
                           << total_games << " games, "
                           << games_per_sec << " games/sec"
                           << ")." << sync_endl;
        }
    }
}

} // namespace

int run_normal_datagen(int argc, char* argv[]) {
    auto options = parse_args(argc, argv);

    std::cout << "Starting data generation with " << options.threads << " threads." << std::endl;
    log_configuration(options);

    std::vector<std::thread> helper_threads;
    helper_threads.reserve(std::max(0, options.threads - 1));
    for (auto i = 0; i < options.threads - 1; ++i) {
        helper_threads.emplace_back([=]() {
            thread_main(i + 1, options);
        });
    }

    thread_main(0, options);
}

} // namespace illumina
