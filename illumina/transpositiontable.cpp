#include "transpositiontable.h"

#include <cmath>
#include <cstring>

namespace illumina {

static Score search_score_to_tt(Score search_score, Depth ply) {
    if (search_score >= MATE_THRESHOLD) {
        return search_score + ply;
    }
    if (search_score <= -MATE_THRESHOLD) {
        return search_score - ply;
    }
    return search_score;
}

static Score tt_score_to_search(Score tt_score, Depth ply) {
    if (tt_score >= MATE_THRESHOLD) {
        return tt_score - ply;
    }
    if (tt_score <= -MATE_THRESHOLD) {
        return tt_score + ply;
    }
    return tt_score;
}

void TranspositionTableEntry::replace(ui64 key,
                                      Move move,
                                      Score score,
                                      Depth depth,
                                      i16 static_eval,
                                      BoundType bound_type,
                                      ui8 generation,
                                      bool ttpv) {
    m_key_hi  = key >> 32;
    m_move  = move;
    m_score = score;
    m_static_eval = static_eval;

    m_info = 1; // Start with 1 for 'valid' bit.
    m_info |= (bound_type & BITMASK(2)) << 1;
    m_info |= (generation & BITMASK(8)) << 3;
    m_info |= (depth      & BITMASK(8)) << 11;
    m_info |= (ttpv       & BITMASK(1)) << 19;
}

void TranspositionTable::new_search() {
    m_gen++;
}

bool TranspositionTable::probe(ui64 key, TranspositionTableEntry& entry, Depth ply) {
    entry = entry_ref(key);
    if (entry.key_hi() != (key >> 32)) {
        return false;
    }
    if (!entry.valid()) {
        return false;
    }
    // We've got a valid entry, fix its score.
    entry.m_score = tt_score_to_search(entry.score(), ply);
    return true;
}

void TranspositionTable::try_store(ui64 key,
                                   Depth ply,
                                   Move move,
                                   Score score,
                                   Depth depth,
                                   Score static_eval,
                                   BoundType bound_type,
                                   bool ttpv) {
    TranspositionTableEntry& entry = entry_ref(key);

    // Always add when no existing entry is found.
    if (!entry.valid()) {
        entry.replace(key, move, search_score_to_tt(score, ply), depth, static_eval, bound_type, m_gen, ttpv);
        return;
    }

    // Always replace when current entry has no stored move.
    if (entry.move() == MOVE_NULL && move != MOVE_NULL) {
        entry.replace(key, move, search_score_to_tt(score, ply), depth, static_eval, bound_type, m_gen, ttpv);
        return;
    }

    // Never replace a tt entry with a move for another without a move.
    if (entry.move() != MOVE_NULL && move == MOVE_NULL) {
        return;
    }

    // Always replace older generations.
    if (entry.generation() != m_gen) {
        entry.replace(key, move, search_score_to_tt(score, ply), depth, static_eval, bound_type, m_gen, ttpv);
        return;
    }

    // Always replace when we get a higher depth (with a move assigned).
    if (depth > entry.depth()) {
        entry.replace(key, move, search_score_to_tt(score, ply), depth, static_eval, bound_type, m_gen, ttpv);
        return;
    }

    // Replace when we're getting a more accurate score on the same depth.
    if (depth == entry.depth()
        && ((bound_type == BT_EXACT      && entry.bound_type() != BT_EXACT)
        ||  (bound_type != BT_UPPERBOUND && entry.bound_type() == BT_UPPERBOUND))) {
        entry.replace(key, move, search_score_to_tt(score, ply), depth, static_eval, bound_type, m_gen, ttpv);
        return;
    }
}

void TranspositionTable::clear() {
    if (m_buf == nullptr) {
        return;
    }
    std::memset(m_buf, 0, m_max_entry_count * sizeof(TranspositionTableEntry));
}

inline TranspositionTableEntry& TranspositionTable::entry_ref(ui64 key) {
    return m_buf[key % m_max_entry_count];
}

void TranspositionTable::resize(size_t requested_size) {
    size_t new_n_entries = requested_size / sizeof(TranspositionTableEntry);
    size_t new_size = new_n_entries * sizeof(TranspositionTableEntry);
    if (new_size == m_size_in_bytes && m_buf != nullptr) {
        return;
    }

    auto new_buf = static_cast<TranspositionTableEntry*>(aligned_alloc(CACHE_LINE_SIZE, new_size));
    if (new_buf == nullptr) {
        std::cerr << "Failed to resize transposition table, not enough memory." << std::endl;
        return;
    }
    if (m_buf != nullptr) {
        size_t smaller_size = std::min(m_size_in_bytes, new_size);
        std::memcpy(new_buf, m_buf, smaller_size);
        if (smaller_size < new_size) {
            std::memset(&new_buf[m_max_entry_count], 0, new_size - m_size_in_bytes);
        }
        aligned_free(m_buf);
    }
    m_buf = new_buf;
    m_max_entry_count = new_n_entries;
    m_size_in_bytes = new_size;
}

int TranspositionTable::hash_full() const {
    constexpr size_t SAMPLE_SIZE = 1000;
    int filled = 0;
    for (size_t i = 0; i < SAMPLE_SIZE; ++i) {
        TranspositionTableEntry& entry = m_buf[i];
        if (entry.valid()) {
            filled += 1000;
        }
    }
    return filled / SAMPLE_SIZE;
}

TranspositionTable::TranspositionTable(size_t size)
    : m_size_in_bytes(size), m_max_entry_count(size / sizeof(TranspositionTableEntry)) {
    resize(size);
    clear();
}

TranspositionTable::~TranspositionTable() {
    if (m_buf != nullptr) {
        aligned_free(m_buf);
    }
}

TranspositionTable::TranspositionTable(TranspositionTable &&rhs) noexcept
    : m_buf(rhs. m_buf),
    m_size_in_bytes(rhs.m_size_in_bytes),
    m_max_entry_count(rhs.m_max_entry_count),
    m_gen(rhs.m_gen) {
    rhs.m_buf = nullptr;
}
} // illumina