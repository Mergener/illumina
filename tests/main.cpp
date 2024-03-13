#include <iostream>

#include "litetest.h"
#include "illumina.h"

int main(int argc, char* argv[]) {
    illumina::init();
    std::cout << std::boolalpha;
    return litetest::litetest_main(argc, argv);
}