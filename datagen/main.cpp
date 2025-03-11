#include <iostream>
#include <sstream>
#include <random>
#include <string_view>
#include <string>
#include <fstream>
#include <algorithm>

#include <illumina.h>

#include "datagen_types.h"

#include "extractors/base_extractor.h"
#include "writers/marlinflow.h"

namespace illumina {

void thread_main(std::string out_file) {
    ThreadContext ctx;
    BaseExtractor extractor;
    MarlinflowDataWriter writer;

    std::ofstream stream(out_file);

    while (true) {
        Game game = simulate();
        auto data = extractor.extract_data(ctx, game);
        writer.write_data(ctx, stream, game, data);
    }
}

}

int main(int argc, char* argv[]) {
    std::ios_base::sync_with_stdio(false);

    std::string out_file;

    if (argc < 2) {
        std::cout << "Enter an output file path: " << std::endl;
        std::getline(std::cin, out_file);

        out_file.erase(std::remove(out_file.begin(), out_file.end(), '"'), out_file.end());
        out_file.erase(std::remove(out_file.begin(), out_file.end(), '\n'), out_file.end());
    }
    else {
        out_file = argv[1];
    }

    illumina::init();

    illumina::thread_main(out_file);

    return 0;
}