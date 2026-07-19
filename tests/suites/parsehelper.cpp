#include "../litetest/litetest.h"

#include "parsehelper.h"

using namespace litetest;
using namespace illumina;

TEST_SUITE(ParseHelper);

TEST_CASE(ReadChunk) {
    auto text1 = std::string("   rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b\tKQkq e3       0 1");
    auto parse_helper = ParseHelper(text1);

    for (auto i = 0; i < 2; ++i) {
        EXPECT(parse_helper.finished()).to_be(false);
        EXPECT(parse_helper.read_chunk()).to_be("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR");
        EXPECT(parse_helper.read_chunk()).to_be("b");
        EXPECT(parse_helper.read_chunk()).to_be("KQkq");
        EXPECT(parse_helper.read_chunk()).to_be("e3");
        EXPECT(parse_helper.read_chunk()).to_be("0");
        EXPECT(parse_helper.finished()).to_be(false);
        EXPECT(parse_helper.read_chunk()).to_be("1");
        EXPECT(parse_helper.finished()).to_be(true);
        EXPECT(parse_helper.read_chunk()).to_be("");
        EXPECT(parse_helper.finished()).to_be(true);
        parse_helper.rewind_all();
    }
}
