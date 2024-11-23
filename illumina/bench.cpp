#include "bench.h"

namespace illumina {

BenchSettings default_bench_settings() {
    BenchSettings settings;

    settings.hash_size_mb = DEFAULT_BENCH_HASH_SIZE_MB;
    settings.search_settings.max_depth = DEFAULT_BENCH_DEPTH;

    settings.boards = {
        Board::standard_startpos(),
        Board("r1b1kb1r/1p3pp1/p1p2n1p/8/3qp1P1/PPN1P2P/3P1PB1/R2QK1NR b KQkq - 0 13"),
        Board("r7/1p3p1k/2p5/p5Q1/8/5B2/q3bKP1/8 w - - 2 34"),
        Board("6R1/8/8/8/4r3/8/3K1p2/5k2 w - - 18 78"),
        Board("r2qk1nr/ppp1p1bp/2n1b3/3p1pp1/2PP4/P1N3P1/RP2PP1P/2BQKBNR w Kkq - 3 7"),
        Board("8/8/8/R4pk1/1r6/5K2/8/8 b - - 9 57"),
        Board("1rb5/8/p1pqNk2/3p2pp/1P1P4/2P4B/2Q1RPKb/8 w - - 0 40"),
        Board("rnbq2nr/1pp2kb1/7p/p2pp1p1/P1P3P1/1P2P3/3PBP2/RNBQK1NR b KQ - 2 10"),
        Board("rn1qk2r/1b1p1pp1/p2Ppn2/1pP4p/1P6/P7/3NPPPP/R2QKBNR w KQk - 1 15"),
        Board("r2q2nr/4k3/pp2bNp1/4Q1P1/P4P1p/4B2P/1PP5/R3K2R w KQ - 2 27"),
        Board("rnbqkbnr/pp1p3p/8/2p1pp2/2P5/7N/PP1PPPPP/RNBQKBR1 b Qkq - 1 5"),
        Board("4N3/5k2/r3R3/2r5/8/6PP/3P1K2/8 w - - 2 46"),
        Board("6k1/1p1r1Nbp/2n3p1/p1P2p2/P7/1QP5/4P2P/2qNKR2 w - - 4 29"),
        Board("r3rnk1/pp3pb1/q7/2p4R/P2P4/1Pn3P1/4NP2/R1BQ1K2 w - - 3 27"),
        Board("1k2r2r/ppp2p1p/3p1np1/1q1p4/3P1b2/1PQ2PN1/PBP1P1PP/RN2K2R b KQ - 0 1")
    };

    return settings;
}

BenchResults bench(const BenchSettings& settings) {
    // Setup searcher.
    Searcher searcher;
    searcher.tt().resize(settings.hash_size_mb * 1024 * 1024);

    // Search every position.
    TimePoint before = Clock::now();
    BenchResults results;
    for (const Board& board: settings.boards) {
        searcher.new_game();

        TimePoint search_before = Clock::now();
        SearchResults search_results = searcher.search(board,
                                                       settings.search_settings);
        TimePoint search_after = Clock::now();

        results.search_time_ms += delta_ms(search_after, search_before);
        results.total_nodes += search_results.total_nodes;
        results.best_moves.push_back(search_results.best_move);

        if (settings.on_board_searched != nullptr) {
            settings.on_board_searched(board, search_results.score, search_results.best_move);
        }
    }
    TimePoint after = Clock::now();
    ui64 elapsed_ms = delta_ms(after, before);

    // Compute NPS and save final results to bench results object.
    results.bench_time_ms = elapsed_ms;
    results.nps = double(results.total_nodes) / (double(results.search_time_ms) / 1000.0);

    return results;
}

} // illumina