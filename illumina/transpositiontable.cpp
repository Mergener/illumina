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
    m_key_low = key & 0xFFFFFFFF;
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
    if (entry.key() != key) {
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
    std::memset(m_buf.get(), 0, m_max_entry_count * sizeof(TranspositionTableEntry));
}

inline TranspositionTableEntry& TranspositionTable::entry_ref(ui64 key) {
    return m_buf[key % m_max_entry_count];
}

void TranspositionTable::resize(size_t new_size) {
    try {
        if (new_size == m_size_in_bytes && m_buf != nullptr) {
            return;
        }

        size_t new_n_entries = new_size / sizeof(TranspositionTableEntry);
        auto new_buf         = std::make_unique<TranspositionTableEntry[]>(new_n_entries);
        m_buf                = std::move(new_buf);
        m_max_entry_count    = new_n_entries;
    }
    catch (const std::bad_alloc& bad_alloc) {
        std::cerr << "Failed to resize transposition table, not enough memory." << std::endl;
    }
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

} // illumina