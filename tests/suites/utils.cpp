#include <doctest/doctest.h>

#include "utils.h"
#include "types.h"

using namespace illumina;

TEST_SUITE_BEGIN("Utils");

TEST_CASE("TryParseInt") {
    struct {
        std::string str;
        int base;
        bool expect_success;
        i64 expected_value = 0; // Ignored if expect_success == false

        void run() {
            i64 result;
            bool success = try_parse_int(str, result, base);
            REQUIRE_EQ(success, expect_success);
            if (expect_success) {
                REQUIRE_EQ(result, expected_value);
            }
        }
    } tests[] = {
        { "", 10, false },
        { "2f", 10, false },
        { "45", 5, false },
        { "123", 10, true, 123 },
        { "2f", 16, true, 0x2f },
        { "45", 8, true, 045 },
        { "1101", 2, true, 0b1101 },
        { "298746", 10, true, 298746 },
        { "-4800", 10, true, -4800 },
        { "-4523", 16, true, -0x4523 },
    };

    for (auto& test: tests) {
        CAPTURE(test.str);
        CAPTURE(test.base);
        CAPTURE(test.expect_success);
        CAPTURE(test.expected_value);
        test.run();
    }
}

TEST_SUITE_END;
