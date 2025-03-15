#ifndef ILLUMINA_DATAGEN_TYPES_H
#define ILLUMINA_DATAGEN_TYPES_H

#include <string>
#include <random>

#include <illumina.h>

#include "simulation.h"

namespace illumina {

struct DatagenOptions {
    int threads = 1;
    std::string out_file_name {};
    std::string pipeline_file_path {};
};

struct ThreadContext {
    int thread_index;

    bool is_main_thread() const;
};

inline bool ThreadContext::is_main_thread() const {
    return thread_index == 0;
}

/**
 * Data extracted from a DataExtractor.
 */
struct DataPoint {
    std::string fen;
    GamePlyData ply_data;
};

} // illumina

#endif // ILLUMINA_DATAGEN_TYPES_H
