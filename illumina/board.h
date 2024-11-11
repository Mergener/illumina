#ifndef ILLUMINA_BOARD_H
#define ILLUMINA_BOARD_H

#include <array>
#include <functional>
#include <optional>
#include <string_view>
#include <vector>

#include "attacks.h"
#include "types.h"
#include "zobrist.h"

namespace illumina {

/**
 * Listens to changes made to a board object.
 */
struct BoardListener {
    std::function<void(const Board& board, Piece p, Square s)> on_add_piece = nullptr;
    std::function<void(const Board& board, Piece p, Square s)> on_remove_piece = nullptr;
    std::function<void(const Board& board, Move move)> on_make_move = nullptr;
    std::function<void(const Board& board, Move move)> on_undo_move = nullptr;
    std::function<void(const Board& board)> on_make_null_move = nullptr;
    std::function<void(const Board& board)> on_undo_null_move = nullptr;
};

constexpr ui64 EMPTY_BOARD_HASH_KEY = 1;

enum class BoardOutcome {
    UNFINISHED,
    STALEMATE,
    CHECKMATE,
    DRAW_BY_REPETITION,
    DRAW_BY_50_MOVES_RULE,
    DRAW_BY_INSUFFICIENT_MATERIAL
};

struct BoardResult {
    std::optional<Color> winner  = std::nullopt;
    BoardOutcome         outcome = BoardOutcome::UNFINISHED;

    bool is_draw() const;
    bool is_finished() const;
};

class Board {
public:
    Color          color_to_move() const;
    Bitboard       occupancy() const;
    Bitboard       piece_bb(Piece piece) const;
    Bitboard       color_bb(Color color) const;
    Bitboard       piece_type_bb(PieceType pt) const;
    Piece          piece_at(Square s) const;
    Square         ep_square() const;
    bool           in_check() const;
    bool           in_double_check() const;
    ui64           hash_key() const;
    Square         castle_rook_square(Color color, Side side) const;
    void           set_castle_rook_square(Color color, Side side, Square square);
    int            rule50() const;
    bool           legal() const;
    int            ply_count() const;
    CastlingRights castling_rights() const;
    bool           has_castling_rights(Color color, Side side) const;
    Move           last_move() const;
    Bitboard       pinned_bb() const;
    bool           is_pinned(Square s) const;
    Square         pinner_square(Square pinned_sq) const;
    Square         king_square(Color color) const;
    bool           is_attacked_by(Color c, Square s) const;
    bool           is_attacked_by(Color c, Square s, Bitboard occ) const;
    bool           gives_check(Move move) const;
    std::string    fen(bool shredder_fen = false) const;
    std::string    pretty() const;
    bool           is_50_move_rule_draw() const;
    bool           is_repetition_draw(int max_appearances = 3) const;
    bool           is_insufficient_material_draw() const;
    bool           color_has_sufficient_material(Color color) const;
    BoardResult    result() const;
    bool           detect_frc() const;

    void set_piece_at(Square s, Piece p);
    void set_color_to_move(Color c);
    void set_ep_square(Square s);

    void set_castling_rights(CastlingRights castling_rights);
    void set_castling_rights(Color color, Side side, bool allow);
    void make_move(Move move);
    void undo_move();
    void make_null_move();
    void undo_null_move();
    bool is_move_pseudo_legal(Move move) const;
    bool is_move_legal(Move move) const;

    void set_listener(BoardListener listener);

    template <bool QUIET_PAWN_MOVES = false, bool EXCLUDE_KING_ATKS = false>
    Square first_attacker_of(Color c, Square s) const;

    template <bool QUIET_PAWN_MOVES = false, bool EXCLUDE_KING_ATKS = false>
    Square first_attacker_of(Color c, Square s, Bitboard occ) const;

    template <bool QUIET_PAWN_MOVES = false, bool EXCLUDE_KING_ATKS = false>
    Bitboard all_attackers_of(Color c, Square s) const;

    template <PieceType pt, bool QUIET_PAWN_MOVES = false>
    Bitboard all_attackers_of_type(Color c, Square s) const;

    Board() = default;
    Board(const Board& rhs);
    explicit Board(std::string_view fen_str);
    Board(Board&& rhs) noexcept;
    Board& operator=(const Board& rhs);
    ~Board() = default;

