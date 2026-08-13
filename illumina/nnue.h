#ifndef ILLUMINA_NNUE_H
#define ILLUMINA_NNUE_H

#include <array>
#include <vector>

#include "simd.h"
#include "types.h"

namespace illumina {

static constexpr size_t N_INPUTS = 768;
static constexpr size_t L1_SIZE  = 768;

struct EvalNetwork {
    alignas(32) std::array<i16, N_INPUTS * L1_SIZE> l1_weights;
    alignas(32) std::array<i16, L1_SIZE> l1_biases;
    alignas(32) std::array<i16, L1_SIZE * 2> output_weights;
    i16 output_bias;

};

struct Accumulator {
    alignas(32) std::array<i16, L1_SIZE> white {};
    alignas(32) std::array<i16, L1_SIZE> black {};
};

class NNUE {
public:
    void clear();
    void push_accumulator();
    void pop_accumulator();

    void enable_feature(Square square, Piece piece);
    void disable_feature(Square square, Piece piece);

    template <int N_ENABLED, int N_DISABLED>
    void update_features(const std::array<Square, N_ENABLED>& enabled_squares,
                         const std::array<Piece, N_ENABLED>& enabled_pieces,
                         const std::array<Square, N_DISABLED>& disabled_squares,
                         const std::array<Piece, N_DISABLED>& disabled_pieces);

    int forward(Color color) const;

    NNUE();

private:
    const EvalNetwork* m_net;
    Accumulator m_accum {};
    std::vector<Accumulator> m_accum_stack;

    template <Color C>
    static size_t feature_index(Square square, Piece piece);
};

template <Color C>
size_t NNUE::feature_index(Square square, Piece piece) {
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

template <int N_ENABLED, int N_DISABLED>
void NNUE::update_features(const std::array<Square, N_ENABLED>& enabled_squares,
                           const std::array<Piece, N_ENABLED>& enabled_pieces,
                           const std::array<Square, N_DISABLED>& disabled_squares,
                           const std::array<Piece, N_DISABLED>& disabled_pieces) {
    static_assert(N_ENABLED >= 0  && N_ENABLED <= 2);
    static_assert(N_DISABLED >= 0 && N_DISABLED <= 2);

    std::array<size_t, N_ENABLED>  en_white_idxs;
    std::array<size_t, N_ENABLED>  en_black_idxs;
    std::array<size_t, N_DISABLED> dis_white_idxs;
    std::array<size_t, N_DISABLED> dis_black_idxs;

    for (int i = 0; i < N_ENABLED; ++i) {
        en_white_idxs[i] = feature_index<CL_WHITE>(enabled_squares[i], enabled_pieces[i]);
        en_black_idxs[i] = feature_index<CL_BLACK>(enabled_squares[i], enabled_pieces[i]);
    }
    for (int i = 0; i < N_DISABLED; ++i) {
        dis_white_idxs[i] = feature_index<CL_WHITE>(disabled_squares[i], disabled_pieces[i]);
        dis_black_idxs[i] = feature_index<CL_BLACK>(disabled_squares[i], disabled_pieces[i]);
    }

    auto update = [this](auto& accum, const auto& enabled, const auto& disabled) {
        for (size_t i = 0; i < L1_SIZE; i += SimdVecI16::STRIDE) {
            SimdVecI16 value = SimdVecI16::load_aligned(&accum[i]);

            for (size_t index : enabled) {
                value += SimdVecI16::load_aligned(&m_net->l1_weights[index * L1_SIZE + i]);
            }
            for (size_t index : disabled) {
                value -= SimdVecI16::load_aligned(&m_net->l1_weights[index * L1_SIZE + i]);
            }

            value.store_aligned(&accum[i]);
        }
    };

    update(m_accum.white, en_white_idxs, dis_white_idxs);
    update(m_accum.black, en_black_idxs, dis_black_idxs);
}

} // illumina

#endif // ILLUMINA_NNUE_H
