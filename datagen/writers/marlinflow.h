#ifndef ILLUMINA_MARLINFLOW_H
#define ILLUMINA_MARLINFLOW_H

#include "../datagen_types.h"

namespace illumina {

class MarlinflowDataWriter : public DataWriter {
public:
    ui64 write_data(ThreadContext& ctx,
                    std::ostream& stream,
                    const Game& game,
                    const std::vector<ExtractedData>& extracted_data) override;

    ~MarlinflowDataWriter() noexcept override;
};

} // illumina

#endif // ILLUMINA_MARLINFLOW_H
