#ifndef ILLUMINA_NNUE_H
#define ILLUMINA_NNUE_H

#include <array>
#include <vector>
#include <istream>
#include <sstream>
#include <type_traits>
#include <immintrin.h>

#include "types.h"

namespace illumina {

enum class ActivationFunction {
    CReLU,
    SCReLU
};

static constexpr size_t N_INPUTS = 768;
static constexpr ActivationFunction L1_ACTIVATION = ActivationFunction::SCReLU;
static constexpr size_t L1_SIZE  = 256;

struct EvalNetwork {
    alignas(32) std::array<i16, N_INPUTS * L1_SIZE> l1_weights;
    alignas(32) std::array<i16, L1_SIZE> l1_biases;
    alignas(32) std::array<i16, L1_SIZE * 2> output_weights;
    i16 output_bias;

    EvalNetwork() = default;
    EvalNetwork(std::istream& stream);
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

    using EnabledIndexesArray  = typename std::conditional_t<(N_ENABLED > 0),
            std::array<size_t, N_ENABLED>,
            std::nullptr_t>;

    using DisabledIndexesArray = typename std::conditional_t<(N_DISABLED > 0),
            std::array<size_t, N_DISABLED>,
            std::nullptr_t>;

    EnabledIndexesArray  en_white_idxs;
    EnabledIndexesArray  en_black_idxs;
    DisabledIndexesArray dis_white_idxs;
    DisabledIndexesArray dis_black_idxs;

