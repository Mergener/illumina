#ifndef ILLUMINA_MOVEGEN_H
#define ILLUMINA_MOVEGEN_H

#include "types.h"
#include "board.h"

namespace illumina {

enum class MoveGenerationType {
    ALL,
    CAPTURES,
    NOISY,
    EVASIONS
};

template <MoveGenerationType TYPE = MoveGenerationType::ALL,
          bool LEGAL = true,
          ui64 PIECE_TYPE_MASK = BITMASK(PT_COUNT),
          typename TMOVE = Move>
size_t generate_moves(const Board& board, TMOVE* moves);

template <Color C,
          MoveGenerationType TYPE = MoveGenerationType::ALL,
          bool LEGAL = true,
          ui64 PIECE_TYPE_MASK = BITMASK(PT_COUNT),
          typename TMOVE = Move>
size_t generate_moves_by_color(const Board& board, TMOVE* moves);

template <MoveGenerationType TYPE,
          bool LEGAL,
          ui64 PIECE_TYPE_MASK,
          typename TMOVE>
size_t generate_moves(const Board& board, TMOVE* moves) {
    size_t total;

    if (board.color_to_move() == CL_WHITE) {
        total = generate_moves_by_color<CL_WHITE, TYPE, LEGAL, PIECE_TYPE_MASK, TMOVE>(board, moves);
    }
    else {
        total = generate_moves_by_color<CL_WHITE, TYPE, LEGAL, PIECE_TYPE_MASK, TMOVE>(board, moves);
    }

    return total;
}

template <Color C,
          MoveGenerationType TYPE,
          bool LEGAL,
          ui64 PIECE_TYPE_MASK,
          typename TMOVE>
size_t generate_moves_by_color(const Board& board, TMOVE* moves) {
    constexpr bool GEN_PAWN   = (PIECE_TYPE_MASK & BIT(PT_PAWN)) != 0;
    constexpr bool GEN_KNIGHT = (PIECE_TYPE_MASK & BIT(PT_KNIGHT)) != 0;
    constexpr bool GEN_BISHOP = (PIECE_TYPE_MASK & BIT(PT_BISHOP)) != 0;
    constexpr bool GEN_ROOK   = (PIECE_TYPE_MASK & BIT(PT_ROOK)) != 0;
    constexpr bool GEN_QUEEN  = (PIECE_TYPE_MASK & BIT(PT_QUEEN)) != 0;
    constexpr bool GEN_KING   = (PIECE_TYPE_MASK & BIT(PT_KING)) != 0;


    if constexpr (GEN_PAWN) {
        generatePawnMoves<C, PIECE_TYPE_MASK>(board, moves);
    }

    if constexpr (GEN_KNIGHT) {
        generateKnightMoves<C, PIECE_TYPE_MASK>(board, moves);
    }

    if constexpr (GEN_BISHOP) {
        generateBishopMoves<C, PIECE_TYPE_MASK>(board, moves);
    }

    if constexpr (GEN_ROOK) {
        generateRookMoves<C, PIECE_TYPE_MASK>(board, moves);
    }

    if constexpr (GEN_QUEEN) {
        generateQueenMoves<C, PIECE_TYPE_MASK>(board, moves);
    }

    if constexpr (GEN_KING) {
        generateKingMoves<C, PIECE_TYPE_MASK, LEGAL>(board, moves);
    }
}


} // illumina

#endif // ILLUMINA_MOVEGEN_H