    static Board standard_startpos();
    static Board random_frc_startpos(bool mirrored = true);

private:
    // If any fields are added to this class, make sure to properly update
    // the copy, assignment and move constructors -- these were manually written
    // to prevent copying m_listeners to board copies.

    std::array<Piece, SQ_COUNT> m_pieces {};
    std::array<std::array<Bitboard, PT_COUNT>, CL_COUNT> m_bbs {};
    Color m_ctm = CL_WHITE;
    Bitboard m_occ = 0;

    std::array<Square, SQ_COUNT> m_pinners;
    Bitboard m_pinned_bb = 0;

    int m_base_ply_count = 0; // Gets added by m_prev_states.size()

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
        CastlingRights castle_rights = CR_NONE;
    };

    std::vector<State> m_prev_states;
    State m_state {};

    BoardListener m_listener {};

    Bitboard& piece_bb_ref(Piece piece);
    Bitboard& color_bb_ref(Color color);

    template <bool DO_ZOB, bool DO_PINS_AND_CHECKS>
    void set_piece_at_internal(Square s, Piece p);

    template <bool DO_ZOB, bool DO_PINS_AND_CHECKS>
    void piece_added(Square s, Piece p);

    template <bool DO_ZOB, bool DO_PINS_AND_CHECKS>
    void piece_removed(Square s);

    template <bool CHECK>
    bool is_move_legal(Move move) const;

    bool is_move_movement_valid(Move move) const;
    bool is_castles_pseudo_legal(Square king_square, Color c, Side castling_side) const;

