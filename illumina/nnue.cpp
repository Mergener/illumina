#include "nnue.h"

#include <incbin/incbin.h>
#include <nlohmann/json/json.hpp>

namespace illumina {

INCTXT(_default_network, NNUE_PATH);

static const EvalNetwork* s_default_network = nullptr;

constexpr int SCALE = 400;
constexpr int Q1    = 255;
constexpr int Q2    = 64;
constexpr int Q     = L1_ACTIVATION == ActivationFunction::CReLU
                      ? (Q1 * Q2)
                      : (Q1 * Q1 * Q2);

template <Color C>
static size_t feature_index(Square square, Piece piece) {
    Color color     = piece.color();
    size_t type_idx = piece.type() - 1;

    if constexpr (C == CL_BLACK) {
        square = mirror_vertical(square);
        color  = opposite_color(color);
    }

    size_t index = 0;
    index = index * CL_COUNT + color;
    index = index * (PT_COUNT - 1) + type_idx;
    index = index * SQ_COUNT + square;
    return index;
}

void NNUE::clear() {
    // Copy all biases.
    std::copy(m_net->l1_biases.begin(), m_net->l1_biases.end(), m_accum.white.begin());
    std::copy(m_net->l1_biases.begin(), m_net->l1_biases.end(), m_accum.black.begin());
}

int NNUE::forward(Color color) const {
    if constexpr (L1_ACTIVATION == ActivationFunction::CReLU) {
        int sum = 0;

        auto our_accum = color == CL_WHITE ? m_accum.white : m_accum.black;
        auto their_accum = color == CL_WHITE ? m_accum.black : m_accum.white;

        for (size_t i = 0; i < L1_SIZE; ++i) {
            int activated = std::clamp(int(our_accum[i]), 0, Q1);
            sum += activated * m_net->output_weights[i];
        }

        for (size_t i = 0; i < L1_SIZE; ++i) {
            int activated = std::clamp(int(their_accum[i]), 0, Q1);
            sum += activated * m_net->output_weights[L1_SIZE + i];
        }

        return (sum + m_net->output_bias) * SCALE / Q;
    }
    else if constexpr (L1_ACTIVATION == ActivationFunction::SCReLU) {
        int sum = 0;

        auto our_accum = color == CL_WHITE ? m_accum.white : m_accum.black;
        auto their_accum = color == CL_WHITE ? m_accum.black : m_accum.white;

        for (size_t i = 0; i < L1_SIZE; ++i) {
            int activated = std::clamp(int(our_accum[i]), 0, Q1);
            activated = activated * activated;
            sum += activated * m_net->output_weights[i];
        }

        for (size_t i = 0; i < L1_SIZE; ++i) {
            int activated = std::clamp(int(their_accum[i]), 0, Q1);
            activated = activated * activated;
            sum += activated * m_net->output_weights[L1_SIZE + i];
        }

        return (sum + m_net->output_bias) * SCALE / Q;
    }
}

void NNUE::enable_feature(Square square, Piece piece) {
    size_t white_idx = feature_index<CL_WHITE>(square, piece);
    size_t black_idx = feature_index<CL_BLACK>(square, piece);

    for (size_t i = 0; i < L1_SIZE; ++i) {
        m_accum.white[i] += m_net->l1_weights[N_INPUTS * i + white_idx];
        m_accum.black[i] += m_net->l1_weights[N_INPUTS * i + black_idx];
    }
}

void NNUE::disable_feature(Square square, Piece piece) {
    size_t white_idx = feature_index<CL_WHITE>(square, piece);
    size_t black_idx = feature_index<CL_BLACK>(square, piece);

    for (size_t i = 0; i < L1_SIZE; ++i) {
        m_accum.white[i] -= m_net->l1_weights[N_INPUTS * i + white_idx];
        m_accum.black[i] -= m_net->l1_weights[N_INPUTS * i + black_idx];
    }
}

void NNUE::push_accumulator() {
    m_accum_stack.push_back(m_accum);
}

void NNUE::pop_accumulator() {
    ILLUMINA_ASSERT(!m_accum_stack.empty());

    m_accum = m_accum_stack.back();
    m_accum_stack.pop_back();
}

NNUE::NNUE()
    : m_net(s_default_network) {
    clear();
}

template <typename T>
static void copy_params_from_json(const nlohmann::json& j,
                                  std::string_view json_field_name,
                                  T& arr) {
    nlohmann::json j_arr = j[json_field_name];
    for (size_t i = 0; i < arr.size(); ++i) {
        arr[i] = j_arr.at(i);
    }
}

EvalNetwork::EvalNetwork(std::istream& stream) {
    nlohmann::json j = nlohmann::json::parse(stream);

    copy_params_from_json(j, "l1_weights", l1_weights);
    copy_params_from_json(j, "l1_biases", l1_biases);
    copy_params_from_json(j, "out_weights", output_weights);

    output_bias = j.at("out_biases")[0];
}

void init_nnue() {
    std::string str = std::string(g_default_networkData, g_default_networkSize);
    std::istringstream stream(str);
    s_default_network = new EvalNetwork(stream);
}

} // illumina