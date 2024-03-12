#ifndef ILLUMINA_BOARD_H
#define ILLUMINA_BOARD_H

#include <array>
#include <string_view>
#include <vector>

#include "types.h"

namespace illumina {

struct BoardState {
    Move last_move   = MOVE_NULL;
    Square ep_square = SQ_NULL;
    ui64 hash_key    = 1;
    ui16 rule_50     = 0;
    ui8 n_checkers   = 0;
    std::array<std::array<Bitboard, PT_COUNT>, CL_COUNT> attacks;
    CastlingRights castle_rights;
};

class Board {
public:
    inline Color    color_to_move() const       { return m_ctm; }
    inline Bitboard occupancy() const           { return m_occ; }
    inline Bitboard piece_bb(Piece piece) const { return m_bbs[piece.color()][piece.type()]; }
    inline Piece    piece_at(Square s) const    { return m_pieces[s]; }
    inline bool     frc() const                 { return m_frc; }
    inline bool     in_check() const            { return m_state.n_checkers > 0; }
    inline bool     in_double_check() const     { return m_state.n_checkers == 2; }

    inline Square get_castle_rook_square(Color color, BoardSide side) const {
        return m_castle_rook_squares[color][int(side)];
    }

    void set_piece_at(Square s, Piece p); // TODO

    bool legal() const; // TODO

    void make_move(Move move); // TODO
    void undo_move(); // TODO
    void make_null_move(); // TODO
    void undo_null_move(); // TODO
    bool is_move_pseudo_legal(Move move) const; // TODO
    bool is_move_legal(Move move) const; // TODO

    inline Square king_square(Color color) const { return pop_lsb(piece_bb(Piece(color, PT_KING))); }

    inline Board(bool frc = false)
        : m_frc(frc) {}
    Board(std::string_view fen_str, bool force_frc); // TODO
    ~Board() = default;
    Board(Board&& rhs) = default;
    Board& operator=(const Board& rhs) = default;

private:
    std::array<Piece, SQ_COUNT> m_pieces {};
    std::array<std::array<Bitboard, PT_COUNT>, CL_COUNT> m_bbs {};
    Color m_ctm = CL_WHITE;
    Bitboard m_occ = 0;
    Bitboard m_pinned = 0;

    std::array<std::array<Square, BOARD_SIDE_COUNT>, CL_COUNT> m_castle_rook_squares = {
        std::array<Square, BOARD_SIDE_COUNT> { SQ_H1, SQ_A1 },
        std::array<Square, BOARD_SIDE_COUNT> { SQ_H8, SQ_A8 }
    };

    std::vector<BoardState> m_prev_states;
    BoardState m_state;

    bool m_frc = false;
};

} // illumina

#endif // ILLUMINA_BOARD_H
