#ifndef ILLUMINA_GENERATOR_H
#define ILLUMINA_GENERATOR_H

#include <string>

#include "simulation.h"

namespace illumina {

class DataGenerator : public SimulationListener {
public:
    virtual void start(const std::string& out_file_path) = 0;
    virtual void finish() = 0;

    virtual ~DataGenerator() = default;
};

} // illumina

#endif // ILLUMINA_GENERATOR_H
