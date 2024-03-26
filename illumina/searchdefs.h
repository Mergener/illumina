#ifndef ILLUMINA_SEARCHTYPES_H
#define ILLUMINA_SEARCHTYPES_H

#include "types.h"

namespace illumina {

//
// Numeric types
//

using Score = i32;
using Depth = int;

//
// Constants
//

constexpr Depth MAX_DEPTH        = 128;

/**
 * Limited by the number of bits dedicated to store the score
 * in a TT entry (currently 22).
 */
constexpr Score SCORE_UPPERBOUND = BITMASK(20);
constexpr Score MAX_SCORE        = 32005;
constexpr Score MATE_SCORE       = MAX_SCORE - 1;
constexpr Score MATE_THRESHOLD   = MATE_SCORE - 1024;

static_assert(SCORE_UPPERBOUND < BITMASK(sizeof(Score) * 8));
static_assert(MAX_SCORE        <= SCORE_UPPERBOUND);
static_assert(MATE_SCORE       < MAX_SCORE);
static_assert(MATE_THRESHOLD   < MATE_SCORE);

constexpr Score is_mate_score(Score score) {
    return score >= MATE_THRESHOLD || score <= -MATE_THRESHOLD;
}

constexpr int plies_to_mate(Score score) {
    return std::abs(MATE_SCORE - score);
}

constexpr int moves_to_mate(Score score) {
    return (plies_to_mate(score) + 1) / 2;
}

//
// Node types
//

enum NodeType : ui8 {
    NT_PV,
    NT_ALL,
    NT_CUT
};

//
// Search Moves
//

class SearchMove : public Move {
public:
    SearchMove(const Move& move = MOVE_NULL);
    SearchMove& operator=(const Move& move);
    SearchMove(Move&& move);

    i32 value() const;
    void set_value(i32 val);

private:
    i32 m_val;
};

inline SearchMove::SearchMove(const Move& move)
    : Move(move), m_val(0) {
}

inline SearchMove::SearchMove(Move&& move)
    : Move(move), m_val(0) {
}

inline SearchMove& SearchMove::operator=(const Move& move) {
    m_data = move.raw();
    return *this;
}

inline i32 SearchMove::value() const {
    return m_val;
}

inline void SearchMove::set_value(i32 val) {
    m_val = val;
}

}

#endif // ILLUMINA_SEARCHTYPES_H
