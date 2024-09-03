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

class MoveHistory {
public:
    const std::array<Move, 2>& killers(Depth ply) const;
    bool is_killer(Depth ply, Move move) const;
    void set_killer(Depth ply, Move killer);
    void reset();

    int  quiet_history(Move move, Move last_move, bool gives_check) const;
    void update_quiet_history(Move move,
                              Move last_move,
                              Depth depth,
                              bool gives_check,
                              bool good);

    MoveHistory();

private:
    // Search history contains rather heavy array. To prevent unintended
    // stack allocations, all history data is kept in an indirect fashion,
    // with its lifetime managed by a unique_ptr.
    struct Data {
        std::array<std::array<Move, 2>, MAX_DEPTH> m_killers {};
        ButterflyArray<int> m_quiet_history {};
        PieceToArray<PieceToArray<int>> m_counter_move_history {};
        PieceToArray<int> m_check_history {};
    };
    std::unique_ptr<Data> m_data = std::make_unique<Data>();

    static void update_history(int& history,
                               Depth depth,
                               bool good);
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
    return m_data->m_killers[ply];
}

inline bool MoveHistory::is_killer(Depth ply, Move move) const {
    const std::array<Move, 2>& arr = killers(ply);
    return arr[0] == move || arr[1] == move;
}

inline void MoveHistory::set_killer(Depth ply, Move killer) {
    std::array<Move, 2>& arr = m_data->m_killers[ply];
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
               i64(MV_HIST_REGULAR_QHIST_WEIGHT * m_data->m_quiet_history.get(move))
             + i64(MV_HIST_COUNTER_MOVE_WEIGHT  * m_data->m_counter_move_history.get(last_move).get(move))
             + i64(MV_HIST_CHECK_QHIST_WEIGHT   * (gives_check ? m_data->m_check_history.get(move) : 0))
             ) / 1024;
}

inline void MoveHistory::update_quiet_history(Move move,
                                              Move last_move,
                                              Depth depth,
                                              bool gives_check,
                                              bool good) {
    update_history(m_data->m_quiet_history.get(move), depth, good);
    if (last_move != MOVE_NULL) {
        update_history(m_data->m_counter_move_history.get(last_move).get(move), depth, good);
    }

    if (gives_check) {
        update_history(m_data->m_check_history.get(move), depth, good);
    }
}

inline void MoveHistory::update_history(int& history,
                                        Depth depth,
                                        bool good) {
    int delta = (depth < MV_HIST_QUIET_HIGH_DEPTH_THRESHOLD)
              ? (depth * depth)
              : (MV_HIST_QUIET_HIGH_DEPTH_FACTOR * depth * depth);
    int sign  = good ? 1 : -1;
    history  += (sign * delta) - std::min(history, MAX_HISTORY) * delta / MAX_HISTORY;
}

inline MoveHistory::MoveHistory() {
    reset();
}

} // illumina

#endif // ILLUMINA_MOVEHISTORY_H
