#include "transpositiontable.h"

#include <cmath>
#include <cstring>

namespace illumina {

void TranspositionTableEntry::replace(ui64 key,
                                      Move move,
                                      Score score,
                                      Depth depth,
                                      NodeType node_type,
                                      ui8 generation) {
    m_key   = key;
    m_move  = move;
    m_score = score;

    m_info = 1; // Start with 1 for 'valid' bit.
    m_info |= (node_type  & BITMASK(2)) << 1;
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
                                   NodeType node_type) {
    TranspositionTableEntry& entry = entry_ref(key);

    if (!entry.valid() || entry.generation() != m_gen) {
        entry.replace(key, move, score, depth, node_type, m_gen);
        return;
    }

    if (entry.depth() <= depth &&
        node_type == NT_PV     &&
        entry.node_type() != NT_PV) {
        // Replace since we now have an exact score.
        entry.replace(key, move, score, depth, node_type, m_gen);
        return;
    }

    if (entry.depth() <= depth) {
        // Higher depth, replace it.
        entry.replace(key, move, score, depth, node_type, m_gen);
        return;
    }
}

void TranspositionTable::clear() {
    m_buf = std::make_unique<TranspositionTableEntry[]>(m_size);
    std::memset(m_buf.get(), 0, m_size * sizeof(TranspositionTableEntry));
}

inline TranspositionTableEntry& TranspositionTable::entry_ref(ui64 key) {
    return m_buf[key % m_size];
}

TranspositionTable::TranspositionTable(size_t size)
    : m_size(size) {
    m_buf = std::make_unique<TranspositionTableEntry[]>(m_size);
    std::memset(m_buf.get(), 0, m_size * sizeof(TranspositionTableEntry));
}

} // illumina