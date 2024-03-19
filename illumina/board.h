#ifndef ILLUMINA_BOARD_H
#define ILLUMINA_BOARD_H

#include <array>
#include <string_view>
#include <vector>

#include "attacks.h"
#include "types.h"
#include "zobrist.h"

namespace illumina {

constexpr ui64 EMPTY_BOARD_HASH_KEY = 1;

class Board {
public:
    Color    color_to_move() const;
    Bitboard occupancy() const;
    Bitboard piece_bb(Piece piece) const;
    Bitboard color_bb(Color color) const; // TODO
    Bitboard piece_attacks(Piece piece) const; // TODO
    Bitboard color_attacks(Color color) const; // TODO
    Piece    piece_at(Square s) const;
    Square   ep_square() const;
    bool     frc() const;
    bool     in_check() const;
    bool     in_double_check() const;
    ui64     hash_key() const;
    Square   castle_rook_square(Color color, Side side) const;
    int      rule50() const;
    bool     legal() const; // TODO
    int      ply_count() const;
    bool     has_castling_rights(Color color, Side side) const;
    CastlingRights castling_rights() const;
    Move     last_move() const; // TODO
    Bitboard pinned_bb() const; // TODO
    Square   pinner_square(Square pinned_sq) const; // TODO

    void set_piece_at(Square s, Piece p);
    void put_piece(Square s, Piece p); // TODO
    void remove_piece(Square s); // TODO
    void set_color_to_move(Color c);
    void set_ep_square(Square s);

    void set_castling_rights(CastlingRights castling_rights);
    void set_castling_rights(Color color, Side side, bool allow);
    void make_move(Move move); // TODO
    void undo_move(); // TODO
    void make_null_move(); // TODO
    void undo_null_move(); // TODO
    bool is_move_pseudo_legal(Move move) const; // TODO
    bool is_move_legal(Move move) const; // TODO

    Square king_square(Color color) const;
    template <bool EXCLUDE_KING_ATKS = false>
    Square first_attacker_of(Color c, Square s) const;

    template <bool EXCLUDE_KING_ATKS = false>
    Bitboard all_attackers_of(Color c, Square s) const;

    template <PieceType pt>
    Bitboard all_attackers_of_type(Color c, Square s) const;

    Board() = default;
    Board(std::string_view fen_str, bool force_frc = false);
    ~Board() = default;
    Board(Board&& rhs) = default;
    Board& operator=(const Board& rhs) = default;

private:
    std::array<Piece, SQ_COUNT> m_pieces {};
    std::array<std::array<Bitboard, PT_COUNT>, CL_COUNT> m_bbs {};
    Color m_ctm = CL_WHITE;
    Bitboard m_occ = 0;
    Bitboard m_pinned = 0;
    int m_base_ply_count = 0;

    std::array<std::array<Square, SIDE_COUNT>, CL_COUNT> m_castle_rook_squares = {
        std::array<Square, SIDE_COUNT> {
            standard_castle_rook_src_square(CL_WHITE, SIDE_KING),
            standard_castle_rook_src_square(CL_WHITE, SIDE_QUEEN)
        },
        std::array<Square, SIDE_COUNT> {
            standard_castle_rook_src_square(CL_BLACK, SIDE_KING),
            standard_castle_rook_src_square(CL_BLACK, SIDE_QUEEN)
        },
    };

    struct State {
        Move last_move   = MOVE_NULL;
        Square ep_square = SQ_NULL;
        ui64 hash_key    = EMPTY_BOARD_HASH_KEY;
        ui16 rule50      = 0;
        ui8 n_checkers   = 0;
        std::array<std::array<Bitboard, PT_COUNT>, CL_COUNT> attacks {};
        CastlingRights castle_rights = CR_NONE;
    };

    std::vector<State> m_prev_states;
    State m_state {};

    bool m_frc = false;

    Bitboard& piece_bb_ref(Piece piece);
    Bitboard& color_bb_ref(Color color);

    template <bool DO_ZOB>
    void put_piece_internal(Square s, Piece p);

