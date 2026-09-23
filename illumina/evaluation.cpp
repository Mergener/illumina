#include "evaluation.h"

#include "endgame.h"
#include "tunablevalues.h"

#include <cmath>

namespace illumina {

void Evaluation::on_new_board(const Board& board) {
    m_n_eval_lazy_updates = 0;
    m_n_complexity_lazy_updates = 0;
    m_eval_nnue.clear();
    m_complexity_nnue.clear();

    // Activate every feature individually.
    Bitboard bb = board.occupancy();
    while (bb) {
        Square s = lsb(bb);
        Piece piece = board.piece_at(s);
        m_eval_nnue.enable_feature(s, piece);
        m_complexity_nnue.enable_feature(s, piece);
        bb = unset_lsb(bb);
    }
}

void Evaluation::on_make_move(const Board& board, Move move) {
    m_eval_lazy_updates[m_n_eval_lazy_updates++] = move;
    m_complexity_lazy_updates[m_n_complexity_lazy_updates++] = move;
}

void Evaluation::on_undo_move(const Board& board, Move move) {
    if (m_n_eval_lazy_updates != 0) {
        --m_n_eval_lazy_updates;
    } else {
        m_eval_nnue.pop_accumulator();
    }
    if (m_n_complexity_lazy_updates != 0) {
        --m_n_complexity_lazy_updates;
    } else {
        m_complexity_nnue.pop_accumulator();
    }
}

void Evaluation::on_make_null_move(const Board& board) {
    m_eval_lazy_updates[m_n_eval_lazy_updates++] = MOVE_NULL;
    m_complexity_lazy_updates[m_n_complexity_lazy_updates++] = MOVE_NULL;
}

void Evaluation::on_undo_null_move(const Board& board) {
    if (m_n_eval_lazy_updates != 0) {
        --m_n_eval_lazy_updates;
    }
    if (m_n_complexity_lazy_updates != 0) {
        --m_n_complexity_lazy_updates;
    }
}

template <typename Network>
static void update_move_features(Network& nnue, Move move) {
    Color moved_color = move.source_piece().color();
    switch (move.type()) {
        case MT_EN_PASSANT:
            nnue.template update_features<1, 2>(
                    {move.destination()},
                    {move.source_piece()},
                    {move.source(), move.destination() - pawn_push_direction(moved_color)},
                    {move.source_piece(), Piece(opposite_color(moved_color), PT_PAWN)});
            break;
        case MT_CASTLES:
            nnue.template update_features<2, 2>(
                    {castled_rook_square(moved_color, move.castles_side()), move.destination()},
                    {Piece(moved_color, PT_ROOK), move.source_piece()},
                    {move.castles_rook_src_square(), move.source()},
                    {Piece(moved_color, PT_ROOK), move.source_piece()});
            break;
        case MT_PROMOTION_CAPTURE:
            nnue.template update_features<1, 2>(
                    {move.destination()},
                    {Piece(moved_color, move.promotion_piece_type())},
                    {move.source(), move.destination()},
                    {move.source_piece(), move.captured_piece()});
            break;
        case MT_SIMPLE_CAPTURE:
            nnue.template update_features<1, 2>(
                    {move.destination()},
                    {move.source_piece()},
                    {move.source(), move.destination()},
                    {move.source_piece(), move.captured_piece()});
            break;
        case MT_SIMPLE_PROMOTION:
            nnue.template update_features<1, 1>(
                    {move.destination()},
                    {Piece(moved_color, move.promotion_piece_type())},
                    {move.source()},
                    {move.source_piece()});
            break;
        default:
            nnue.template update_features<1, 1>(
                    {move.destination()},
                    {move.source_piece()},
                    {move.source()},
                    {move.source_piece()});
            break;
    }
}

template <typename Network>
static void apply_pending_updates(Network& nnue, const std::array<Move, MAX_DEPTH>& updates,
                                  size_t& count) {
    for (size_t i = 0; i < count; ++i) {
        Move move = updates[i];
        if (move != MOVE_NULL) {
            nnue.push_accumulator();
            update_move_features(nnue, move);
        }
    }
    count = 0;
}

void Evaluation::apply_eval_lazy_updates() {
    apply_pending_updates(m_eval_nnue, m_eval_lazy_updates, m_n_eval_lazy_updates);
}

void Evaluation::apply_complexity_lazy_updates() {
    apply_pending_updates(m_complexity_nnue, m_complexity_lazy_updates,
                          m_n_complexity_lazy_updates);
}

Score Evaluation::compute(const Board& board) {
    apply_eval_lazy_updates();
    return std::clamp(m_eval_nnue.forward(board.color_to_move(), popcount(board.occupancy())), -KNOWN_WIN + 1, KNOWN_WIN - 1);
}

int Evaluation::complexity(const Board& board) {
    apply_complexity_lazy_updates();
    return std::clamp(m_complexity_nnue.forward(board.color_to_move(), popcount(board.occupancy())), 0, 16384);
}


static std::pair<double, double> wdl_params(Score score, const Board& board) {
    // Stockfish WDL normalization model parameters.
    // Generated using https://github.com/official-stockfish/WDL_model.
    constexpr double AS[] = {-416.97348813, 1213.95351188, -1368.58758315, 855.21105608};
    constexpr double BS[] = {-155.52564502, 417.75145499, -364.40511303, 181.81249513};

    int material = 1 * popcount(board.piece_type_bb(PT_PAWN))
                   + 3 * popcount(board.piece_type_bb(PT_KNIGHT))
                   + 3 * popcount(board.piece_type_bb(PT_BISHOP))
                   + 5 * popcount(board.piece_type_bb(PT_ROOK))
                   + 9 * popcount(board.piece_type_bb(PT_QUEEN));

    double x = std::clamp(material, 17, 78) / 58.0;

    double p_a = ((AS[0] * x + AS[1]) * x + AS[2]) * x + AS[3];
    double p_b = ((BS[0] * x + BS[1]) * x + BS[2]) * x + BS[3];

    return { p_a, p_b };
}

Score normalize_score(Score score, const Board& board) {
    if (score == 0 || std::abs(score) >= KNOWN_WIN) {
        return score;
    }

    auto [a, _] = wdl_params(score, board);
    return Score(std::round(100.0 * double(score) / a));
}

WDL wdl_from_score(Score score, const Board& board) {
    if (score >= KNOWN_WIN) {
        return { 1000, 0, 0 };
    }
    if (score <= -KNOWN_WIN) {
        return { 0, 0, 1000 };
    }

    auto [a, b] = wdl_params(score, board);

    int w = std::round(1000.0 / (1.0 + std::exp((a - double(score)) / b)));
    int l = std::round(1000.0 / (1.0 + std::exp((a + double(score)) / b)));
    int d = 1000 - w - l;

    return { w, d, l };
}

} // illumina
