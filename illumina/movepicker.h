#ifndef ILLUMINA_MOVEPICKER_H
#define ILLUMINA_MOVEPICKER_H

#include "board.h"
#include "boardutils.h"
#include "staticlist.h"
#include "searchdefs.h"
#include "movegen.h"

namespace illumina {

class MoveHistory {
public:
    const std::array<Move, 2>& killers(Depth ply) const;
    bool is_killer(Depth ply, Move move) const;
    void set_killer(Depth ply, Move killer);
    void reset();

    int get_history_score(Move move) const;
    void increment_history_score(Move move, Depth depth);

private:
    std::array<std::array<Move, 2>, MAX_DEPTH> m_killers {};
    std::array<std::array<int, SQ_COUNT>, SQ_COUNT> m_history {};
};

enum {
    MCS_HASH_MOVE,

    // For non-check positions:
    MCS_PROMOTION_CAPTURES = MCS_HASH_MOVE + 1,
    MCS_PROMOTIONS,
    MCS_GOOD_CAPTURES,
    MCS_EP,
    MCS_KILLER_MOVES,
    MCS_BAD_CAPTURES,
    MCS_QUIET,
    MCS_END_NOT_CHECK,

    // For check positions:
    MCS_NOISY_EVASIONS = MCS_HASH_MOVE + 1,
    MCS_QUIET_EVASIONS,
    MCS_END_IN_CHECK,
};
using MovePickingStage = int;

template <bool QUIESCE = false>
class MovePicker {
public:
    bool finished() const;
    Move next();
    void score_move(SearchMove& move);

    explicit MovePicker(const Board& board,
                        Depth ply,
                        const MoveHistory& move_hist,
                        Move hash_move = MOVE_NULL);

private:
    struct MoveRange {
        SearchMove* begin;
        SearchMove* end;
    };

    // Move generation state
    SearchMove       m_moves[MAX_GENERATED_MOVES];
    MovePickingStage m_stage;
    MoveRange        m_curr_move_range;
    SearchMove*      m_moves_end;
    MoveRange        m_bad_captures_range { nullptr, nullptr };
    size_t           m_n_visited_killers;

    // Context-related fields
    const Board* m_board;
    const MoveHistory* m_mv_hist;
    const MovePickingStage m_end_stage;
    const Move  m_hash_move;
    const Depth m_ply;

    void advance_stage();

