#ifndef ILLUMINA_MOVEPICKER_H
#define ILLUMINA_MOVEPICKER_H

#include "board.h"
#include "staticlist.h"
#include "searchdefs.h"
#include "movegen.h"

namespace illumina {
template <bool QUIESCE = false>
class MovePicker {
public:
    bool finished() const;
    Move next();

    explicit MovePicker(const Board& board,
                        Move hash_move = MOVE_NULL);

private:
    SearchMove   m_moves[MAX_GENERATED_MOVES];
    SearchMove*  m_it;
    SearchMove*  m_end;
    const Board* m_board;
};

template <bool QUIESCE>
inline Move MovePicker<QUIESCE>::next() {
    if (finished()) {
        return MOVE_NULL;
    }

    Move move = Move(*m_it++);
    if (!m_board->in_check() && !m_board->is_move_legal(move)) {
        return next();
    }

    return move;
}

template <bool QUIESCE>
inline bool MovePicker<QUIESCE>::finished() const {
    return m_it >= m_end;
}

template <bool QUIESCE>
inline MovePicker<QUIESCE>::MovePicker(const Board& board, Move hash_move)
    : m_it(m_moves), m_board(&board) {
        if (m_board->in_check()) {
            m_end = generate_moves<QUIESCE ? MoveGenerationType::NOISY : MoveGenerationType::ALL, true>(board, m_moves);
        }
        else {
            m_end = generate_moves<QUIESCE ? MoveGenerationType::NOISY : MoveGenerationType::ALL>(board, m_moves);
        }

        std::sort(m_moves, m_end, [hash_move](SearchMove a, SearchMove b) {
            if (a == hash_move) {
                return true;
            }
            if (b == hash_move) {
                return false;
            }

            return a.is_capture() > b.is_capture();
        });
    }

} // illumina

#endif // ILLUMINA_MOVEPICKER_H
