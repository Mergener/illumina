#include <doctest/doctest.h>

#include "parsehelper.h"

using namespace illumina;

TEST_SUITE_BEGIN("ParseHelper");

TEST_CASE("ReadChunk") {
    std::string text1 = "   rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b\tKQkq e3       0 1";
    ParseHelper parse_helper(text1);

    for (int i = 0; i < 2; ++i) {
        REQUIRE_EQ(parse_helper.finished(), false);
        REQUIRE_EQ(parse_helper.read_chunk(), "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR");
        REQUIRE_EQ(parse_helper.read_chunk(), "b");
        REQUIRE_EQ(parse_helper.read_chunk(), "KQkq");
        REQUIRE_EQ(parse_helper.read_chunk(), "e3");
        REQUIRE_EQ(parse_helper.read_chunk(), "0");
        REQUIRE_EQ(parse_helper.finished(), false);
        REQUIRE_EQ(parse_helper.read_chunk(), "1");
        REQUIRE_EQ(parse_helper.finished(), true);
        REQUIRE_EQ(parse_helper.read_chunk(), "");
        REQUIRE_EQ(parse_helper.finished(), true);
        parse_helper.rewind_all();
    }
}

TEST_SUITE_END;
