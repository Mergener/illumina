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
    Score     static_eval() const;
    bool      ttpv() const;

private:
    ui32 m_key_low;
    ui32 m_key_hi;
    Move m_move;

    // m_info  encoding:
    //  0:     valid
    //  1-2:   bound_type
    //  3-10:  generation
    //  11-18: depth
    //  19:    ttpv
    ui32 m_info;

    i16 m_score;
    i16 m_static_eval;

    void replace(ui64 key, Move move,
                 Score score, Depth depth,
                 i16 static_eval,
                 BoundType bound_type,
                 ui8 generation,
                 bool ttpv);
};

constexpr size_t TT_DEFAULT_SIZE_MB = 32;

class TranspositionTable {
public:
    void clear();
    size_t size() const;
    void resize(size_t new_size_bytes);
    void new_search();
    bool probe(ui64 key, TranspositionTableEntry& entry, Depth ply = 0);
    void try_store(ui64 key,
                   Depth ply,
                   Move move,
                   Score score,
                   Depth depth,
                   Score static_eval,
                   BoundType bound_type,
                   bool ttpv);
    int hash_full() const;
    void prefetch(ui64 zob) const;

    explicit TranspositionTable(size_t size_bytes = TT_DEFAULT_SIZE_MB * 1024 * 1024);
    ~TranspositionTable() = default;
    TranspositionTable(TranspositionTable&& rhs) = default;
    TranspositionTable(const TranspositionTable& rhs) = delete;
    TranspositionTable& operator=(const TranspositionTable& rhs) = delete;

private:
    std::unique_ptr<TranspositionTableEntry[]> m_buf = nullptr;
    size_t m_size_in_bytes;
    size_t m_max_entry_count;
    ui8 m_gen = 0;

    TranspositionTableEntry& entry_ref(ui64 key);
    const TranspositionTableEntry& entry_ref(ui64 key) const;
};

inline ui64 TranspositionTableEntry::key() const {
    return ui64(m_key_low) | (ui64(m_key_hi) << 32);
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

inline bool TranspositionTableEntry::ttpv() const {
    return (m_info >> 19) & BITMASK(1);
}

inline Score TranspositionTableEntry::score() const {
    return m_score;
}

inline Score TranspositionTableEntry::static_eval() const {
    return m_static_eval;
}

inline size_t TranspositionTable::size() const {
    return m_size_in_bytes;
}

inline const TranspositionTableEntry& TranspositionTable::entry_ref(ui64 key) const {
    return m_buf[key % m_max_entry_count];
}

inline void TranspositionTable::prefetch(ui64 zob) const {
    __builtin_prefetch(&entry_ref(zob));
}

} // illumina

#endif // ILLUMINA_TRANSPOSITIONTABLE_H
