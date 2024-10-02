#ifndef ILLUMINA_BENCH_H
#define ILLUMINA_BENCH_H

#include <vector>

#include "types.h"
#include "search.h"

namespace illumina {

static constexpr size_t DEFAULT_BENCH_HASH_SIZE_MB = 32;
static constexpr Depth  DEFAULT_BENCH_DEPTH = 14;

struct BenchSettings {
    SearchSettings search_settings;
    size_t hash_size_mb;
    std::vector<Board> boards;
    std::function<void(const Board& board, Score score, Move move)> on_board_searched = nullptr;
};

struct BenchResults {
    ui64 total_nodes {};
    ui64 bench_time_ms {};
    ui64 search_time_ms {};
    ui64 nps {};
    std::vector<Move> best_moves;
};

BenchSettings default_bench_settings();
BenchResults bench(const BenchSettings& settings = default_bench_settings());

} // illumina

#endif //ILLUMINA_BENCH_H
