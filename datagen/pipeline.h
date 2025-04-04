#ifndef ILLUMINA_PIPELINE_H
#define ILLUMINA_PIPELINE_H

#include <random>

#include <nlohmann/json/json.hpp>

#include "datagen_types.h"
#include "weighted_vector.h"

namespace illumina {

class Generator {
public:
    virtual void load_settings(const nlohmann::json& j);
    virtual void Board(Board& board);

    virtual ~Generator() = default;
};

/**
 * Nitpicks data from a simulation.
 */
class DataSelector {
public:
    virtual void load_settings(const nlohmann::json& j);

    virtual std::vector<DataPoint> select(ThreadContext& ctx,
                                          const Game& game) = 0;

    virtual ~DataSelector() = default;
};

inline void DataSelector::load_settings(const nlohmann::json& j) {
    // Ignore
}

/**
 * Writes data to an output stream.
 */
class DataFormatter {
public:
    virtual void load_settings(const nlohmann::json& j);

    virtual ui64 write(ThreadContext& ctx,
                       std::ostream& stream,
                       const Game& game,
                       const std::vector<DataPoint>& extracted_data) = 0;

    virtual ~DataFormatter() = default;
};

inline void DataFormatter::load_settings(const nlohmann::json& j) {
    // Ignore
}

class Pipeline {
public:
    /**
     * Constructs a pipeline object from a Pipeline definition JSON.
     *
     * If the JSON string is empty (default), we use the default
     * pipeline included in datagen/pipelines.
     */
    Pipeline(const std::string& pipeline_json = "");

    DataSelector&  pick_selector() const;
    DataFormatter& get_formatter() const;

private:
    WeightedVector<std::unique_ptr<DataSelector>> m_selectors;
    std::unique_ptr<DataFormatter> m_formatter;
    std::unique_ptr<std::mt19937> m_rng = std::make_unique<std::mt19937>();
};

} // illumina

#endif // ILLUMINA_PIPELINE_H
