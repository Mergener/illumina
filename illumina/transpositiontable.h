#ifndef ILLUMINA_TRANSPOSITIONTABLE_H
#define ILLUMINA_TRANSPOSITIONTABLE_H

#include <memory>

#include "searchdefs.h"

namespace illumina {

class TranspositionTableEntry {
    friend class TranspositionTable;
public:
    ui64      key() const;
    Move      move() const;
    BoundType bound_type() const;
    bool      valid() const;
    ui8       generation() const;
    Score     score() const;
    Depth     depth() const;

private:
    ui64 m_key;
    Move m_move;

    // m_info  encoding:
    //  0:     valid
    //  1-2:   bound_type
    //  3-10:  generation
    //  11-18: depth
    ui32 m_info;

    Score m_score;

    void replace(ui64 key, Move move,
                 Score score, Depth depth,
                 BoundType bound_type,
                 ui8 generation);
};

constexpr size_t TT_DEFAULT_SIZE_MB = 8;

class TranspositionTable {
public:
    void clear();
    void resize(size_t new_size_bytes);
    void new_search();
    bool probe(ui64 key, TranspositionTableEntry& entry);
    void try_store(ui64 key, Move move, Score score, Depth depth, BoundType bound_type);

    explicit TranspositionTable(size_t size_bytes = TT_DEFAULT_SIZE_MB * 1024 * 1024);
    ~TranspositionTable() = default;
    TranspositionTable(TranspositionTable&& rhs) = default;
    TranspositionTable(const TranspositionTable& rhs) = delete;
    TranspositionTable& operator=(const TranspositionTable& rhs) = delete;

private:
    std::unique_ptr<TranspositionTableEntry[]> m_buf = nullptr;
    size_t m_size;
    size_t m_n_entries;
    ui8 m_gen = 0;

    TranspositionTableEntry& entry_ref(ui64 key);
};

inline ui64 TranspositionTableEntry::key() const {
    return m_key;
}

inline Move TranspositionTableEntry::move() const {
    return m_move;
}

inline bool TranspositionTableEntry::valid() const {
    return m_info & 1;
}

inline BoundType TranspositionTableEntry::bound_type() const {
    return BoundType((m_info >> 1) & BITMASK(2));
}

inline ui8 TranspositionTableEntry::generation() const {
    return (m_info >> 3) & BITMASK(8);
}

inline Depth TranspositionTableEntry::depth() const {
    return (m_info >> 11) & BITMASK(8);
}

inline Score TranspositionTableEntry::score() const {
    return m_score;
}

} // illumina

#endif // ILLUMINA_TRANSPOSITIONTABLE_H
