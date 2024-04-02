#include "transpositiontable.h"

#include <cmath>
#include <cstring>

namespace illumina {

void TranspositionTableEntry::replace(ui64 key,
                                      Move move,
                                      Score score,
                                      Depth depth,
                                      BoundType bound_type,
                                      ui8 generation) {
    m_key   = key;
    m_move  = move;
    m_score = score;

    m_info = 1; // Start with 1 for 'valid' bit.
    m_info |= (bound_type & BITMASK(2)) << 1;
    m_info |= (generation & BITMASK(8)) << 3;
    m_info |= (depth      & BITMASK(8)) << 11;
}

void TranspositionTable::new_search() {
    m_gen++;
}

bool TranspositionTable::probe(ui64 key, TranspositionTableEntry& entry) {
    entry = entry_ref(key);
    if (entry.key() != key) {
        return false;
    }
    if (!entry.valid()) {
        return false;
    }
    return true;
}

void TranspositionTable::try_store(ui64 key,
                                   Move move,
                                   Score score,
                                   Depth depth,
                                   BoundType bound_type) {
    TranspositionTableEntry& entry = entry_ref(key);

    if (!entry.valid() || entry.generation() != m_gen) {
        entry.replace(key, move, score, depth, bound_type, m_gen);
        return;
    }

    if (entry.depth() <= depth &&
        bound_type == BT_EXACT &&
        entry.bound_type() != BT_EXACT) {
        // Replace since we now have an exact score.
        entry.replace(key, move, score, depth, bound_type, m_gen);
        return;
    }

    if (entry.depth() <= depth) {
        // Higher depth, replace it.
        entry.replace(key, move, score, depth, bound_type, m_gen);
        return;
    }
}

void TranspositionTable::clear() {
    std::memset(m_buf.get(), 0, m_n_entries * sizeof(TranspositionTableEntry));
}

inline TranspositionTableEntry& TranspositionTable::entry_ref(ui64 key) {
    return m_buf[key % m_n_entries];
}

void TranspositionTable::resize(size_t new_size) {
    try {
        if (new_size == m_size && m_buf != nullptr) {
            return;
        }

        size_t new_n_entries = new_size / sizeof(TranspositionTableEntry);
        auto new_buf         = std::make_unique<TranspositionTableEntry[]>(new_n_entries);
        m_buf                = std::move(new_buf);
        m_n_entries          = new_n_entries;
    }
    catch (const std::bad_alloc& bad_alloc) {
        std::cerr << "Failed to resize transposition table, not enough memory." << std::endl;
    }
}

TranspositionTable::TranspositionTable(size_t size)
    : m_size(size), m_n_entries(size / sizeof(TranspositionTableEntry)) {
    resize(size);
    clear();
}

} // illumina