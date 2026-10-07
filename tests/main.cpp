#include <iostream>

#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>

#include "illumina.h"

int main(int argc, char* argv[]) {
    illumina::init();

    std::ios::sync_with_stdio(false);
    std::cin.tie();
    std::cout << std::boolalpha;

    doctest::Context context(argc, argv);
    return context.run();
}
