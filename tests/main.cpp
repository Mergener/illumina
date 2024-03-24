#include <iostream>

#include "litetest.h"
#include "illumina.h"

int main(int argc, char* argv[]) {
    illumina::init();

    std::ios::sync_with_stdio(false);
    std::cin.tie();
    std::cout << std::boolalpha;
    return litetest::litetest_main(argc, argv);
}