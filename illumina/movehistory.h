#ifndef ILLUMINA_MOVEHISTORY_H
#define ILLUMINA_MOVEHISTORY_H

#include <array>
#include <memory>
#include <cstring>

#include "searchdefs.h"
#include "tunablevalues.h"
#include "types.h"

namespace illumina {

static constexpr int MAX_HISTORY = 16384;
static constexpr size_t CORRHIST_ENTRIES = 16384;
static constexpr int CORRHIST_GRAIN = 256;
static constexpr int CORRHIST_BASE_WEIGHT = 1024;
static constexpr int MAX_CORRHIST = 16384;

template <typename T>
struct ButterflyArray : std::array<std::array<T, SQ_COUNT>, SQ_COUNT> {
    T& get(Move move);
    const T& get(Move move) const;
};

template <typename T>
struct PieceToArray : std::array<std::array<std::array<T, SQ_COUNT>, PT_COUNT>, CL_COUNT> {
    T& get(Move move);
    const T& get(Move move) const;
};

struct CorrhistEntry {
    i16  value;
    ui16 key_low;
};
using CorrhistTable = std::array<std::array<CorrhistEntry, CORRHIST_ENTRIES>, CL_COUNT>;

class MoveHistory {
public:
    const std::array<Move, 2>& killers(Depth ply) const;
    bool is_killer(Depth ply, Move move) const;
    void set_killer(Depth ply, Move killer);
    void reset();

    void update_corrhist(const Board& board,
                         Depth depth,
                         int diff);

    int pawn_corrhist(const Board& board) const;
    int non_pawn_corrhist(const Board& board) const;

    int correct_eval_with_corrhist(const Board& board,
                                   int static_eval) const;

    int  quiet_history(Move move, Move last_move, bool gives_check) const;
    void update_quiet_history(Move move,
                              Move last_move,
                              Depth depth,
                              bool good);

    int capture_history(Move move) const;
    void update_capture_history(Move move, Depth depth, bool good);

    MoveHistory();

private:
    // Search history contains rather heavy array. To prevent unintended
    // stack allocations, all history data is kept in an indirect fashion,
    // with its lifetime managed by a unique_ptr.
    struct Data {
        CorrhistTable pawn_corrhist;
        CorrhistTable non_pawn_corrhist;
        std::array<std::array<Move, 2>, MAX_DEPTH> killers {};
        ButterflyArray<int> main_history {};
        PieceToArray<PieceToArray<int>> counter_move_history {};
        PieceToArray<int> check_history {};
        std::array<PieceToArray<int>, PT_COUNT> capture_history {};
    };
    std::unique_ptr<Data> m_data = std::make_unique<Data>();

    void update_corrhist_entry(CorrhistTable& table,
                               ui64 key,
                               Color color,
                               int depth_score,
                               int diff);

    static void update_history_by_depth(int& history,
                                        Depth depth,
                                        bool good);

