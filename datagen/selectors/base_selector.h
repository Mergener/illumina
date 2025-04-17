#ifndef ILLUMINA_BASE_EXTRACTOR_H
#define ILLUMINA_BASE_EXTRACTOR_H

#include "../pipeline.h"

namespace illumina {

class BaseSelector : public DataSelector {
public:
    void load_settings(const nlohmann::json& j) override;
    std::vector<DataPoint> select(ThreadContext& ctx,
                                  const Game& game) override;

    BaseSelector() = default;
    ~BaseSelector() noexcept override;

private:
    int m_min_positions_per_game = 12;
    int m_max_positions_per_game = 16;
    bool m_exclude_checks = true;
    bool m_exclude_mate_scores = true;
    bool m_exclude_last_move_captures = true;
};

} // illumina

#endif // ILLUMINA_BASE_EXTRACTOR_H
