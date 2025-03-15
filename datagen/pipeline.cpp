#include "pipeline.h"

#include <random>
#include <functional>
#include <unordered_map>

#include <incbin/incbin.h>

#include "selectors/base_selector.h"
#include "formatters/marlinflow.h"

namespace illumina {

INCTXT(_default_pipeline, PIPELINE_PATH);

void Pipeline::compute_selector_weight_sum() {
    int sum = 0;
    for (const auto& selector: m_selectors) {
        sum += selector.weight;
    }
    m_selector_weight_sum = sum;
}

DataSelector& Pipeline::pick_selector() const {
    std::uniform_int_distribution<int> dist(1, m_selector_weight_sum);
    int random = dist(*m_rng);

    auto it = m_selectors.begin();
    while (random > 0) {
        random -= it->weight;
        it++;
    }
    it--;

    return *it->value;
}

DataFormatter& Pipeline::get_formatter() const {
    return *m_formatter;
}

struct SelectorDefinition {
    std::string type = "base";
    int weight = 1000;
    std::optional<nlohmann::json> options = std::nullopt;
};

void from_json(const nlohmann::json& j, SelectorDefinition& d) {
    d.type = j.at("type");

    if (j.contains("options")) {
        d.options = j.at("options");
    }
}

struct FormatterDefinition {
    std::string type = "marlinflow";
    std::optional<nlohmann::json> options = std::nullopt;
};

void from_json(const nlohmann::json& j, FormatterDefinition& d) {
    d.type = j.at("type");

    if (j.contains("options")) {
        d.options = j.at("options");
    }
}

struct PipelineDefinition {
    std::vector<SelectorDefinition> selectors;
    FormatterDefinition formatter;
};

void from_json(const nlohmann::json& j, PipelineDefinition& d) {
    d.selectors = j.at("selectors");
    d.formatter = j.at("formatter");
}

template <typename T>
std::unique_ptr<T> build_component() {
    return std::make_unique<T>();
}

template <typename T>
using ComponentBuilder = std::function<std::unique_ptr<T>()>;

static std::unordered_map<std::string, ComponentBuilder<DataSelector>> s_selector_builders = {
    { "base", build_component<BaseSelector> }
};

static std::unordered_map<std::string, ComponentBuilder<DataFormatter>> s_formatter_builders = {
    { "marlinflow", build_component<MarlinflowFormatter> }
};

Pipeline::Pipeline(const std::string& pipeline_json) {
    try {
        // Load pipeline JSON, either from parameter or the default one.
        std::string_view json = !pipeline_json.empty()
                                ? pipeline_json
                                : std::string_view(g_default_pipelineData, g_default_pipelineSize);

        PipelineDefinition pipeline_defs = nlohmann::json::parse(json);

        // Once the definitions are loaded, we build our components.
        for (const auto& selector_def: pipeline_defs.selectors) {
            auto it = s_selector_builders.find(selector_def.type);
            if (it == s_selector_builders.end()) {
                throw std::out_of_range("Unrecognized selector type " + selector_def.type);
            }
            m_selectors.push_back({ it->second(), selector_def.weight });

            if (selector_def.options.has_value()) {
                DataSelector& selector = *(m_selectors.back().value);
                selector.load_settings(*selector_def.options);
            }
        }
        // Make sure selector weight sum is properly computed.
        compute_selector_weight_sum();

        const auto& formatter_def = pipeline_defs.formatter;
        auto it = s_formatter_builders.find(pipeline_defs.formatter.type);
        if (it == s_formatter_builders.end()) {
            throw std::out_of_range("Unrecognized formatter type " + pipeline_defs.formatter.type);
        }
        m_formatter = it->second();

        if (formatter_def.options.has_value()) {
            m_formatter->load_settings(*formatter_def.options);
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to parse pipeline definition." << std::endl;
        throw e;
    }
}

} // illumina