#ifndef ILLUMINA_MFLOWGEN_H
#define ILLUMINA_MFLOWGEN_H

#include <fstream>

#include "generator.h"

namespace illumina {

class MarlinformatDataGenerator : DataGenerator {
public:
    void start(const std::string& out_file_path) override;
    void finish() override;

    ~MarlinformatDataGenerator() = default;
    
private:
    std::ofstream m_file_stream;
};

} // illumina

#endif // ILLUMINA_MFLOWGEN_H
