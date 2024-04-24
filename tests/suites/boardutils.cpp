#include "../litetest/litetest.h"

#include "boardutils.h"

using namespace litetest;
using namespace illumina;

TEST_SUITE(BoardUtils);

TEST_CASE(HasGoodSee) {
    struct {
        std::string board_fen;
        Square source;
        Square dest;
        int threshold;
        bool expected_output;

        void run() const {
            Board board(board_fen);
            EXPECT(has_good_see(board, source, dest, threshold)).to_be(expected_output);
        }
    } tests[] = {
        { "rnbqk1nr/pppp1ppp/3b4/4p3/3P4/5N2/PPP1PPPP/RNBQKB1R w KQkq - 0 1", SQ_D4, SQ_E5, 0, true },
        { "rnbqk1nr/pppp1ppp/3b4/4p3/3P4/5N2/PPP1PPPP/RNBQKB1R w KQkq - 0 1", SQ_F3, SQ_E5, 0, true },
        { "rnbqkbnr/ppp1pppp/8/8/3P4/8/PPP1PPPP/RNBQKBNR b KQkq - 0 1", SQ_D8, SQ_D4, 0, false },
        { "8/8/5q2/4p3/8/2B5/1B5k/1K6 w - - 0 1", SQ_C3, SQ_E5, 0, true },
        { "8/8/1r3q2/4p3/8/2B5/1B5k/1K6 w - - 0 1", SQ_C3, SQ_E5, 0, false },
        { "8/8/5q2/4p3/K7/2B5/7k/8 w - - 0 1", SQ_C3, SQ_E5, 0, false }
    };

    for (auto& test: tests) {
        test.run();
    }
}