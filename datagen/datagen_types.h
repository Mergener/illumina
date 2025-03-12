#ifndef ILLUMINA_DATAGEN_TYPES_H
#define ILLUMINA_DATAGEN_TYPES_H

#include <string>
#include <random>

#include <illumina.h>

#include "simulation.h"

namespace illumina {

struct DatagenOptions {
    int threads = 1;
    std::string out_file_name;
};

struct ThreadContext {
    int thread_index;
};

/**
 * Data extracted from a DataExtractor.
 */
struct DataPoint {
    std::string fen;
    GamePlyData ply_data;
};

/**
 * Nitpicks data from a simulation.
 */
class DataSelector {
public:
    virtual std::vector<DataPoint> select(ThreadContext& ctx,
                                          const Game& game) = 0;

    virtual ~DataSelector() = default;
};

/**
 * Writes data to an output stream.
 */
class DataFormatter {
public:
    virtual ui64 write_data(ThreadContext& ctx,
                            std::ostream& stream,
                            const Game& game,
                            const std::vector<DataPoint>& extracted_data) = 0;

    virtual ~DataFormatter() = default;
};

} // illumina

#endif // ILLUMINA_DATAGEN_TYPES_H
