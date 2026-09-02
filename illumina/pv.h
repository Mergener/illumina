#ifndef ILLUMINA_PV_H
#define ILLUMINA_PV_H

#include <cstring>

#include "searchdefs.h"

namespace illumina {

class PvLineView {
    friend class PvTable;
public:
    Move* begin() const;
    Move* end() const;

private:
    Move* m_begin;
    Move* m_end;
    PvLineView(Move* begin, Move* end);
};

class PvTable {
    static constexpr int PADDING = 1;
    static constexpr int MAX_PV_LENGTH = MAX_DEPTH + PADDING;
    static constexpr int SIZE = MAX_PV_LENGTH * (MAX_PV_LENGTH + 1) / 2;

public:
    Move get(int base_ply, int pv_ply) const;
    void set(int base_ply, int pv_ply, Move move);

    PvLineView line(int base_ply);

    void clear();

private:
    Move m_moves[SIZE] {};

    static constexpr int ply_index(int base_ply, int pv_ply);
};

inline Move PvTable::get(int base_ply, int pv_ply) const {
    int idx = ply_index(base_ply, pv_ply);
    return m_moves[idx];
}

inline void PvTable::set(int base_ply, int pv_ply, Move move) {
    int idx = ply_index(base_ply, pv_ply);
    m_moves[idx] = move;
}

inline void PvTable::clear() {
    std::memset(m_moves, 0, SIZE * sizeof(Move));
}

inline PvLineView PvTable::line(int base_ply) {
    return {
        &m_moves[ply_index(base_ply, 0)],
        &m_moves[ply_index(base_ply + 1, 0) - PADDING]
    };
}

inline Move* PvLineView::begin() const {
    return m_begin;
}

inline Move* PvLineView::end() const {
    return m_end;
}

inline PvLineView::PvLineView(Move* begin, Move* end)
    : m_begin(begin), m_end(end) {}

constexpr int PvTable::ply_index(int base_ply, int pv_ply) {
    return MAX_PV_LENGTH * base_ply - base_ply * (base_ply + 1) / 2 + pv_ply;
}

} // namespace illumina

#endif //ILLUMINA_PV_H