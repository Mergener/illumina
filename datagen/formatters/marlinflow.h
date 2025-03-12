#ifndef ILLUMINA_MARLINFLOW_H
#define ILLUMINA_MARLINFLOW_H

#include "../datagen_types.h"

namespace illumina {

class MarlinflowFormatter : public DataFormatter {
public:
    ui64 write_data(ThreadContext& ctx,
                    std::ostream& stream,
                    const Game& game,
                    const std::vector<DataPoint>& extracted_data) override;

    ~MarlinflowFormatter() noexcept override;
};

} // illumina

#endif // ILLUMINA_MARLINFLOW_H
