#include "../litetest/litetest.h"

#include <iostream>

#include "types.h"

using namespace litetest;
using namespace illumina;

TEST_SUITE(Types);

SUITE_SETUP(Types) {
    init_types();
}