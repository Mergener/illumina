#include "../litetest/litetest.h"

#include "search.h"

using namespace illumina;

TEST_SUITE(Search);

TEST_CASE(ExpectedNodes) {
    Searcher searcher;

    Board board = Board::standard_startpos();

    SearchResults results_before;
    SearchResults results_after;

    SearchSettings settings;
    settings.max_depth = 10;

    searcher.new_game();

    results_before = searcher.search(board, settings);
    searcher.new_game();
    results_after = searcher.search(board, settings);

    EXPECT(results_after.best_move).to_be(results_before.best_move);
    EXPECT(results_after.ponder_move).to_be(results_before.ponder_move);
    EXPECT(results_after.score).to_be(results_before.score);
    EXPECT(results_after.total_nodes).to_be(results_before.total_nodes);
}