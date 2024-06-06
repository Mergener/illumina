#ifndef ILLUMINA_MOVEHISTORY_H
#define ILLUMINA_MOVEHISTORY_H

#include <array>

#include "searchdefs.h"
#include "types.h"

namespace illumina {

class MoveHistory {
    using ButterflyHistoryArray = std::array<std::array<int, SQ_COUNT>, SQ_COUNT>;

public:
    const std::array<Move, 2>& killers(Depth ply) const;
    bool is_killer(Depth ply, Move move) const;
    void set_killer(Depth ply, Move killer);
    void reset();

    int  quiet_history(Move move) const;
    void update_quiet_history(Move move, Depth depth, bool good);

private:
    std::array<std::array<Move, 2>, MAX_DEPTH> m_killers {};
    ButterflyHistoryArray m_quiet_history {};

    static int& history_ref(ButterflyHistoryArray& history,
                            Move move);

    static const int& history_ref(const ButterflyHistoryArray& history,
                                  Move move);

    static void update_history(ButterflyHistoryArray& history,
                               Move move,
                               Depth depth,
                               bool good);
};



inline const std::array<Move, 2>& MoveHistory::killers(Depth ply) const {
    return m_killers[ply];
}

inline bool MoveHistory::is_killer(Depth ply, Move move) const {
    const std::array<Move, 2>& arr = killers(ply);
    return arr[0] == move || arr[1] == move;
}

inline void MoveHistory::set_killer(Depth ply, Move killer) {
    std::array<Move, 2>& arr = m_killers[ply];
    if (killer == arr[0]) {
        return;
    }
    arr[1] = arr[0];
    arr[0] = killer;
}

inline void MoveHistory::reset() {
    std::fill(m_killers.begin(), m_killers.end(), std::array<Move, 2> { MOVE_NULL, MOVE_NULL });
}

inline int MoveHistory::quiet_history(Move move) const {
    ILLUMINA_ASSERT(move.is_quiet());
    return history_ref(m_quiet_history, move);
}

inline void MoveHistory::update_quiet_history(Move move,
                                              Depth depth,
                                              bool good) {
    ILLUMINA_ASSERT(move.is_quiet());
    update_history(m_quiet_history, move, depth, good);
}

inline int& MoveHistory::history_ref(ButterflyHistoryArray& history, Move move) {
    return history[move.source()][move.destination()];
}

inline const int& MoveHistory::history_ref(const MoveHistory::ButterflyHistoryArray& history, Move move) {
    return history_ref(const_cast<ButterflyHistoryArray&>(history), move);
}

inline void MoveHistory::update_history(MoveHistory::ButterflyHistoryArray& history,
                                        Move move,
                                        Depth depth,
                                        bool good) {
    int delta = (depth < 12) ? (depth * depth) : (32 * depth * depth);
    int sign  = good ? 1 : -1;
    int& hist = history_ref(history, move);
    hist     += (sign * delta) - std::min(hist, 16384) * delta / 16384;
}

} // illumina

#endif // ILLUMINA_MOVEHISTORY_H
