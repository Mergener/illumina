#include "endgame.h"

#include <unordered_map>
#include <algorithm>

namespace illumina {

static ui64 create_endgame_key(int our_pawns, int our_knights, int our_bishops, int our_rooks, int our_queens,
                               int their_pawns, int their_knights, int their_bishops, int their_rooks, int their_queens) {

    ui64 key =         (our_pawns     & 0x3F);
    key = (key << 6) | (our_knights   & 0x3F);
    key = (key << 6) | (our_bishops   & 0x3F);
    key = (key << 6) | (our_rooks     & 0x3F);
    key = (key << 6) | (our_queens    & 0x3F);
    key = (key << 6) | (their_pawns   & 0x3F);
    key = (key << 6) | (their_knights & 0x3F);
    key = (key << 6) | (their_bishops & 0x3F);
    key = (key << 6) | (their_rooks   & 0x3F);
    key = (key << 6) | (their_queens  & 0x3F);
    return key;
}

static std::unordered_map<ui64, EndgameType> s_eg_table = {
    //                   P  N  B  R  Q  p  n  b  r  q
    { create_endgame_key(0, 0, 0, 0, 1, 0, 0, 0, 0, 0), EG_KQ_K },
    { create_endgame_key(0, 0, 0, 1, 0, 0, 0, 0, 0, 0), EG_KR_K },
    { create_endgame_key(0, 1, 1, 0, 0, 0, 0, 0, 0, 0), EG_KBN_K },
    { create_endgame_key(0, 0, 0, 0, 1, 0, 0, 0, 1, 0), EG_KQ_KR },
    { create_endgame_key(0, 0, 0, 0, 1, 0, 0, 1, 0, 0), EG_KQ_KB },
    { create_endgame_key(0, 0, 0, 0, 1, 0, 1, 0, 0, 0), EG_KQ_KN },
    { create_endgame_key(0, 0, 0, 1, 0, 0, 1, 0, 0, 0), EG_KR_KN },
    { create_endgame_key(0, 0, 0, 1, 0, 0, 0, 1, 0, 0), EG_KR_KB },
};

static EndgameType identify_endgame_type(const Board& board, Color stronger) {
    Color us   = stronger;
    Color them = opposite_color(us);

    ui64 key = create_endgame_key(popcount(board.piece_bb(Piece(us, PT_PAWN))),
                                  popcount(board.piece_bb(Piece(us, PT_KNIGHT))),
                                  popcount(board.piece_bb(Piece(us, PT_BISHOP))),
                                  popcount(board.piece_bb(Piece(us, PT_ROOK))),
                                  popcount(board.piece_bb(Piece(us, PT_QUEEN))),
                                  popcount(board.piece_bb(Piece(them, PT_PAWN))),
                                  popcount(board.piece_bb(Piece(them, PT_KNIGHT))),
                                  popcount(board.piece_bb(Piece(them, PT_BISHOP))),
                                  popcount(board.piece_bb(Piece(them, PT_ROOK))),
                                  popcount(board.piece_bb(Piece(them, PT_QUEEN))));

    auto it = s_eg_table.find(key);
    return it != s_eg_table.end() ? it->second : EG_UNKNOWN;
}

static Score corner_king_evaluation(const Board& board,
                                    Color winning_king_color) {
    Square winning_king_sq = board.king_square(winning_king_color);
    Square losing_king_sq  = board.king_square(opposite_color(winning_king_color));

    int ct_dist_losing = center_manhattan_distance(losing_king_sq);
    int king_dist      = manhattan_distance(winning_king_sq, losing_king_sq);

    return 8 * ((ct_dist_losing * ct_dist_losing) - (king_dist));
}

static Score evaluate_endgame(const Board& board,
                              EndgameType eg_type,
                              Color stronger_player) {
    switch (eg_type) {
        case EG_KQ_KR:
            return KNOWN_WIN * 2
                 - ((rook_attacks(board.king_square(stronger_player), 0) & board.piece_bb(Piece(stronger_player, PT_QUEEN))) != 0) * 500
                 + corner_king_evaluation(board, stronger_player);

        case EG_KQ_KB:
            return KNOWN_WIN * 2
                 + corner_king_evaluation(board, stronger_player)
                 - ((bishop_attacks(board.king_square(stronger_player), 0) & board.piece_bb(Piece(stronger_player, PT_QUEEN))) != 0) * 500
                 + 625;

        case EG_KQ_KN:
            return KNOWN_WIN * 2
                 + corner_king_evaluation(board, stronger_player)
                 + 1250;

        case EG_KR_K:
            return KNOWN_WIN * 2
                 + corner_king_evaluation(board, stronger_player)
                 + 2500;

        case EG_KQ_K:
            return KNOWN_WIN * 2
                 + corner_king_evaluation(board, stronger_player)
                 + 5000;

        case EG_KBN_K: {
            constexpr i32 LONE_KING_BONUS_DS[] {
                0, 1, 2, 3, 4, 5, 6, 7,
                1, 2, 3, 4, 5, 6, 7, 6,
                2, 3, 4, 5, 6, 7, 6, 5,
                3, 4, 5, 6, 7, 6, 5, 4,
                4, 5, 6, 7, 6, 5, 4, 3,
                5, 6, 7, 6, 5, 4, 3, 2,
                6, 7, 6, 5, 4, 3, 2, 1,
                7, 6, 5, 4, 3, 2, 1, 0,
            };
            constexpr i32 LONE_KING_BONUS_LS[] {
                7, 6, 5, 4, 3, 2, 1, 0,
                6, 7, 6, 5, 4, 3, 2, 1,
                5, 6, 7, 6, 5, 4, 3, 2,
                4, 5, 6, 7, 6, 5, 4, 3,
                3, 4, 5, 6, 7, 6, 5, 4,
                2, 3, 4, 5, 6, 7, 6, 5,
                1, 2, 3, 4, 5, 6, 7, 6,
                0, 1, 2, 3, 4, 5, 6, 7,
            };

            i32 base = KNOWN_WIN / 2;

            Square our_bishop = lsb(board.piece_bb(Piece(stronger_player, PT_BISHOP)));
            Square their_king = board.king_square(opposite_color(stronger_player));

            i32 their_king_bonus;
            if (bit_is_set(LIGHT_SQUARES, our_bishop)) {
                their_king_bonus = LONE_KING_BONUS_LS[their_king];
            }
            else {
                their_king_bonus = LONE_KING_BONUS_DS[their_king];
            }

            return base - their_king_bonus * 50;
        }
        default:
            return 0;
    }
}

Endgame identify_endgame(const Board& board) {
    Endgame endgame;
    for (Color c: COLORS) {
        EndgameType type = identify_endgame_type(board, c);
        if (type == EG_UNKNOWN) {
            continue;
        }

        // Found an endgame, evaluate and return.
        endgame.type = type;
        Score stronger_player_evaluation = evaluate_endgame(board, type, c);
        endgame.evaluation = board.color_to_move() == c
                             ? stronger_player_evaluation
                             : -stronger_player_evaluation;
        break;

    }
    return endgame;
}

} // illumina