    if constexpr (N_ENABLED >= 1) {
        en_white_idxs[0] = feature_index<CL_WHITE>(enabled_squares[0], enabled_pieces[0]);
        en_black_idxs[0] = feature_index<CL_BLACK>(enabled_squares[0], enabled_pieces[0]);
    }
    if constexpr (N_ENABLED >= 2) {
        en_white_idxs[1] = feature_index<CL_WHITE>(enabled_squares[1], enabled_pieces[1]);
        en_black_idxs[1] = feature_index<CL_BLACK>(enabled_squares[1], enabled_pieces[1]);
    }
    if constexpr (N_DISABLED >= 1) {
        dis_white_idxs[0] = feature_index<CL_WHITE>(disabled_squares[0], disabled_pieces[0]);
        dis_black_idxs[0] = feature_index<CL_BLACK>(disabled_squares[0], disabled_pieces[0]);
    }
    if constexpr (N_DISABLED >= 2) {
        dis_white_idxs[1] = feature_index<CL_WHITE>(disabled_squares[1], disabled_pieces[1]);
        dis_black_idxs[1] = feature_index<CL_BLACK>(disabled_squares[1], disabled_pieces[1]);
    }

#ifdef HAS_AVX2
    constexpr size_t STRIDE = sizeof(__m256i) / sizeof(i16);
    for (size_t i = 0; i < L1_SIZE; i += STRIDE) {
        __m256i* white_accum_ptr = reinterpret_cast<__m256i*>(&m_accum.white[i]);
        __m256i* black_accum_ptr = reinterpret_cast<__m256i*>(&m_accum.black[i]);

        if constexpr (N_ENABLED >= 1) {
            const __m256i* weights_ptr = reinterpret_cast<const __m256i*>(&m_net->l1_weights[en_white_idxs[0] * L1_SIZE + i]);
            __m256i weights = _mm256_load_si256(weights_ptr);
            __m256i accum   = _mm256_load_si256(white_accum_ptr);
            __m256i sum     = _mm256_add_epi16(accum, weights);
            _mm256_store_si256(white_accum_ptr, sum);
        }
        if constexpr (N_ENABLED >= 2) {
            const __m256i* weights_ptr = reinterpret_cast<const __m256i*>(&m_net->l1_weights[en_white_idxs[1] * L1_SIZE + i]);
            __m256i weights = _mm256_load_si256(weights_ptr);
            __m256i accum   = _mm256_load_si256(white_accum_ptr);
            __m256i sum     = _mm256_add_epi16(accum, weights);
            _mm256_store_si256(white_accum_ptr, sum);
        }
        if constexpr (N_DISABLED >= 1) {
            const __m256i* weights_ptr = reinterpret_cast<const __m256i*>(&m_net->l1_weights[dis_white_idxs[0] * L1_SIZE + i]);
            __m256i weights = _mm256_load_si256(weights_ptr);
            __m256i accum   = _mm256_load_si256(white_accum_ptr);
            __m256i sub     = _mm256_sub_epi16(accum, weights);
            _mm256_store_si256(white_accum_ptr, sub);
        }
        if constexpr (N_DISABLED >= 2) {
            const __m256i* weights_ptr = reinterpret_cast<const __m256i*>(&m_net->l1_weights[dis_white_idxs[1] * L1_SIZE + i]);
            __m256i weights = _mm256_load_si256(weights_ptr);
            __m256i accum   = _mm256_load_si256(white_accum_ptr);
            __m256i sub     = _mm256_sub_epi16(accum, weights);
            _mm256_store_si256(white_accum_ptr, sub);
        }

        if constexpr (N_ENABLED >= 1) {
            const __m256i* weights_ptr = reinterpret_cast<const __m256i*>(&m_net->l1_weights[en_black_idxs[0] * L1_SIZE + i]);
            __m256i weights = _mm256_load_si256(weights_ptr);
            __m256i accum   = _mm256_load_si256(black_accum_ptr);
            __m256i sum     = _mm256_add_epi16(accum, weights);
            _mm256_store_si256(black_accum_ptr, sum);
        }
        if constexpr (N_ENABLED >= 2) {
            const __m256i* weights_ptr = reinterpret_cast<const __m256i*>(&m_net->l1_weights[en_black_idxs[1] * L1_SIZE + i]);
            __m256i weights = _mm256_load_si256(weights_ptr);
            __m256i accum   = _mm256_load_si256(black_accum_ptr);
            __m256i sum     = _mm256_add_epi16(accum, weights);
            _mm256_store_si256(black_accum_ptr, sum);
        }
        if constexpr (N_DISABLED >= 1) {
            const __m256i* weights_ptr = reinterpret_cast<const __m256i*>(&m_net->l1_weights[dis_black_idxs[0] * L1_SIZE + i]);
            __m256i weights = _mm256_load_si256(weights_ptr);
            __m256i accum   = _mm256_load_si256(black_accum_ptr);
            __m256i sub     = _mm256_sub_epi16(accum, weights);
            _mm256_store_si256(black_accum_ptr, sub);
        }
        if constexpr (N_DISABLED >= 2) {
            const __m256i* weights_ptr = reinterpret_cast<const __m256i*>(&m_net->l1_weights[dis_black_idxs[1] * L1_SIZE + i]);
            __m256i weights = _mm256_load_si256(weights_ptr);
            __m256i accum   = _mm256_load_si256(black_accum_ptr);
            __m256i sub     = _mm256_sub_epi16(accum, weights);
            _mm256_store_si256(black_accum_ptr, sub);
        }
    }
#else
    for (size_t i = 0; i < L1_SIZE; ++i) {
        if constexpr (N_ENABLED >= 1) {
            m_accum.white[i] += m_net->l1_weights[en_white_idxs[0] * L1_SIZE + i];
        }
        if constexpr (N_ENABLED >= 2) {
            m_accum.white[i] += m_net->l1_weights[en_white_idxs[1] * L1_SIZE + i];
        }
        if constexpr (N_DISABLED >= 1) {
            m_accum.white[i] -= m_net->l1_weights[dis_white_idxs[0] * L1_SIZE + i];
        }
        if constexpr (N_DISABLED >= 2) {
            m_accum.white[i] -= m_net->l1_weights[dis_white_idxs[1] * L1_SIZE + i];
        }
        if constexpr (N_ENABLED >= 1) {
            m_accum.black[i] += m_net->l1_weights[en_black_idxs[0] * L1_SIZE + i];
        }
        if constexpr (N_ENABLED >= 2) {
            m_accum.black[i] += m_net->l1_weights[en_black_idxs[1] * L1_SIZE + i];
        }
        if constexpr (N_DISABLED >= 1) {
            m_accum.black[i] -= m_net->l1_weights[dis_black_idxs[0] * L1_SIZE + i];
        }
        if constexpr (N_DISABLED >= 2) {
            m_accum.black[i] -= m_net->l1_weights[dis_black_idxs[1] * L1_SIZE + i];
        }
    }
#endif
}

} // illumina

#endif // ILLUMINA_NNUE_H
