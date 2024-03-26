#include "evaluation.h"

namespace illumina {

template <Color C>
Score count_material(const Board& board) {
    Score material = Score(popcount(board.piece_bb(Piece(C, PT_PAWN))) * 82)
         + Score(popcount(board.piece_bb(Piece(C, PT_KNIGHT))) * 337)
         + Score(popcount(board.piece_bb(Piece(C, PT_BISHOP))) * 365)
         + Score(popcount(board.piece_bb(Piece(C, PT_ROOK))) * 477)
         + Score(popcount(board.piece_bb(Piece(C, PT_QUEEN))) * 1025);

    return material;
}

static constexpr Score pawn_scores[] = {
      0,   0,   0,   0,   0,   0,  0,   0,
     98, 134,  61,  95,  68, 126, 34, -11,
     -6,   7,  26,  31,  65,  56, 25, -20,
    -14,  13,   6,  21,  23,  12, 17, -23,
    -27,  -2,  -5,  12,  17,   6, 10, -25,
    -26,  -4,  -4, -10,   3,   3, 33, -12,
    -35,  -1, -20, -23, -15,  24, 38, -22,
      0,   0,   0,   0,   0,   0,  0,   0,
};

static constexpr Score knight_scores[] = {
    -167, -89, -34, -49,  61, -97, -15, -107,
    -73, -41,  72,  36,  23,  62,   7,  -17,
    -47,  60,  37,  65,  84, 129,  73,   44,
    -9,  17,  19,  53,  37,  69,  18,   22,
    -13,   4,  16,  13,  28,  19,  21,   -8,
    -23,  -9,  12,  10,  19,  17,  25,  -16,
    -29, -53, -12,  -3,  -1,  18, -14,  -19,
    -105, -21, -58, -33, -17, -28, -19,  -23,
};

static constexpr Score bishop_scores[] = {
    -29,   4, -82, -37, -25, -42,   7,  -8,
    -26,  16, -18, -13,  30,  59,  18, -47,
    -16,  37,  43,  40,  35,  50,  37,  -2,
    -4,   5,  19,  50,  37,  37,   7,  -2,
    -6,  13,  13,  26,  34,  12,  10,   4,
    0,  15,  15,  15,  14,  27,  18,  10,
    4,  15,  16,   0,   7,  21,  33,   1,
    -33,  -3, -14, -21, -13, -12, -39, -21,
};

static constexpr Score rook_scores[] = {
    32,  42,  32,  51, 63,  9,  31,  43,
    27,  32,  58,  62, 80, 67,  26,  44,
    -5,  19,  26,  36, 17, 45,  61,  16,
    -24, -11,   7,  26, 24, 35,  -8, -20,
    -36, -26, -12,  -1,  9, -7,   6, -23,
    -45, -25, -16, -17,  3,  0,  -5, -33,
    -44, -16, -20,  -9, -1, 11,  -6, -71,
    -19, -13,   1,  17, 16,  7, -37, -26,
};

static constexpr Score queen_scores[] = {
    -28,   0,  29,  12,  59,  44,  43,  45,
    -24, -39,  -5,   1, -16,  57,  28,  54,
    -13, -17,   7,   8,  29,  56,  47,  57,
    -27, -27, -16, -16,  -1,  17,  -2,   1,
    -9, -26,  -9, -10,  -2,  -4,   3,  -3,
    -14,   2, -11,  -2,  -5,   2,  14,   5,
    -35,  -8,  11,   2,   8,  15,  -3,   1,
    -1, -18,  -9,  10, -15, -25, -31, -50,
};

static constexpr Score king_scores[] = {
    -65,  23,  16, -15, -56, -34,   2,  13,
    29,  -1, -20,  -7,  -8,  -4, -38, -29,
    -9,  24,   2, -16, -20,   6,  22, -22,
    -17, -20, -12, -27, -30, -25, -14, -36,
    -49,  -1, -27, -39, -46, -44, -33, -51,
    -14, -14, -22, -46, -44, -30, -15, -27,
    1,   7,  -8, -64, -43, -16,   9,   8,
    -15,  36,  12, -54,   8, -28,  24,  14,
};

static const Score* scores[] = {
    nullptr,
    pawn_scores,
    knight_scores,
    bishop_scores,
    rook_scores,
    queen_scores,
    king_scores
};

template <Color C, PieceType PT>
Score count_positional(const Board& board) {
    Bitboard bb = board.piece_bb(Piece(C, PT));
    Score total = 0;

    while (bb) {
        Square s = lsb(bb);

        if constexpr (C == CL_WHITE) {
            s = mirror_vertical(s);
        }

        total += scores[PT][s];

        bb = unset_lsb(bb);
    }

    return total;
}

void Evaluation::on_new_board(const Board& board) {
    m_ctm = board.color_to_move();
    Score white_eval = 0;

    white_eval += count_material<CL_WHITE>(board)   - count_material<CL_BLACK>(board);
    white_eval += count_positional<CL_WHITE, PT_PAWN>(board) - count_positional<CL_BLACK, PT_PAWN>(board);
    white_eval += count_positional<CL_WHITE, PT_KNIGHT>(board) - count_positional<CL_BLACK, PT_KNIGHT>(board);
    white_eval += count_positional<CL_WHITE, PT_BISHOP>(board) - count_positional<CL_BLACK, PT_BISHOP>(board);
    white_eval += count_positional<CL_WHITE, PT_ROOK>(board) - count_positional<CL_BLACK, PT_ROOK>(board);
    white_eval += count_positional<CL_WHITE, PT_QUEEN>(board) - count_positional<CL_BLACK, PT_QUEEN>(board);
    white_eval += count_positional<CL_WHITE, PT_KING>(board) - count_positional<CL_BLACK, PT_KING>(board);

    m_score_white = white_eval;
}

} // illumina