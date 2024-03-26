#ifndef ILLUMINA_SEARCHTYPES_H
#define ILLUMINA_SEARCHTYPES_H

#include "types.h"

namespace illumina {

//
// Numeric types
//

using Score = int;
using Depth = int;

//
// Constants
//

constexpr Depth MAX_DEPTH      = 128;
constexpr Score MAX_SCORE      = 32005;
constexpr Score MATE_SCORE     = MAX_SCORE - 1;
constexpr Score MATE_THRESHOLD = MATE_SCORE - 1024;

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
