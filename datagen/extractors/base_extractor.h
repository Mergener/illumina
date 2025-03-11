#ifndef ILLUMINA_BASE_EXTRACTOR_H
#define ILLUMINA_BASE_EXTRACTOR_H

#include "../datagen_types.h"

namespace illumina {

class BaseExtractor : public DataExtractor {
public:
    std::vector<ExtractedData> extract_data(ThreadContext& ctx,
                                            const Game& game) override;

    BaseExtractor() = default;
    BaseExtractor(int min_positions_per_game,
                  int max_positions_per_game);
    ~BaseExtractor() noexcept override;

private:
    int m_min_positions_per_game = 12;
    int m_max_positions_per_game = 16;
};

} // illumina

#endif // ILLUMINA_BASE_EXTRACTOR_H
