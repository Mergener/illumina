#include <atomic>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <argparse/argparse.hpp>

#include <illumina.h>

#include "logger.h"

namespace illumina {

namespace {

namespace fs = std::filesystem;

struct RelabelOptions {
    fs::path input_path = "data";
    fs::path output_path = "out_data";
    int threads = 6;
    ui64 search_node_limit = 10000;
    double complete_output_size_ratio = 0.97;
};

struct RelabelTask {
    fs::path input_path;
    fs::path output_path;
};

std::string trim(std::string value) {
    const size_t begin = value.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return {};
    }
    const size_t end = value.find_last_not_of(" \t\r\n");
    return value.substr(begin, end - begin + 1);
}

RelabelOptions parse_args(int argc, char* argv[]) {
    RelabelOptions options {};
    std::string input_path = options.input_path.string();
    std::string output_path = options.output_path.string();

    argparse::ArgumentParser args(argv[0],
                                  "",
                                  argparse::default_arguments::none);

    args.add_argument("-h", "--help")
        .action([&args](const auto&) {
            std::cout << args;
            std::exit(EXIT_SUCCESS);
        })
        .default_value(false)
        .implicit_value(true)
        .nargs(0)
        .help("shows help message and exits");

    args.add_argument("-i", "--input")
        .default_value(input_path)
        .store_into(input_path)
        .help("input file or directory (directories are processed recursively).");

    args.add_argument("-o", "--output")
        .default_value(output_path)
        .store_into(output_path)
        .help("output file or directory.");

    args.add_argument("-t", "--threads")
        .default_value(options.threads)
        .store_into(options.threads)
        .help("number of files to relabel concurrently.");

    args.add_argument("--search-node-limit")
        .default_value(options.search_node_limit)
        .store_into(options.search_node_limit)
        .help("node limit for each position search.");

    args.add_argument("--complete-output-size-ratio")
        .default_value(options.complete_output_size_ratio)
        .store_into(options.complete_output_size_ratio)
        .help("skip outputs at least this large relative to their input.");

    try {
        args.parse_args(argc, argv);
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << '\n' << args;
        std::exit(EXIT_FAILURE);
    }

    if (options.threads < 1) {
        throw std::invalid_argument("threads must be at least 1");
    }
    if (options.search_node_limit < 1) {
        throw std::invalid_argument("search-node-limit must be at least 1");
    }
    if (options.complete_output_size_ratio < 0.0) {
        throw std::invalid_argument("complete-output-size-ratio must be non-negative");
    }
    options.input_path = input_path;
    options.output_path = output_path;
    return options;
}

bool output_is_complete(const RelabelTask& task, double ratio) {
    std::error_code error;
    if (!fs::is_regular_file(task.output_path, error)) {
        return false;
    }

    const uintmax_t input_size = fs::file_size(task.input_path);
    const uintmax_t output_size = fs::file_size(task.output_path);
    return double(output_size) >= double(input_size) * ratio;
}

std::vector<RelabelTask> collect_tasks(const RelabelOptions& options) {
    if (!fs::exists(options.input_path)) {
        throw std::invalid_argument("input path does not exist: " + options.input_path.string());
    }

    std::vector<RelabelTask> tasks;
    if (fs::is_regular_file(options.input_path)) {
        RelabelTask task { options.input_path, options.output_path };
        if (!output_is_complete(task, options.complete_output_size_ratio)) {
            tasks.push_back(std::move(task));
        }
        return tasks;
    }
    if (!fs::is_directory(options.input_path)) {
        throw std::invalid_argument("input path is neither a file nor a directory: "
                                    + options.input_path.string());
    }

    for (const fs::directory_entry& entry :
         fs::recursive_directory_iterator(options.input_path)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        RelabelTask task {
            entry.path(),
            options.output_path / fs::relative(entry.path(), options.input_path)
        };
        if (!output_is_complete(task, options.complete_output_size_ratio)) {
            tasks.push_back(std::move(task));
        }
    }
    return tasks;
}

fs::path contradiction_path(const fs::path& output_path) {
    return output_path.parent_path()
         / (output_path.stem().string() + "-contradiction" + output_path.extension().string());
}

bool is_contradiction(Score normalized_score, const std::string& wdl) {
    if (wdl == "0.5") {
        return std::abs(normalized_score) > 100;
    }
    if (wdl == "1.0") {
        return normalized_score < 50;
    }
    return normalized_score > -50;
}

Score parse_score(const std::string& value) {
    size_t parsed = 0;
    const long score = std::stol(value, &parsed);
    if (parsed != value.size()) {
        throw std::invalid_argument("invalid score '" + value + "'");
    }
    return Score(score);
}

void parse_data_line(const std::string& line,
                     std::string& fen,
                     Score& previous_score,
                     std::string& wdl) {
    constexpr std::string_view delimiter = " | ";
    const size_t first = line.find(delimiter);
    const size_t second = first == std::string::npos
                        ? std::string::npos
                        : line.find(delimiter, first + delimiter.size());
    if (first == std::string::npos
        || second == std::string::npos
        || line.find(delimiter, second + delimiter.size()) != std::string::npos) {
        throw std::invalid_argument("expected 'FEN | score | WDL'");
    }

    fen = line.substr(0, first);
    previous_score = parse_score(line.substr(first + delimiter.size(),
                                             second - first - delimiter.size()));
    wdl = line.substr(second + delimiter.size());
}

Score relabel_score(Searcher& searcher, const Board& board, ui64 node_limit) {
    searcher.tt().clear();

    SearchSettings settings {};
    settings.max_nodes = node_limit;
    settings.move_time = 10000;

    Score score = searcher.search(board, settings).score;
    if (board.color_to_move() == CL_BLACK) {
        score = -score;
    }

    if (is_mate_score(score)) {
        score = score < 0 ? -KNOWN_WIN : KNOWN_WIN;
    }
    return score;
}

void relabel_file(const RelabelTask& task,
                  const RelabelOptions& options,
                  int thread_index) {
    ThreadContext context { thread_index };
    sync_cout(context) << "Starting " << task.input_path << " -> "
                       << task.output_path << sync_endl;

    if (!task.output_path.parent_path().empty()) {
        fs::create_directories(task.output_path.parent_path());
    }

    std::ifstream input(task.input_path);
    if (!input) {
        throw std::runtime_error("failed to open input file " + task.input_path.string());
    }

    size_t completed_lines = 0;
    {
        std::ifstream existing_output(task.output_path);
        std::string line;
        while (std::getline(existing_output, line)) {
            completed_lines++;
        }
    }

    std::string line;
    for (size_t i = 0; i < completed_lines && std::getline(input, line); ++i) {
    }

    std::ofstream output(task.output_path, std::ios_base::app);
    std::ofstream contradictions(contradiction_path(task.output_path), std::ios_base::app);
    if (!output || !contradictions) {
        throw std::runtime_error("failed to open outputs for " + task.input_path.string());
    }

    Searcher searcher { TranspositionTable(16 * 1024 * 1024) };
    size_t line_number = completed_lines;
    while (std::getline(input, line)) {
        line_number++;
        line = trim(std::move(line));
        if (line.empty()) {
            continue;
        }

        try {
            std::string fen;
            std::string wdl;
            Score previous_score;
            parse_data_line(line, fen, previous_score, wdl);

            const Board board(fen);
            const Score new_score = relabel_score(searcher,
                                                  board,
                                                  options.search_node_limit);
            output << fen << " | " << new_score << " | " << wdl << '\n' << std::flush;

            const Score normalized = normalize_score(new_score, board);
            if (is_contradiction(normalized, wdl)) {
                contradictions << fen
                               << "\n\tprev: " << previous_score
                               << " (" << normalize_score(previous_score, board) << " normalized)"
                               << "\n\tnew: " << new_score
                               << " (" << normalized << " normalized)"
                               << "\n\twdl: " << wdl << "\n\n"
                               << std::flush;
            }
        }
        catch (const std::exception& e) {
            sync_cout(context) << "Error processing line " << line_number
                               << " of " << task.input_path << ": " << e.what()
                               << sync_endl;
        }
    }

    sync_cout(context) << "Done processing " << task.input_path << "." << sync_endl;
}

void log_configuration(const RelabelOptions& options, size_t task_count) {
    sync_cout() << "Using relabel settings:"
                << "\n  input: " << options.input_path
                << "\n  output: " << options.output_path
                << "\n  threads: " << options.threads
                << "\n  search_node_limit: " << options.search_node_limit
                << "\n  complete_output_size_ratio: " << options.complete_output_size_ratio
                << "\n  files_to_process: " << task_count
                << sync_endl;
}

} // namespace

int run_relabel_datagen(int argc, char* argv[]) {
    const RelabelOptions options = parse_args(argc, argv);
    const std::vector<RelabelTask> tasks = collect_tasks(options);
    log_configuration(options, tasks.size());

    std::atomic_size_t next_task { 0 };
    std::atomic_bool failed { false };
    const size_t worker_count = std::min(tasks.size(), size_t(options.threads));
    std::vector<std::thread> workers;
    workers.reserve(worker_count);

    for (size_t worker = 0; worker < worker_count; ++worker) {
        workers.emplace_back([&, worker]() {
            while (true) {
                const size_t task_index = next_task.fetch_add(1);
                if (task_index >= tasks.size()) {
                    return;
                }
                try {
                    relabel_file(tasks[task_index], options, int(worker));
                }
                catch (const std::exception& e) {
                    failed = true;
                    ThreadContext context { int(worker) };
                    sync_cout(context) << "Error processing " << tasks[task_index].input_path
                                       << ": " << e.what() << sync_endl;
                }
            }
        });
    }

    for (std::thread& worker : workers) {
        worker.join();
    }

    return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}

} // illumina
