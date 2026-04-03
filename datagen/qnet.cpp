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

struct DataPoint {
    std::string fen;
    Score eval_score;
    Score qsearch_score;
};

struct QNetOptions {
    int threads = 1;
    std::string out_file_name {};
    ui64 search_node_limit = 2000;
    int min_random_plies = 12;
    int max_random_plies = 12;
    double extraction_ratio = 0.01;
};

QNetOptions parse_args(int argc, char* argv[]) {
    QNetOptions options {};

    argparse::ArgumentParser args(argv[0],
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

    args.add_argument("--extraction-ratio")
        .default_value(options.extraction_ratio)
        .store_into(options.extraction_ratio)
        .help("ratio of all possible data points that gets sampled into the final data.");

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

    return options;
}

std::string output_file_name(const QNetOptions& options, int thread_index) {
    if (thread_index == 0) {
        return options.out_file_name;
    }

    std::filesystem::path output_path(options.out_file_name);
    const std::filesystem::path extension = output_path.extension();

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

std::vector<DataPoint> simulate(Searcher& white_searcher,
                                Searcher& black_searcher,
                                const QNetOptions& options) {
    std::vector<DataPoint> all_data_points;

    struct {
        Searcher* searcher;
    } players[CL_COUNT] = {
        { &white_searcher },
        { &black_searcher }
    };

    std::random_device rnd;
    std::mt19937 gen(rnd());
    std::uniform_real_distribution<double> dist(0.0, 1.0);

    SearchSettings search_settings;
    search_settings.max_nodes = options.search_node_limit;
    search_settings.move_time = 10000;

    Board board = Board::standard_startpos();

    bool done_creating_startpos = false;
    int n_random_plies = random(options.min_random_plies, options.max_random_plies + 1);
    while (!done_creating_startpos) {
        for (int i = 0; i < n_random_plies; ++i) {
            Move legal_moves[MAX_GENERATED_MOVES];
            Move* begin = legal_moves;
            Move* end = generate_moves(board, begin);
            size_t n_moves = end - begin;

            if (n_moves == 0) {
                board = Board::standard_startpos();
                i = -1;
                continue;
            }

            Move move = legal_moves[random(size_t(0), n_moves)];
            board.make_move(move);
        }

        auto& player = players[board.color_to_move()];

        SearchSettings validation_search_settings = search_settings;
        validation_search_settings.max_nodes = UINT64_MAX;
        validation_search_settings.max_depth = 2;

        SearchResults search_results = player.searcher->search(board, validation_search_settings);

        if (std::abs(search_results.score) >= 200) {
            board = Board::standard_startpos();
            continue;
        }

        done_creating_startpos = true;
    }

    search_settings.post_qsearch_handler = [&](const Board& board, Score static_eval, Score qsearch_score) {
        auto r = dist(gen);
        if (r < options.extraction_ratio) {
            auto score_sign = board.color_to_move() == CL_WHITE ? 1 : -1;
            all_data_points.push_back(DataPoint {
                board.fen(),
                static_eval * score_sign,
                qsearch_score * score_sign
            });
        }
    };

    BoardResult result = board.result();
    while (!result.is_finished()) {
        auto& player = players[board.color_to_move()];

        SearchResults search_results = player.searcher->search(board, search_settings);
        Move best_move = search_results.best_move;

        board.make_move(best_move);
        result = board.result();
    }

    return all_data_points;
}

std::string bytes_str(ui64 bytes) {
    constexpr ui64 KiB = 1024ull;
    constexpr ui64 MiB = KiB * 1024ull;
    constexpr ui64 GiB = MiB * 1024ull;
    constexpr ui64 TiB = GiB * 1024ull;

    if (bytes < KiB) {
        return std::to_string(bytes) + " B";
    }

    std::string unit;
    ui64 divisor = KiB;

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
    constexpr ui64 one_minute = 60;
    constexpr ui64 one_hour = one_minute * 60;

    ui64 elapsed_seconds = elapsed_ms / 1000;

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

void log_configuration(const QNetOptions& options) {
    sync_cout() << "Using qnet datagen settings:"
                << "\n  threads: " << options.threads
                << "\n  output: " << options.out_file_name
                << "\n  search_node_limit: " << options.search_node_limit
                << "\n  min_random_plies: " << options.min_random_plies
                << "\n  max_random_plies: " << options.max_random_plies
                << sync_endl;
}

ui64 write_datapoints(std::ostream& stream,
                      const std::vector<DataPoint>& extracted_data) {
    for (const DataPoint& data : extracted_data) {
        stream << data.fen << " | "
               << data.eval_score << " | "
               << data.qsearch_score << '\n';
    }

    return extracted_data.size();
}

[[noreturn]] void thread_main(int thread_index, const QNetOptions& options) {
    ThreadContext ctx {};
    ctx.thread_index = thread_index;

    const std::string out_file = output_file_name(options, thread_index);
    std::ofstream fstream(out_file, std::ios_base::app);
    if (!fstream) {
        throw std::runtime_error("failed to open output file " + out_file);
    }

    TimePoint start = Clock::now();
    ui64 total_data_points = 0;
    ui64 unlogged_data_points = 0;
    ui64 total_bytes = 0;
    ui64 total_games = 0;

    Searcher white_searcher {};
    Searcher black_searcher {};

    sync_cout(ctx) << "Starting and saving data to " << out_file << "." << sync_endl;

    while (true) {
        white_searcher.tt().new_search();
        black_searcher.tt().new_search();

        auto data = simulate(white_searcher, black_searcher, options);

        std::stringstream sstream;
        ui64 data_points = write_datapoints(sstream, data);

        const std::string data_str = sstream.str();
        fstream << data_str << std::flush;

        total_games++;
        total_bytes += data_str.size();
        total_data_points += data_points;
        unlogged_data_points += data_points;

        if (unlogged_data_points >= 1000) {
            unlogged_data_points = 0;

            ui64 elapsed_ms = delta_ms(Clock::now(), start);
            double bytes_per_data = double(total_bytes) / double(total_data_points);
            double data_per_sec = double(total_data_points) / (elapsed_ms / 1000.0);
            double games_per_sec = double(total_games) / (elapsed_ms / 1000.0);

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

int run_qnet_datagen(int argc, char* argv[]) {
    QNetOptions options = parse_args(argc, argv);

    std::cout << "Starting data generation with " << options.threads << " threads." << std::endl;
    log_configuration(options);

    std::vector<std::thread> helper_threads;
    helper_threads.reserve(std::max(0, options.threads - 1));
    for (int i = 0; i < options.threads - 1; ++i) {
        helper_threads.emplace_back([=]() {
            thread_main(i + 1, options);
        });
    }

    thread_main(0, options);
}

} // illumina
