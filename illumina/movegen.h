#ifndef ILLUMINA_MOVEGEN_H
#define ILLUMINA_MOVEGEN_H

#include <type_traits>
#include <algorithm>

#include "debug.h"
#include "types.h"
#include "board.h"
#include "attacks.h"
#include "staticlist.h"

namespace illumina {

constexpr size_t MAX_GENERATED_MOVES = 256;

template <ui64 MOVE_TYPE_MASK = UINT64_MAX,
          bool LEGAL_ONLY = true,
          ui64 PIECE_TYPE_MASK = BITMASK(PT_COUNT),
          typename TMOVE = Move,
          ui64 PROMOTION_TYPE_MASK = BITMASK(PT_COUNT)>
TMOVE* generate_moves(const Board& board, TMOVE* moves);

template <Color C,
          ui64 MOVE_TYPE_MASK = UINT64_MAX,
          ui64 PIECE_TYPE_MASK = BITMASK(PT_COUNT),
          typename TMOVE = Move,
          ui64 PROMOTION_TYPE_MASK = BITMASK(PT_COUNT)>
TMOVE* generate_moves_by_color(const Board& board, TMOVE* moves);

template <Color C,
          ui64 MOVE_TYPE_MASK = UINT64_MAX,
          typename TMOVE = Move,
          ui64 PROMOTION_TYPE_MASK = BITMASK(PT_COUNT)>
TMOVE* generate_pawn_moves_by_color(const Board& board, TMOVE* moves);

template <Color C,
          ui64 MOVE_TYPE_MASK = UINT64_MAX,
          typename TMOVE = Move>
TMOVE* generate_knight_moves_by_color(const Board& board, TMOVE* moves);

template <Color C,
          ui64 MOVE_TYPE_MASK = UINT64_MAX,
          typename TMOVE = Move>
TMOVE* generate_bishop_moves_by_color(const Board& board, TMOVE* moves);

template <Color C,
          ui64 MOVE_TYPE_MASK = UINT64_MAX,
          typename TMOVE = Move>
TMOVE* generate_rook_moves_by_color(const Board& board, TMOVE* moves);

template <Color C,
          ui64 MOVE_TYPE_MASK = UINT64_MAX,
          typename TMOVE = Move>
TMOVE* generate_queen_moves_by_color(const Board& board, TMOVE* moves);

template <Color C,
          ui64 MOVE_TYPE_MASK = UINT64_MAX,
          typename TMOVE = Move>
TMOVE* generate_king_moves_by_color(const Board& board, TMOVE* moves);

template <Color C,
          ui64 MOVE_TYPE_MASK = UINT64_MAX,
          typename TMOVE = Move>
TMOVE* generate_evasions_by_color(const Board& board, TMOVE* moves);

template <ui64 MOVE_TYPE_MASK = UINT64_MAX,
          typename TMOVE = Move>
TMOVE* generate_evasions(const Board& board, TMOVE* moves);

} // illumina

#include "movegen_impl.h"

#endif // ILLUMINA_MOVEGEN_H