    static void update_history(int& history,
                               int bonus);
};

template<typename T>
T& ButterflyArray<T>::get(Move move) {
    return (*this)[move.source()][move.destination()];
}

template<typename T>
const T& ButterflyArray<T>::get(Move move) const {
    return (*this)[move.source()][move.destination()];
}

template<typename T>
T& PieceToArray<T>::get(Move move) {
    return (*this)[move.source_piece().color()][move.source_piece().type()][move.destination()];
}

template<typename T>
const T& PieceToArray<T>::get(Move move) const {
    return (*this)[move.source_piece().color()][move.source_piece().type()][move.destination()];
}

inline const std::array<Move, 2>& MoveHistory::killers(Depth ply) const {
    return m_data->killers[ply];
}

inline bool MoveHistory::is_killer(Depth ply, Move move) const {
    const std::array<Move, 2>& arr = killers(ply);
    return arr[0] == move || arr[1] == move;
}

inline void MoveHistory::set_killer(Depth ply, Move killer) {
    std::array<Move, 2>& arr = m_data->killers[ply];
    if (killer == arr[0]) {
        return;
    }
    arr[1] = arr[0];
    arr[0] = killer;
}

inline void MoveHistory::reset() {
    std::memset(m_data.get(), 0, sizeof(Data));
}

inline int MoveHistory::quiet_history(Move move, Move last_move, bool gives_check) const {
    return int(
               i64(MV_HIST_REGULAR_QHIST_WEIGHT * m_data->main_history.get(move))
             + i64(MV_HIST_COUNTER_MOVE_WEIGHT  * m_data->counter_move_history.get(last_move).get(move)));
}

inline void MoveHistory::update_quiet_history(Move move,
                                              Move last_move,
                                              Depth depth,
                                              bool good) {
    update_history_by_depth(m_data->main_history.get(move), depth, good);
    if (last_move != MOVE_NULL) {
        update_history_by_depth(m_data->counter_move_history.get(last_move).get(move), depth, good);
    }
}

inline int MoveHistory::capture_history(Move move) const {
    return m_data->capture_history[move.captured_piece().type()].get(move);
}

inline void MoveHistory::update_capture_history(Move move, Depth depth, bool good) {
    update_history_by_depth(m_data->capture_history[move.captured_piece().type()].get(move), depth, good);
}

inline void MoveHistory::update_corrhist_entry(CorrhistTable& table,
                                               ui64 key,
                                               Color color,
                                               int depth_score,
                                               int diff) {
    int scaled_diff = diff * CORRHIST_GRAIN;
    CorrhistEntry& entry = table[color][key % CORRHIST_ENTRIES];
    entry.key_low = key & BITMASK(16);
    i16& entry_value = entry.value;
    entry_value = std::clamp((int(entry_value) * (CORRHIST_BASE_WEIGHT - depth_score) + scaled_diff * depth_score) / CORRHIST_BASE_WEIGHT,
                             -MAX_CORRHIST, MAX_CORRHIST);
}

inline void MoveHistory::update_corrhist(const Board& board,
                                         Depth depth,
                                         int diff) {
    Color color = board.color_to_move();
    int depth_score = std::min(depth * depth + 2 * depth + 1, 128);

    // Update pawn corrhist.
    update_corrhist_entry(m_data->pawn_corrhist,
                          board.pawn_key(),
                          color,
                          depth_score,
                          diff);

    // Update non-pawn corrhist.
    update_corrhist_entry(m_data->non_pawn_corrhist,
                          board.non_pawn_key(),
                          color,
                          depth_score,
                          diff);
}

inline int MoveHistory::pawn_corrhist(const Board& board) const {
    CorrhistEntry& entry = m_data->pawn_corrhist[board.color_to_move()]
                                                [board.pawn_key() % CORRHIST_ENTRIES];
    return entry.key_low == (board.pawn_key() & BITMASK(16))
         ? entry.value
         : 0;
}

inline int MoveHistory::non_pawn_corrhist(const Board& board) const {
    CorrhistEntry& entry = m_data->non_pawn_corrhist[board.color_to_move()]
                                                    [board.pawn_key() % CORRHIST_ENTRIES];
    return entry.key_low == (board.non_pawn_key() & BITMASK(16))
         ? entry.value
         : 0;
}

inline int MoveHistory::correct_eval_with_corrhist(const Board& board,
                                                   int static_eval) const {
    if (std::abs(static_eval) >= KNOWN_WIN) {
        return static_eval;
    }

    int unscaled_correction = pawn_corrhist(board)
                            + non_pawn_corrhist(board);

    return std::clamp(static_eval + unscaled_correction / CORRHIST_GRAIN,
                      -KNOWN_WIN, KNOWN_WIN);
}

inline void MoveHistory::update_history_by_depth(int& history,
                                                 Depth depth,
                                                 bool good) {
    int delta = (depth < MV_HIST_QUIET_HIGH_DEPTH_THRESHOLD)
              ? (depth * depth)
              : (MV_HIST_QUIET_HIGH_DEPTH_FACTOR * depth * depth);
    int sign  = good ? 1 : -1;
    update_history(history, sign * delta);
}

inline void MoveHistory::update_history(int& history, int bonus) {
    int clamped_bonus = std::clamp(bonus, -MAX_HISTORY, MAX_HISTORY);
    history          += clamped_bonus - history * std::abs(clamped_bonus) / MAX_HISTORY;
}

inline MoveHistory::MoveHistory() {
    reset();
}

} // illumina

#endif // ILLUMINA_MOVEHISTORY_H