    template <bool DO_ZOB>
    void remove_piece_internal(Square s);
};

inline Color Board::color_to_move() const {
    return m_ctm;
}

inline Bitboard Board::occupancy() const {
    return m_occ;
}

inline Bitboard Board::piece_bb(Piece piece) const {
    return m_bbs[piece.color()][piece.type()];
}

inline Piece Board::piece_at(Square s) const {
    return m_pieces[s];
}

inline Square Board::ep_square() const {
    return m_state.ep_square;
}

inline int Board::rule50() const {
    return m_state.rule50;
}

inline bool Board::frc() const {
    return m_frc;
}

inline bool Board::in_check() const {
    return m_state.n_checkers > 0;
}

inline bool Board::in_double_check() const {
    return m_state.n_checkers >= 2;
}

inline ui64 Board::hash_key() const {
    return m_state.hash_key;
}

inline int Board::ply_count() const {
    return m_base_ply_count + int(m_prev_states.size());
}

inline Square Board::castle_rook_square(Color color, Side side) const {
    return m_castle_rook_squares[color][side];
}

inline bool Board::has_castling_rights(Color color, Side side) const {
    ui8 idx = color * 2 + ui8(side);
    return (castling_rights() & BIT(idx)) != 0;
}

inline CastlingRights Board::castling_rights() const {
    return m_state.castle_rights;
}

inline void Board::set_piece_at(Square s, Piece p) {
    ILLUMINA_ASSERT_VALID_SQUARE(s);
    if (p != PIECE_NULL) {
        put_piece(s, p);
    }
    else {
        remove_piece(s);
    }
}

inline void Board::put_piece(Square s, Piece p) {
    ILLUMINA_ASSERT_VALID_SQUARE(s);
    ILLUMINA_ASSERT_VALID_PIECE_TYPE(p.type());

    put_piece_internal<true>(s, p);
}

inline void Board::remove_piece(Square s) {
    ILLUMINA_ASSERT_VALID_SQUARE(s);
    remove_piece_internal<true>(s);
}

template <bool DO_ZOB>
inline void Board::put_piece_internal(Square s, Piece p) {
    Piece prev_piece        = piece_at(s);

    Bitboard& prev_color_bb = color_bb_ref(prev_piece.color());
    Bitboard& new_color_bb  = color_bb_ref(p.color());
    Bitboard& prev_piece_bb = piece_bb_ref(prev_piece);
    Bitboard& new_piece_bb  = piece_bb_ref(p);

    prev_piece_bb = unset_bit(prev_piece_bb, s);
    prev_piece_bb = unset_bit(prev_color_bb, s);
    new_piece_bb  = set_bit(new_piece_bb, s);
    new_piece_bb  = set_bit(new_color_bb, s);

    m_pieces[s] = p;

    m_occ = set_bit(m_occ, s);

    if constexpr (DO_ZOB) {
        m_state.hash_key ^= zob_piece_square_key(prev_piece, s);
        m_state.hash_key ^= zob_piece_square_key(p, s);
    }
}

template <bool DO_ZOB>
inline void Board::remove_piece_internal(Square s) {
    Piece prev_piece = piece_at(s);

    Bitboard& prev_color_bb = color_bb_ref(prev_piece.color());
    Bitboard& prev_piece_bb = piece_bb_ref(prev_piece);

    prev_piece_bb = unset_bit(prev_piece_bb, s);
    prev_piece_bb = unset_bit(prev_color_bb, s);

    m_pieces[s] = PIECE_NULL;

    m_occ = unset_bit(m_occ, s);

    if constexpr (DO_ZOB) {
        m_state.hash_key ^= zob_piece_square_key(prev_piece, s);
    }
}

inline void Board::set_color_to_move(Color c) {
    m_state.hash_key ^= zob_color_to_move_key(m_ctm);
    m_ctm = c;
    m_state.hash_key ^= zob_color_to_move_key(c);
}

inline void Board::set_ep_square(Square s) {
    m_state.hash_key ^= zob_en_passant_square_key(m_state.ep_square);
    m_state.ep_square = s;
    m_state.hash_key ^= zob_en_passant_square_key(s);
}

inline void Board::set_castling_rights(CastlingRights castling_rights) {
    m_state.hash_key     ^= zob_castling_rights_key(m_state.castle_rights);
    m_state.castle_rights = castling_rights;
    m_state.hash_key     ^= zob_castling_rights_key(castling_rights);
}

inline void Board::set_castling_rights(Color color, Side side, bool allow) {
    CastlingRights curr_rights = castling_rights();
    ui8 bit = color * 2 + side;

    if (allow) {
        set_castling_rights(set_bit(curr_rights, bit));
    }
    else {
        set_castling_rights(unset_bit(curr_rights, bit));
    }
}

inline Square Board::king_square(Color color) const {
    return lsb(piece_bb(Piece(color, PT_KING)));
}

template <bool EXCLUDE_KING_ATKS>
inline Square Board::first_attacker_of(Color c, Square s) const {
    Bitboard occ = occupancy();

    Bitboard their_bishops   = piece_bb(Piece(c, PT_BISHOP));
    Bitboard bishop_atks     = bishop_attacks(s, occ);
    Bitboard bishop_attacker = bishop_atks & their_bishops;
    if (bishop_attacker) {
        return lsb(bishop_attacker);
    }

    Bitboard their_rooks   = piece_bb(Piece(c, PT_ROOK));
    Bitboard rook_atks     = rook_attacks(s, occ);
    Bitboard rook_attacker = rook_atks & their_rooks;
    if (rook_attacker) {
        return lsb(rook_attacker);
    }

    Bitboard their_queens   = piece_bb(Piece(c, PT_QUEEN));
    Bitboard queen_atks     = rook_atks | bishop_atks;
    Bitboard queen_attacker = queen_atks & their_queens;
    if (queen_attacker) {
        return lsb(queen_attacker);
    }

    Bitboard their_knights   = piece_bb(Piece(c, PT_KNIGHT));
    Bitboard knight_atks     = knight_attacks(s);
    Bitboard knight_attacker = knight_atks & their_knights;
    if (knight_attacker) {
        return lsb(knight_attacker);
    }

    Bitboard their_pawns   = piece_bb(Piece(c, PT_PAWN));
    Bitboard pawn_atks     = pawn_attacks(s, occ, opposite_color(c));
    Bitboard pawn_attacker = pawn_atks & their_pawns;
    if (pawn_attacker) {
        return lsb(pawn_attacker);
    }

    if constexpr (!EXCLUDE_KING_ATKS) {
        Bitboard their_king_bb  = piece_bb(Piece(c, PT_KING));
        Bitboard king_atks      = king_attacks(s);
        Bitboard king_attacker  = king_atks & their_king_bb;
        if (king_attacker) {
            return lsb(king_attacker);
        }
    }

    return SQ_NULL;
}

template <PieceType pt>
inline Bitboard Board::all_attackers_of_type(Color c, Square s) const {
    if constexpr (pt == PT_KNIGHT) {
        return piece_bb(Piece(c, PT_KNIGHT)) & knight_attacks(s);
    }
    if constexpr (pt == PT_KING) {
        return piece_bb(Piece(c, PT_KING)) & king_attacks(s);
    }
    Bitboard occ = occupancy();
    if constexpr (pt == PT_PAWN) {
        return piece_bb(Piece(c, PT_PAWN)) & pawn_attacks(s, occ, opposite_color(c));
    }
    if constexpr (pt == PT_BISHOP) {
        return piece_bb(Piece(c, PT_BISHOP)) & bishop_attacks(s, occ);
    }
    if constexpr (pt == PT_ROOK) {
        return piece_bb(Piece(c, PT_ROOK)) & rook_attacks(s, occ);
    }
    if constexpr (pt == PT_QUEEN) {
        return piece_bb(Piece(c, PT_QUEEN)) & queen_attacks(s, occ);
    }
}

template <bool EXCLUDE_KING_ATKS>
inline Bitboard Board::all_attackers_of(Color c, Square s) const {
    Bitboard ret = 0;

    ret |= all_attackers_of_type<PT_PAWN>(c, s);
    ret |= all_attackers_of_type<PT_KNIGHT>(c, s);
    ret |= all_attackers_of_type<PT_BISHOP>(c, s);
    ret |= all_attackers_of_type<PT_ROOK>(c, s);
    ret |= all_attackers_of_type<PT_QUEEN>(c, s);

    if constexpr (!EXCLUDE_KING_ATKS) {
        ret |= all_attackers_of_type<PT_KING>(c, s);
    }

    return ret;
}

inline Bitboard& Board::piece_bb_ref(Piece piece) {
    return m_bbs[piece.color()][piece.type()];
}

inline Bitboard& Board::color_bb_ref(Color color) {
    return m_bbs[PT_NULL][color];
}

} // illumina

#endif // ILLUMINA_BOARD_H
