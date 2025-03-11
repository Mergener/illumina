#include <iostream>
#include <sstream>
#include <random>
#include <string_view>
#include <string>
#include <fstream>
#include <algorithm>
#include <thread>

#include <illumina.h>

#include "datagen_types.h"

#include "extractors/base_extractor.h"
#include "writers/marlinflow.h"

namespace illumina {

[[noreturn]] void thread_main(std::string out_file) {
    ThreadContext ctx {};
    BaseExtractor extractor {};
    MarlinflowDataWriter writer {};

    std::cout << "Thread "
              << std::this_thread::get_id()
              << " saving to file "
              << out_file << std::endl;

    TimePoint start = Clock::now();
    ui64 total_data_points = 0;
    ui64 unlogged_data_points = 0;

    std::ofstream fstream(out_file);

    while (true) {
        Game game = simulate();
        std::vector<ExtractedData> data = extractor.extract_data(ctx, game);

        std::stringstream sstream;
        ui64 data_points = writer.write_data(ctx, sstream, game, data);
        total_data_points += data_points;
        unlogged_data_points += data_points;

        if (unlogged_data_points >= 1000) {
            unlogged_data_points = 0;

            
        }
    }
}

}

int main(int argc, char* argv[]) {
    std::ios_base::sync_with_stdio(false);

    std::string out_file_name;

    if (argc < 2) {
        std::cout << "Enter an output file name: " << std::endl;
        std::getline(std::cin, out_file_name);

        out_file_name.erase(std::remove(out_file_name.begin(), out_file_name.end(), '"'), out_file_name.end());
        out_file_name.erase(std::remove(out_file_name.begin(), out_file_name.end(), '\n'), out_file_name.end());
    }
    else {
        out_file_name = argv[1];
    }

    illumina::init();

    illumina::thread_main(out_file_name + ".txt");
}