    void compute_checkers();
    void compute_pins();
    void scan_pins(Bitboard attackers, Square king_square, Color pinned_color);
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

inline Bitboard Board::color_bb(Color color) const {
    return m_bbs[color][PT_NULL];
}

inline Bitboard Board::piece_type_bb(PieceType pt) const {
    return piece_bb(Piece(CL_WHITE, pt)) | piece_bb(Piece(CL_BLACK, pt));
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

inline void Board::set_castle_rook_square(Color color, Side side, Square square) {
    m_castle_rook_squares[color][side] = square;
}

inline bool Board::has_castling_rights(Color color, Side side) const {
    ui8 idx = color * 2 + ui8(side);
    return (castling_rights() & BIT(idx)) != 0;
}

inline CastlingRights Board::castling_rights() const {
    return m_state.castle_rights;
}

inline void Board::set_piece_at(Square s, Piece p) {
    set_piece_at_internal<true, true>(s, p);
}

inline Move Board::last_move() const {
    return m_state.last_move;
}

inline Bitboard Board::pinned_bb() const {
    return m_pinned_bb;
}

inline bool Board::is_pinned(Square s) const {
    return bit_is_set(pinned_bb(), s);
}

inline Square Board::pinner_square(Square pinned_sq) const {
    return m_pinners[pinned_sq];
}

inline Square Board::king_square(Color color) const {
    return lsb(piece_bb(Piece(color, PT_KING)));
}

inline bool Board::is_move_legal(Move move) const {
    return in_check() ? is_move_legal<true>(move) : is_move_legal<false>(move);
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

inline Bitboard& Board::piece_bb_ref(Piece piece) {
    return m_bbs[piece.color()][piece.type()];
}

inline Bitboard& Board::color_bb_ref(Color color) {
    return m_bbs[color][PT_NULL];
}

template <bool DO_ZOB, bool DO_PINS_AND_CHECKS>
inline void Board::set_piece_at_internal(Square s, Piece p) {
    ILLUMINA_ASSERT_VALID_SQUARE(s);

    Piece prev_piece = piece_at(s);
    if (p == prev_piece) {
        return;
    }

    if (p == PIECE_NULL) {
        // Note that we can assume there was a piece on 's' because
        // of the check 'p == prev_piece' above.
        piece_removed<DO_ZOB, false>(s);
        m_occ = unset_bit(m_occ, s);
    }
    else {
        if (prev_piece != PIECE_NULL) {
            // We removed a piece but placed a new one on the same square.
            // Don't change the occupancy.
            piece_removed<DO_ZOB, false>(s);
        }
        else {
            // We're adding a new piece, set bit on occupancy.
            m_occ = set_bit(m_occ, s);
        }
        piece_added<DO_ZOB, false>(s, p);
    }

    if constexpr (DO_PINS_AND_CHECKS) {
        compute_pins();
    }
}

template <bool DO_ZOB, bool DO_PINS_AND_CHECKS>
inline void Board::piece_added(Square s, Piece p) {
    if (m_listener.on_add_piece) {
        m_listener.on_add_piece(*this, p, s);
    }

    Color piece_color = p.color();

    Bitboard& new_color_bb  = color_bb_ref(piece_color);
    Bitboard& new_piece_bb  = piece_bb_ref(p);

    new_piece_bb = set_bit(new_piece_bb, s);
    new_color_bb = set_bit(new_color_bb, s);

    m_pieces[s] = p;

    if constexpr (DO_ZOB) {
        m_state.hash_key ^= zob_piece_square_key(p, s);
    }

    if constexpr (DO_PINS_AND_CHECKS) {
        compute_pins();
        compute_checkers();
    }
}

template <bool DO_ZOB, bool DO_PINS_AND_CHECKS>
inline void Board::piece_removed(Square s) {
    Piece prev_piece = piece_at(s);
    if (m_listener.on_remove_piece) {
        m_listener.on_remove_piece(*this, prev_piece, s);
    }

    Bitboard& prev_piece_bb = piece_bb_ref(prev_piece);
    Bitboard& prev_color_bb = color_bb_ref(prev_piece.color());

    prev_piece_bb = unset_bit(prev_piece_bb, s);
    prev_color_bb = unset_bit(prev_color_bb, s);

    m_pieces[s] = PIECE_NULL;

    if constexpr (DO_ZOB) {
        m_state.hash_key ^= zob_piece_square_key(prev_piece, s);
    }

    if constexpr (DO_PINS_AND_CHECKS) {
        compute_pins();
        compute_checkers();
    }
}

inline bool Board::is_50_move_rule_draw() const {
    return rule50() >= 100;
}

inline bool Board::is_insufficient_material_draw() const {
    return !color_has_sufficient_material(CL_WHITE) && !color_has_sufficient_material(CL_BLACK);
}

template <bool QUIET_PAWN_MOVES, bool EXCLUDE_KING_ATKS>
inline Square Board::first_attacker_of(Color c, Square s) const {
    return first_attacker_of(c, s, occupancy());
}

template <bool QUIET_PAWN_MOVES, bool EXCLUDE_KING_ATKS>
inline Square Board::first_attacker_of(Color c, Square s, Bitboard occ) const {
    Bitboard their_pawns   = piece_bb(Piece(c, PT_PAWN));
    Bitboard pawn_targets;
    if constexpr (QUIET_PAWN_MOVES) {
        pawn_targets = pawn_pushes(s, opposite_color(c), occ);
    }
    else {
        pawn_targets = pawn_attacks(s, opposite_color(c));
    }
    Bitboard pawn_attacker = pawn_targets & their_pawns;
    if (pawn_attacker) {
        return lsb(pawn_attacker);
    }

    Bitboard their_knights   = piece_bb(Piece(c, PT_KNIGHT));
    Bitboard knight_atks     = knight_attacks(s);
    Bitboard knight_attacker = knight_atks & their_knights;
    if (knight_attacker) {
        return lsb(knight_attacker);
    }

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

template <PieceType pt, bool QUIET_PAWN_MOVES>
inline Bitboard Board::all_attackers_of_type(Color c, Square s) const {
    if constexpr (pt == PT_KNIGHT) {
        return piece_bb(Piece(c, PT_KNIGHT)) & knight_attacks(s);
    }
    if constexpr (pt == PT_KING) {
        return piece_bb(Piece(c, PT_KING)) & king_attacks(s);
    }
    Bitboard occ = occupancy();
    if constexpr (pt == PT_PAWN) {
        Bitboard pawn_targets;
        Bitboard our_pawns = piece_bb(Piece(c, PT_PAWN));
        if constexpr (QUIET_PAWN_MOVES) {
            pawn_targets = reverse_pawn_pushes(s, c, occ & ~(our_pawns));
        }
        else {
            pawn_targets = pawn_attacks(s, opposite_color(c));
        }

        return our_pawns & pawn_targets;
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

template <bool QUIET_PAWN_MOVES, bool EXCLUDE_KING_ATKS>
inline Bitboard Board::all_attackers_of(Color c, Square s) const {
    Bitboard ret = 0;

    ret |= all_attackers_of_type<PT_PAWN, QUIET_PAWN_MOVES>(c, s);
    ret |= all_attackers_of_type<PT_KNIGHT>(c, s);
    ret |= all_attackers_of_type<PT_BISHOP>(c, s);
    ret |= all_attackers_of_type<PT_ROOK>(c, s);
    ret |= all_attackers_of_type<PT_QUEEN>(c, s);

    if constexpr (!EXCLUDE_KING_ATKS) {
        ret |= all_attackers_of_type<PT_KING>(c, s);
    }

    return ret;
}

template <bool CHECK>
inline bool Board::is_move_legal(Move move) const {
    Color us        = color_to_move();
    Square our_king = king_square(us);
    if (our_king == SQ_NULL) {
        // Any pseudo legal move is legal with no king
        // on the board.
        return true;
    }

    Color them      = opposite_color(us);
    Bitboard occ    = occupancy();
    Square src      = move.source();
    Square dest     = move.destination();
    Piece src_piece = move.source_piece();

    // Regardless of the position being a check or not, pinned
    // pieces can only move alongside their pins.
    if (is_pinned(src)) {
        Square pinner    = m_pinners[src];
        Bitboard between = between_bb(our_king, pinner);
        between          = set_bit(between, pinner);

        if (!bit_is_set(between, dest)) {
            // Trying to move away from pin, illegal.
            return false;
        }
    }

    // En-passant must be treated with extra care.
    if (move.type() == MT_EN_PASSANT) {
        Square capt_pawn_sq = dest + pawn_push_direction(them);
        Bitboard ep_occ     = occ;

        // Pretend there are no pawns
        ep_occ = unset_bit(ep_occ, capt_pawn_sq);
        ep_occ = unset_bit(ep_occ, src);

        Bitboard king_rank_bb = rank_bb(square_rank(our_king));

        Bitboard their_rooks    = piece_bb(Piece(them, PT_ROOK));
        Bitboard their_queens   = piece_bb(Piece(them, PT_QUEEN));
        Bitboard their_hor_atks = rook_attacks(our_king, ep_occ) & (their_rooks | their_queens) & king_rank_bb;

        if (rook_attacks(our_king, ep_occ) & their_hor_atks) {
            return false;
        }

        if (CHECK) {
            Bitboard their_bishops        = piece_bb(Piece(them, PT_BISHOP));
            Bitboard their_diag_attackers = their_bishops | their_queens;
            if (bishop_attacks(our_king, occ) & their_diag_attackers) {
                return false;
            }
        }
    }
    else if (src_piece.type() == PT_KING) {
        // King cannot be moving to a square attacked by any piece
        if (is_attacked_by(them, dest)) {
            return false;
        }

        // The conditional above does not cover cases where the king runs
        // in the same direction a slider is attacking them.
        Bitboard occ_without_king = occ & (~BIT(our_king));

        Bitboard their_bishops = piece_bb(Piece(them, PT_BISHOP));
        Bitboard their_rooks   = piece_bb(Piece(them, PT_ROOK));
        Bitboard their_queens  = piece_bb(Piece(them, PT_QUEEN));

        Bitboard their_diag_atks = bishop_attacks(dest, occ_without_king) & (their_bishops | their_queens);
        if (their_diag_atks != 0) {
            return false;
        }

        Bitboard their_line_atks = rook_attacks(dest, occ_without_king) & (their_rooks | their_queens);
        if (their_line_atks != 0) {
            return false;
        }
    }
    else if constexpr (CHECK) {
        if (m_state.n_checkers > 1) {
            // Only king moves allowed in double checks
            return false;
        }

        // We're in a single check and trying to move a piece that is not the king.
        // The piece we're trying to move can only move to a square between the king
        // and the checker...
        Square atk_square = first_attacker_of(them, our_king);
        Bitboard between  = between_bb(our_king, atk_square);
        between = set_bit(between, atk_square); // ... or capture the checker!

        if (!bit_is_set(between, dest)) {
            return false;
        }
    }
    return true;
}

inline bool BoardResult::is_finished() const {
    return outcome != BoardOutcome::UNFINISHED;
}

inline bool BoardResult::is_draw() const {
    return    outcome != BoardOutcome::UNFINISHED
           && outcome != BoardOutcome::CHECKMATE;
}

} // illumina

#endif // ILLUMINA_BOARD_H
