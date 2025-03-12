#ifndef ILLUMINA_BASE_EXTRACTOR_H
#define ILLUMINA_BASE_EXTRACTOR_H

#include "../datagen_types.h"

namespace illumina {

class BaseSelector : public DataSelector {
public:
    std::vector<DataPoint> select(ThreadContext& ctx,
                                  const Game& game) override;

    BaseSelector() = default;
    BaseSelector(int min_positions_per_game,
                 int max_positions_per_game);
    ~BaseSelector() noexcept override;

private:
    int m_min_positions_per_game = 12;
    int m_max_positions_per_game = 16;
};

} // illumina

#endif // ILLUMINA_BASE_EXTRACTOR_H
