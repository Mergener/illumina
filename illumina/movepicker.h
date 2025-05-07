#ifndef ILLUMINA_MOVEPICKER_H
#define ILLUMINA_MOVEPICKER_H

#include "board.h"
#include "boardutils.h"
#include "staticlist.h"
#include "searchdefs.h"
#include "movegen.h"
#include "movehistory.h"

#include <iostream>

namespace illumina {

enum {
    MPS_NOT_STARTED = 0,
    MPS_HASH_MOVE = 1,

    // For non-check positions, non-quiesce:
    MPS_PROMOTION_CAPTURES = MPS_HASH_MOVE + 1,
    MPS_PROMOTIONS,
    MPS_GOOD_CAPTURES,
    MPS_EP,
    MPS_KILLER_MOVES,
    MPS_QUIET,
    MPS_BAD_CAPTURES,
    MPS_END_NOT_CHECK,

    // For check positions, non-quiesce:
    MPS_NOISY_EVASIONS = MPS_HASH_MOVE + 1,
    MPS_KILLER_EVASIONS,
    MPS_QUIET_EVASIONS,
    MPS_END_IN_CHECK,
};
using MovePickingStage = int;

template <bool QUIESCE = false>
class MovePicker {
public:
    MovePickingStage stage() const;
    bool finished() const;
    SearchMove next();
    void score_move(SearchMove& move);
    void skip_quiets();

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
    MovePickingStage m_stage = MPS_NOT_STARTED;
    MoveRange        m_curr_move_range;
    SearchMove*      m_moves_it;
    SearchMove*      m_moves_end;
    MoveRange        m_bad_captures_range;
    bool             m_do_quiets = true;

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
    void generate_killer_moves();
    void generate_hash_move();
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
    SearchMove* begin = m_moves_end;
    m_moves_end = generate_moves<MASK, false>(*m_board, m_moves_end);
    m_curr_move_range = { begin, m_moves_end };
}

template<bool QUIESCE>
void MovePicker<QUIESCE>::generate_simple_promotions() {
    constexpr ui64 MASK = BIT(MT_SIMPLE_PROMOTION);
    SearchMove* begin = m_moves_end;
    m_moves_end = generate_moves<MASK, false>(*m_board, m_moves_end);
    m_curr_move_range = { begin, m_moves_end };
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
        SearchMove& m = *it;

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
    m_curr_move_range = { begin, bad_captures_begin };
}

template<bool QUIESCE>
void MovePicker<QUIESCE>::generate_en_passants() {
    constexpr ui64 MASK = BIT(MT_EN_PASSANT);
    SearchMove* begin = m_moves_end;
    m_moves_end = generate_moves<MASK, false>(*m_board, m_moves_end);
    m_curr_move_range = { begin, m_moves_end };
}

template<bool QUIESCE>
void MovePicker<QUIESCE>::generate_quiet_evasions() {
    constexpr ui64 MASK = BIT(MT_NORMAL) | BIT(MT_DOUBLE_PUSH);
    SearchMove* begin = m_moves_end;
    m_moves_end = generate_evasions<MASK>(*m_board, m_moves_end);

    for (auto it = begin; it != m_moves_end; ++it) {
        SearchMove& m = *it;
        score_move(m);
    }

    insertion_sort(begin, m_moves_end, [](SearchMove& a, SearchMove& b) {
        return a.value() > b.value();
    });

    m_curr_move_range = { begin, m_moves_end };
}

template<bool QUIESCE>
void MovePicker<QUIESCE>::generate_noisy_evasions() {
    constexpr ui64 MASK = BIT(MT_SIMPLE_CAPTURE) | BIT(MT_SIMPLE_PROMOTION) | BIT(MT_PROMOTION_CAPTURE) | BIT(MT_EN_PASSANT);
    SearchMove* begin = m_moves_end;
    m_moves_end = generate_evasions<MASK>(*m_board, m_moves_end);

    bool see_table[SQ_COUNT][SQ_COUNT];
    for (auto it = begin; it != m_moves_end; ++it) {
        SearchMove& m = *it;

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
    m_curr_move_range = { begin, m_moves_end };
}

template<bool QUIESCE>
void MovePicker<QUIESCE>::generate_killer_moves() {
    SearchMove* begin = m_moves_end;

    auto& killers = m_mv_hist->killers(m_ply);
    size_t n_killers = 0;
    for (size_t i = 0; i < 2; ++i) {
        Move killer = killers[i];
        if (!m_board->is_move_pseudo_legal(killer)
            || (m_board->in_check() && !m_board->is_move_legal(killer))) {
            continue;
        }
        begin[n_killers++] = killer;
    }

    m_moves_end += n_killers;
    m_curr_move_range = { begin, m_moves_end };
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
    m_curr_move_range = { begin, m_moves_end };
}

template<bool QUIESCE>
void MovePicker<QUIESCE>::generate_hash_move() {
    SearchMove* begin = m_moves_end;
    if (m_hash_move != MOVE_NULL) {
        *m_moves_end++ = m_hash_move;
    }
    m_curr_move_range = { begin, m_moves_end };
}

template <bool QUIESCE>
inline MovePicker<QUIESCE>::MovePicker(const Board& board,
                                       Depth ply,
                                       const MoveHistory& move_hist,
                                       Move hash_move)
    : m_curr_move_range({ &m_moves[0], &m_moves[0] }),
      m_moves_it(&m_moves[0]),
      m_moves_end(&m_moves[0]),
      m_board(&board), m_mv_hist(&move_hist),
      m_end_stage(board.in_check() ? MPS_END_IN_CHECK : MPS_END_NOT_CHECK),
      m_hash_move(hash_move), m_ply(ply) {
}

template<bool QUIESCE>
void MovePicker<QUIESCE>::advance_stage() {
    m_stage += 1;

    if (!m_board->in_check()) {
        switch (m_stage) {
            case MPS_HASH_MOVE:
                generate_hash_move();
                break;

            case MPS_PROMOTION_CAPTURES: {
                generate_promotion_captures();
                break;
            }

            case MPS_PROMOTIONS: {
                generate_simple_promotions();
                break;
            }

            case MPS_GOOD_CAPTURES: {
                generate_simple_captures();
                break;
            }

            case MPS_EP: {
                generate_en_passants();
                break;
            }

            case MPS_BAD_CAPTURES: {
                m_curr_move_range = m_bad_captures_range;
                break;
            }

            case MPS_KILLER_MOVES: {
                if (QUIESCE) {
                    // Skip this stage on quiescence search.
                    advance_stage();
                    return;
                }

                generate_killer_moves();
                break;
            }

            case MPS_QUIET: {
                if constexpr (QUIESCE) {
                    // Always skip this stage on quiescence search.
                    advance_stage();
                    return;
                }

                if (!m_do_quiets) {
                    advance_stage();
                    return;
                }

                generate_quiets();
                break;
            }

            default:
                break;
        }
    }
    else {
        switch (m_stage) {
            case MPS_HASH_MOVE:
                generate_hash_move();
                break;

            case MPS_NOISY_EVASIONS: {
                generate_noisy_evasions();
                break;
            }

            case MPS_KILLER_EVASIONS: {
                if (QUIESCE) {
                    // Skip this stage on quiescence search.
                    advance_stage();
                    return;
                }

                generate_killer_moves();
                break;
            }

            case MPS_QUIET_EVASIONS: {
                if (QUIESCE) {
                    // Skip this stage on quiescence search.
                    advance_stage();
                    return;
                }

                generate_quiet_evasions();
                break;
            }

            default:
                break;
        }
    }

    m_moves_it = m_curr_move_range.begin;
}

template <bool QUIESCE>
inline SearchMove MovePicker<QUIESCE>::next() {
    if (finished()) {
        // We've finished generating moves.
        return MOVE_NULL;
    }

    if (   stage() == MPS_QUIET
        && !m_do_quiets) {
        advance_stage();
        return next();
    }

    if (m_moves_it >= m_curr_move_range.end) {
        // We finished the current stage.
        advance_stage();
        return next();
    }

    SearchMove move = *m_moves_it++;

    // Prevent hash move revisits.
    if (move == m_hash_move) {
        if (m_stage == MPS_HASH_MOVE) {
            return move;
        }
        return next();
    }
    // Prevent killer move revisits.
    if (!QUIESCE
         &&  m_mv_hist->is_killer(m_ply, move)
         && (m_stage != MPS_KILLER_MOVES
         &&  m_stage != MPS_KILLER_EVASIONS)) {
        return next();
    }

    bool legal = m_board->in_check() // We only generate legal evasions during checks.
              || m_board->is_move_legal(move);
    if (!legal || move == MOVE_NULL) {
        // Move isn't legal, try getting the next one.
        return next();
    }

    // Move is legal, we can return it.
    return move;
}

template<bool QUIESCE>
void MovePicker<QUIESCE>::score_move(SearchMove& move) {
    move.set_value(0);
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

        move.add_value(MVV_LVA[move.source_piece().type()][move.captured_piece().type()]);
    }
    else {
        // Adjust score based on move history.
        move.add_value(m_mv_hist->quiet_history(move, m_board->last_move(), m_board->gives_check(move)));
    }
}

template <bool QUIESCE>
inline bool MovePicker<QUIESCE>::finished() const {
    return m_stage >= m_end_stage;
}

template <bool QUIESCE>
inline MovePickingStage MovePicker<QUIESCE>::stage() const {
    return m_stage;
}

template <bool QUIESCE>
inline void MovePicker<QUIESCE>::skip_quiets() {
    m_do_quiets = false;
}

} // illumina

#endif // ILLUMINA_MOVEPICKER_H
