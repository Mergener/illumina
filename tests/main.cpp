#include "litetest.h"
#include "illumina.h"

int main(int argc, char* argv[]) {
    illumina::init();
    return litetest::litetest_main(argc, argv);
}