#include <iostream>
#include <sstream>
#include <random>
#include <string_view>
#include <string>
#include <fstream>
#include <algorithm>
#include <thread>

#include <illumina.h>

#include <argparse/argparse.hpp>

#include "datagen_types.h"
#include "logger.h"
#include "selectors/base_selector.h"
#include "formatters/marlinflow.h"

namespace illumina {

static DatagenOptions parse_args(int argc, char* argv[]) {
    DatagenOptions options {};

    argparse::ArgumentParser args(argv[0], ILLUMINA_VERSION_NAME);

    args.add_argument("-t", "--threads")
        .default_value(options.threads)
        .store_into(options.threads)
        .help("number of threads.");

    args.add_argument("-f", "--filename")
        .required()
        .store_into(options.out_file_name)
        .help("name of the output file. Extension will be '.txt'.");

    args.add_argument("--pipeline")
        .default_value("")
        .store_into(options.pipeline_file_path)
        .help("path to a datagen pipeline file.");

    try {
        args.parse_args(argc, argv);
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        std::cerr << args;
        std::exit(EXIT_FAILURE);
    }

    return options;
}

[[noreturn]] static void thread_main(int thread_index,
                                     const DatagenOptions& datagen_options) {
    ThreadContext ctx {};
    ctx.thread_index = thread_index;

    // Each thread will write to its own output file.
    // We want the main thread to match the file name
    // specified by the user.
    std::string out_file = datagen_options.out_file_name;
    if (thread_index != 0) {
        out_file += "_" + std::to_string(thread_index);
    }
    out_file += ".txt";
    sync_cout(ctx) << "Starting and saving data to " << out_file << "." << sync_endl;
    std::ofstream fstream(out_file, std::ios_base::app);

    // Record/prepare our data generation state before we
    // jump into the processing state.
    TimePoint start = Clock::now();
    ui64 total_data_points    = 0;
    ui64 unlogged_data_points = 0;
    ui64 total_bytes          = 0;
    ui64 total_games          = 0;

    // Load our pipeline. If the user has specified no pipeline, we
    // will automatically use the default one.
    std::string pipeline_json {};
    const std::string& pipeline_path = datagen_options.pipeline_file_path;
    if (!pipeline_path.empty()) {
        std::ifstream pipeline_stream(pipeline_path);

        if (!pipeline_stream.fail()) {
            // We found a pipeline file. Load it entirely.
            sync_cout() << "Found pipeline definition, loading it." << sync_endl;
            pipeline_json = std::string(std::istreambuf_iterator<char>(pipeline_stream),
                                        std::istreambuf_iterator<char>());
        }
        else if (ctx.is_main_thread()) {
            sync_cout() << "Couldn't find pipeline definition at " << pipeline_path << sync_endl;
            sync_cout() << "Using default pipeline definition."    << sync_endl;
        }
    }
    Pipeline pipeline(pipeline_json);

    Searcher white_searcher {};
    Searcher black_searcher {};

    while (true) {
        // Make sure every new game has a fresh TT.
        white_searcher.tt().new_search();
        black_searcher.tt().new_search();

        // Step 1. Simulate the game.
        Game game = simulate(white_searcher,
                             black_searcher);

        // Step 2. Filter out undesired data extracted from the game.
        std::vector<DataPoint> data = pipeline.get_selector()
                                              .select(ctx, game);

        // Step 3. Format the data.
        // We write the data to a string stream before we
        // output it so that we can keep track of the amount
        // of bytes we're sending.
        std::stringstream sstream;
        ui64 data_points = pipeline.get_formatter().write(ctx, sstream, game, data);

        // Step 4. Write the data on disk.
        std::string data_str = sstream.str();
        fstream << data_str << std::flush;

        // The following part is solely related to reporting our results
        // to the user.

        // Keep track of datagen statistics.
        total_games++;
        total_bytes          += data_str.size();
        total_data_points    += data_points;
        unlogged_data_points += data_points;

        // We don't want to spam stdout too much, so only log information
        // after reasonable intervals.
        if (unlogged_data_points >= 1000) {
            unlogged_data_points = 0;

            static auto bytes_str = [](ui64 bytes) {
                constexpr ui64 KiB =       1024ull;
                constexpr ui64 MiB = KiB * 1024ull;
                constexpr ui64 GiB = MiB * 1024ull;
                constexpr ui64 TiB = GiB * 1024ull;

                if (bytes < KiB) {
                    return std::to_string(bytes) + " B";
                }

                std::string unit;
                ui64 divisor;

                if (bytes < MiB) {
                    divisor = KiB;
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
            };

            static auto time_str = [](ui64 elapsed_ms) {
                constexpr ui64 ONE_MINUTE = 60;
                constexpr ui64 ONE_HOUR   = ONE_MINUTE * 60;

                ui64 elapsed_seconds = elapsed_ms / 1000;

                if (elapsed_seconds < ONE_MINUTE) {
                    return std::to_string(elapsed_seconds) + "s";
                }
                if (elapsed_seconds < ONE_HOUR) {
                    return std::to_string(elapsed_seconds / ONE_MINUTE) + "m"
                         + std::to_string(elapsed_seconds % ONE_MINUTE) + "s";
                }
                return std::to_string(elapsed_seconds / ONE_HOUR) + "h"
                     + std::to_string((elapsed_seconds % ONE_HOUR) / ONE_MINUTE) + "m"
                     + std::to_string(elapsed_seconds % ONE_MINUTE);
            };

            ui64 elapsed_ms       = delta_ms(Clock::now(), start);
            double bytes_per_data = double(total_bytes) / double(total_data_points);
            double data_per_sec   = double(total_data_points) / (elapsed_ms / 1000.0);
            double games_per_sec  = double(total_games) / (elapsed_ms / 1000.0);

            sync_cout(ctx) << std::setprecision(2)
                           << total_data_points      << " data points generated in "
                           << time_str(elapsed_ms)   << " ("
                           << bytes_str(total_bytes) << ", "
                           << bytes_per_data         << " bytes/data, "
                           << data_per_sec           << " data/sec, "
                           << total_games            << " games, "
                           << games_per_sec          << " games/sec"
                           << ")." << sync_endl;
        }
    }
}

static int datagen_main(int argc, char* argv[]) {
    // Parse command line args.
    DatagenOptions options = parse_args(argc, argv);

    std::cout << "Starting data generation with " << options.threads << " threads." << std::endl;

    // Fire helper threads.
    std::vector<std::thread> helper_threads;
    for (int i = 0; i < options.threads - 1; ++i) {
        helper_threads.emplace_back([=]() {
            thread_main(i + 1, options);
        });
    }

    // Run main thread.
    thread_main(0, options);
}

void run_bench() {
    std::cout << "Running bench..." << std::endl;
    BenchResults res = bench();
    std::cout << "Finished bench.\n"
              << "\tNodes: " << res.total_nodes
              << "\tNPS:   " << res.nps
              << std::endl;
}

}

int main(int argc, char* argv[]) {
    std::ios_base::sync_with_stdio(false);

    try {
        illumina::init();
        illumina::run_bench();
        return illumina::datagen_main(argc, argv);
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal:\n" << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}