    void generate_promotion_captures();
    void generate_simple_promotions();
    void generate_simple_captures();
    void generate_en_passants();
    void generate_quiets();
    void generate_quiet_evasions();
    void generate_noisy_evasions();
};

template <typename TIter, typename TCompar>
void insertion_sort(TIter begin, TIter end, TCompar comp) {
    if (begin >= end) {
        return;
    }

    for (TIter i = begin + 1; i != end; ++i) {
        typename std::iterator_traits<TIter>::value_type key = *i;
        TIter j = i - 1;

        while (j >= begin && comp(key, *j)) {
            *(j + 1) = std::move(*j);
            --j;
        }

        *(j + 1) = std::move(key);
    }
}

template<bool QUIESCE>
void MovePicker<QUIESCE>::generate_promotion_captures() {
    constexpr ui64 MASK = BIT(MT_PROMOTION_CAPTURE);
    m_moves_end = generate_moves<MASK, false>(*m_board, m_moves_end);
}

template<bool QUIESCE>
void MovePicker<QUIESCE>::generate_simple_promotions() {
    constexpr ui64 MASK = BIT(MT_SIMPLE_PROMOTION);
    m_moves_end = generate_moves<MASK, false>(*m_board, m_moves_end);
}

template<bool QUIESCE>
void MovePicker<QUIESCE>::generate_simple_captures() {
    constexpr ui64 MASK = BIT(MT_SIMPLE_CAPTURE);

    SearchMove* begin = m_moves_end;
    SearchMove* bad_captures_begin = begin;
    m_moves_end = generate_moves<MASK, false>(*m_board, m_moves_end);


    // For simple captures, we need to classify them as good or bad.
    // Good captures are captures with static exchange evaluation
    // superior or equal to zero.
    bool see_table[SQ_COUNT][SQ_COUNT];
    for (auto it = begin; it != m_moves_end; ++it) {
        SearchMove m = *it;

        bool good_see = has_good_see(*m_board, m.source(), m.destination());
        see_table[m.source()][m.destination()] = good_see;
        score_move(m);

        if (good_see) {
            bad_captures_begin++;
        }
    }

    insertion_sort(begin, m_moves_end, [&see_table](SearchMove& a, SearchMove& b) {
        bool a_good_see = see_table[a.source()][a.destination()];
        bool b_good_see = see_table[b.source()][b.destination()];
        if (a_good_see && !b_good_see) {
            return true;
        }
        if (!a_good_see && b_good_see) {
            return false;
        }

        return a.value() > b.value();
    });

    m_bad_captures_range = { bad_captures_begin, m_moves_end };
}

template<bool QUIESCE>
void MovePicker<QUIESCE>::generate_en_passants() {
    constexpr ui64 MASK = BIT(MT_EN_PASSANT);
    m_moves_end = generate_moves<MASK, false>(*m_board, m_moves_end);
}

template<bool QUIESCE>
void MovePicker<QUIESCE>::generate_quiet_evasions() {
    constexpr ui64 MASK = BIT(MT_NORMAL) | BIT(MT_DOUBLE_PUSH);
    SearchMove* begin = m_moves_end;
    m_moves_end = generate_evasions<MASK>(*m_board, m_moves_end);

    for (auto it = begin; it != m_moves_end; ++it) {
        SearchMove m = *it;
        score_move(m);
    }
}

template<bool QUIESCE>
void MovePicker<QUIESCE>::generate_noisy_evasions() {
    constexpr ui64 MASK = BIT(MT_SIMPLE_CAPTURE) | BIT(MT_SIMPLE_PROMOTION) | BIT(MT_PROMOTION_CAPTURE) | BIT(MT_EN_PASSANT);
    SearchMove* begin = m_moves_end;
    m_moves_end = generate_evasions<MASK>(*m_board, m_moves_end);

    bool see_table[SQ_COUNT][SQ_COUNT];
    for (auto it = begin; it != m_moves_end; ++it) {
        SearchMove m = *it;

        bool good_see = has_good_see(*m_board, m.source(), m.destination());
        see_table[m.source()][m.destination()] = good_see;
        score_move(m);
    }

    insertion_sort(begin, m_moves_end, [&see_table](SearchMove& a, SearchMove& b) {
        bool a_is_prom = a.is_promotion();
        bool b_is_prom = b.is_promotion();
        if (a_is_prom && !b_is_prom) {
            return true;
        }
        if (!a_is_prom && b_is_prom) {
            return false;
        }

        bool a_good_see = see_table[a.source()][a.destination()];
        bool b_good_see = see_table[b.source()][b.destination()];
        if (a_good_see && !b_good_see) {
            return true;
        }
        if (!a_good_see && b_good_see) {
            return false;
        }

        return a.value() > b.value();
    });
}

template<bool QUIESCE>
void MovePicker<QUIESCE>::generate_quiets() {
    constexpr ui64 MASK = BIT(MT_NORMAL) | BIT(MT_DOUBLE_PUSH) | BIT(MT_CASTLES);
    SearchMove* begin = m_moves_end;
    m_moves_end = generate_moves<MASK, false>(*m_board, begin);

    for (auto it = begin; it != m_moves_end; ++it) {
        score_move(*it);
    }

    insertion_sort(begin, m_moves_end, [](SearchMove& a, SearchMove& b) {
        return a.value() > b.value();
    });
}

inline const std::array<Move, 2>& MoveHistory::killers(Depth ply) const {
    return m_killers[ply];
}

inline bool MoveHistory::is_killer(Depth ply, Move move) const {
    const std::array<Move, 2>& arr = killers(ply);
    return arr[0] == move || arr[1] == move;
}

inline void MoveHistory::set_killer(Depth ply, Move killer) {
    std::array<Move, 2>& arr = m_killers[ply];
    arr[1] = arr[0];
    arr[0] = killer;
}

inline void MoveHistory::reset() {
    std::fill(m_killers.begin(), m_killers.end(), std::array<Move, 2> { MOVE_NULL, MOVE_NULL });
}

inline int MoveHistory::get_history_score(Move move) const {
    return m_history[move.source()][move.destination()];
}

inline void MoveHistory::increment_history_score(Move move, Depth depth) {
    m_history[move.source()][move.destination()] += depth * depth;
}

template <bool QUIESCE>
inline MovePicker<QUIESCE>::MovePicker(const Board& board,
                                       Depth ply,
                                       const MoveHistory& move_hist,
                                       Move hash_move)
    : m_board(&board), m_ply(ply),
      m_hash_move(hash_move), m_mv_hist(&move_hist),
      m_end_stage(board.in_check() ? MCS_END_IN_CHECK : MCS_END_NOT_CHECK) {
}

template<bool QUIESCE>
void MovePicker<QUIESCE>::advance_stage() {
    m_stage += 1;

    if (m_board->in_check()) {
        switch (m_stage) {
            case MCS_KILLER_MOVES: {
                m_n_visited_killers = 0;
                break;
            }

            case MCS_PROMOTION_CAPTURES: {
                SearchMove* begin = m_moves_end;
                generate_promotion_captures();
                m_curr_move_range = { begin, m_moves_end };
                break;
            }

            case MCS_PROMOTIONS: {
                SearchMove* begin = m_moves_end;
                generate_simple_promotions();
                m_curr_move_range = { begin, m_moves_end };
                break;
            }

            case MCS_GOOD_CAPTURES: {
                SearchMove* begin = m_moves_end;
                generate_simple_captures();
                m_curr_move_range = { begin, m_bad_captures_range.begin };
                break;
            }

            case MCS_EP: {
                SearchMove* begin = m_moves_end;
                generate_en_passants();
                m_curr_move_range = { begin, m_moves_end };
                break;
            }

            case MCS_BAD_CAPTURES: {
                m_curr_move_range = m_bad_captures_range;
                break;
            }

            case MCS_QUIET: {
                SearchMove* begin = m_moves_end;
                generate_quiets();
                m_curr_move_range = { begin, m_moves_end };
                break;
            }

            default:
                break;
        }
    }
    else {
        switch (m_stage) {
            case MCS_NOISY_EVASIONS: {
                SearchMove* begin = m_moves_end;
                generate_noisy_evasions(begin);
                m_curr_move_range = { begin, m_moves_end };
                break;
            }

            case MCS_QUIET_EVASIONS: {
                SearchMove* begin = m_moves_end;
                generate_quiet_evasions(begin);
                m_curr_move_range = { begin, m_moves_end };
                break;
            }

            default:
                break;
        }
    }
}

template <bool QUIESCE>
inline Move MovePicker<QUIESCE>::next() {
    Move move = MOVE_NULL;

    while (move == MOVE_NULL) {
        switch (m_stage) {
            case MCS_KILLER_MOVES:

        }
        advance_stage();
        if (m_stage >= m_end_stage) {
            // We've finished generating moves.
            return MOVE_NULL;
        }
    }

    bool legal = m_board->in_check() // We only generate legal evasions during checks.
              || m_board->is_move_legal(move);
    if (legal) {
        // Move is legal, we can return it.
        return move;
    }

    // Move wasn't legal, try getting the next one.
    return next();

}

template<bool QUIESCE>
void MovePicker<QUIESCE>::score_move(SearchMove& move) {
    if (move.is_capture()) {
        // Perform MMV-LVA: Most valuable victims -> Least valuable attackers
        constexpr i32 MVV_LVA[PT_COUNT][PT_COUNT] {
            /*         x-    xP    xN    xB    xR    xQ    xK  */
            /* -- */ { 0,    0,    0,    0,    0,    0,    0   },
            /* Px */ { 0,   105,  205,  305,  405,  505,  9999 },
            /* Nx */ { 0,   104,  204,  304,  404,  504,  9999 },
            /* Bx */ { 0,   103,  203,  303,  403,  503,  9999 },
            /* Rx */ { 0,   102,  202,  302,  600,  502,  9999 },
            /* Qx */ { 0,   101,  201,  301,  401,  501,  9999 },
            /* Kx */ { 0,   100,  200,  300,  400,  500,  9999 },
        };

        move.add_value(MVV_LVA[move.source()][move.destination()]);
    }
    else {
        move.add_value(m_mv_hist->get_history_score(move));
    }
}

template <bool QUIESCE>
inline bool MovePicker<QUIESCE>::finished() const {
    return m_stage >= m_end_stage;
}



} // illumina

#endif // ILLUMINA_MOVEPICKER_H
