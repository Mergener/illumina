#include "evaluation.h"

#include <istream>
#include <sstream>
#include <nlohmann/json/json.hpp>
#include <incbin/incbin.h>

namespace illumina {

INCTXT(_default_network, NNUE_PATH);

static const EvalNetwork* s_default_network = nullptr;

constexpr Score SCALE = 400;
constexpr int Q       = 255 * 64;

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

Score NNUE::forward(Color color) const {
    int sum = 0;

    auto our_accum   = color == CL_WHITE ? m_accum.white : m_accum.black;
    auto their_accum = color == CL_WHITE ? m_accum.black : m_accum.white;

    for (size_t i = 0; i < L1_SIZE; ++i) {
        int activated = std::clamp(int(our_accum[i]), 0, 255);
        sum += activated * m_net->output_weights[i];
    }

    for (size_t i = 0; i < L1_SIZE; ++i) {
        int activated = std::clamp(int(their_accum[i]), 0, 255);
        sum += activated * m_net->output_weights[L1_SIZE + i];
    }

    return (sum + m_net->output_bias) * SCALE / Q;
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

void Evaluation::on_new_board(const Board& board) {
    m_nnue.clear();
    m_ctm = board.color_to_move();

    // Activate every feature individually.
    Bitboard bb = board.occupancy();
    while (bb) {
        Square s = lsb(bb);
        m_nnue.enable_feature(s, board.piece_at(s));
        bb = unset_lsb(bb);
    }
}

void Evaluation::on_make_move(const Board& board, Move move) {
    m_ctm = opposite_color(m_ctm);

    // Store reused values.
    Square source      = move.source();
    Square destination = move.destination();
    Piece source_piece = move.source_piece();
    Color moving_color = source_piece.color();

    // Every move type always removes the source piece from the original square.
    m_nnue.disable_feature(source, source_piece);

    // Deactivate feature when moving to previously occupied squares (aka captures except en-passants).
    Piece dest_piece = board.piece_at(move.destination());
    if (dest_piece != PIECE_NULL) {
        m_nnue.disable_feature(destination, dest_piece);
    }

    // Activate the feature that matches the newly positioned piece.
    m_nnue.enable_feature(destination, !move.is_promotion()
                                       ? source_piece
                                       : Piece(moving_color, move.promotion_piece_type()));

    // Castles need to activate/deactivate the castled rook properly.
    if (move.type() == MT_CASTLES) {
        Side side  = move.castles_side();
        Piece rook = Piece(moving_color, PT_ROOK);
        m_nnue.disable_feature(move.castles_rook_src_square(), rook);
        m_nnue.enable_feature(castled_rook_square(moving_color, side), rook);
    }

    // En passants must deactivate the square right behind the moved pawn.
    if (move.type() == MT_EN_PASSANT) {
        Square captured_ep_square = move.destination() - pawn_push_direction(moving_color);
        m_nnue.disable_feature(captured_ep_square, Piece(opposite_color(moving_color), PT_PAWN));
    }
}

void Evaluation::on_undo_move(const Board& board, Move move) {
    m_ctm = opposite_color(m_ctm);

    // Store reused values.
    Square source      = move.source();
    Square destination = move.destination();
    Piece source_piece = move.source_piece();
    Color moving_color = source_piece.color();

    // Every move type always removes the source piece from the original square.
    m_nnue.enable_feature(source, source_piece);

    // Activate feature when moved to previously occupied squares (aka captures except en-passants).
    Piece dest_piece = move.captured_piece();
    if (move.type() == MT_SIMPLE_CAPTURE || move.type() == MT_PROMOTION_CAPTURE) {
        m_nnue.enable_feature(destination, dest_piece);
    }

    // Deactivate the feature that matches the newly positioned piece.
    m_nnue.disable_feature(destination, !move.is_promotion()
                                        ? source_piece
                                        : Piece(moving_color, move.promotion_piece_type()));

    // Castles need to activate/deactivate the castled rook properly.
    if (move.type() == MT_CASTLES) {
        Side side  = move.castles_side();
        Piece rook = Piece(moving_color, PT_ROOK);
        m_nnue.enable_feature(move.castles_rook_src_square(), rook);
        m_nnue.disable_feature(castled_rook_square(moving_color, side), rook);
    }

    // En passants must reactivate the square right behind the moved pawn.
    if (move.type() == MT_EN_PASSANT) {
        Square captured_ep_square = move.destination() - pawn_push_direction(moving_color);
        m_nnue.enable_feature(captured_ep_square, Piece(opposite_color(moving_color), PT_PAWN));
    }
}

void Evaluation::on_make_null_move(const Board& board) {
    m_ctm = opposite_color(m_ctm);
}

void Evaluation::on_undo_null_move(const Board& board) {
    m_ctm = opposite_color(m_ctm);
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
    std::cout << "j_arr size " << json_field_name << " " << j_arr.size() << std::endl;
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

void init_eval() {
    std::string str = std::string(g_default_networkData, g_default_networkSize);
    std::istringstream stream(str);
    s_default_network = new EvalNetwork(stream);
}

} // illumina