#ifndef ILLUMINA_MOVEPICKER_H
#define ILLUMINA_MOVEPICKER_H

#include "board.h"
#include "staticlist.h"
#include "searchdefs.h"
#include "movegen.h"

namespace illumina {

class MoveHistory {
public:
    const std::array<Move, 2>& killers(Depth ply) const;
    bool is_killer(Depth ply, Move move) const;
    void set_killer(Depth ply, Move killer);
    void reset();

private:
    std::array<std::array<Move, 2>, MAX_DEPTH> m_killers {};
};

template <bool QUIESCE = false>
class MovePicker {
public:
    bool finished() const;
    Move next();
    void score_move(SearchMove& move);

    explicit MovePicker(const Board& board,
                        Depth ply,
                        const MoveHistory& move_hist,
                        Move hash_move = MOVE_NULL);

private:
    SearchMove   m_moves[MAX_GENERATED_MOVES];
    Move         m_hash_move;
    size_t       m_count;
    const Board* m_board;
    const MoveHistory* m_mv_hist;
    Depth        m_ply;
};


inline const std::array<Move, 2>& MoveHistory::killers(Depth ply) const {
    return m_killers[ply];
}

inline bool MoveHistory::is_killer(Depth ply, Move move) const {
    const std::array<Move, 2>& arr = killers(ply);
    return arr[0] == move || arr[1] == move;
}

inline void MoveHistory::set_killer(Depth ply, Move killer) {
    std::array<Move, 2>& arr = m_killers[ply];
    arr[1] = arr[0];
    arr[0] = killer;
}

inline void MoveHistory::reset() {
    std::fill(m_killers.begin(), m_killers.end(), std::array<Move, 2> { MOVE_NULL, MOVE_NULL });
}

template <bool QUIESCE>
inline MovePicker<QUIESCE>::MovePicker(const Board& board,
                                       Depth ply,
                                       const MoveHistory& move_hist,
                                       Move hash_move)
    : m_board(&board), m_ply(ply), m_hash_move(hash_move), m_mv_hist(&move_hist) {
    // Start by generating and counting all moves.
    SearchMove* end;
    if (m_board->in_check()) {
        end = generate_moves<QUIESCE ? MoveGenerationType::NOISY : MoveGenerationType::ALL, true>(board, m_moves);
    }
    else {
        end = generate_moves<QUIESCE ? MoveGenerationType::NOISY : MoveGenerationType::ALL>(board, m_moves);
    }
    m_count = end - m_moves;

    // We have generated all moves, now assign scores to them.
    for (SearchMove* it = m_moves; it != end; ++it) {
        score_move(*it);
    }
}

template<bool QUIESCE>
void MovePicker<QUIESCE>::score_move(SearchMove& move) {
    if (move == m_hash_move) {
        // Hash moves always receive maximum value.
        move.set_value(INT32_MAX);
        return;
    }
    if (move.is_promotion()) {
        move.add_value(2048);
    }
    if (move.is_capture()) {
        move.add_value(1024);

        // Perform MMV-LVA: Most valuable victims -> Least valuable attackers
        constexpr i32 MVV_LVA[PT_COUNT][PT_COUNT] {
            /*         x-    xP    xN    xB    xR    xQ    xK  */
            /* -- */ { 0,    0,    0,    0,    0,    0,    0   },
            /* Px */ { 0,   105,  205,  305,  405,  505,  9999 },
            /* Nx */ { 0,   104,  204,  304,  404,  504,  9999 },
            /* Bx */ { 0,   103,  203,  303,  403,  503,  9999 },
            /* Rx */ { 0,   102,  202,  302,  600,  502,  9999 },
            /* Qx */ { 0,   101,  201,  301,  401,  501,  9999 },
            /* Kx */ { 0,   100,  200,  300,  400,  500,  9999 },
        };

        move.add_value(MVV_LVA[move.source()][move.destination()]);
    }

    if (!move.is_promotion() && !move.is_capture()) {
        if (m_mv_hist->is_killer(m_ply, move)) {
            move.add_value(128);
        }
    }
}

template <bool QUIESCE>
inline Move MovePicker<QUIESCE>::next() {
    // Find move with the highest score.
    i32 hi_score         = INT32_MIN;
    size_t best_move_idx = m_count;

    for (size_t i = 0; i < m_count; ++i) {
        SearchMove& move = m_moves[i];
        i32 move_score   = move.value();

        if (move_score > hi_score) {
            hi_score  = move_score;
            best_move_idx = i;
        }
    }

    if (best_move_idx == m_count) {
        // We ran out of moves.
        return MOVE_NULL;
    }

    // We found a move. Copy it and remove it from the
    // remaining moves.
    SearchMove& search_move = m_moves[best_move_idx];
    Move move = search_move;
    std::swap(search_move, m_moves[m_count - 1]);
    m_count--;

    bool legal = m_board->in_check() // We only generate evasions during checks, assume legality.
              || m_board->is_move_legal(move);
    if (legal) {
        // Move is legal, we can return it.
        return move;
    }

    // Move wasn't legal, try getting the next one.
    return next();
}

template <bool QUIESCE>
inline bool MovePicker<QUIESCE>::finished() const {
    return m_count == 0;
}



} // illumina

#endif // ILLUMINA_MOVEPICKER_H
