#include "../litetest/litetest.h"

#include "utils.h"
#include "types.h"

using namespace litetest;
using namespace illumina;

TEST_SUITE(Utils);

TEST_CASE(TryParseInt) {
    struct {
        std::string str;
        int base;
        bool expect_success;
        i64 expected_value = 0; // Ignored if expect_success == false

        void run() {
            i64 result;
            bool success = try_parse_int(str, result, base);
            EXPECT(success).to_be(expect_success);
            if (expect_success) {
                EXPECT(result).to_be(expected_value);
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
        test.run();
    }
}