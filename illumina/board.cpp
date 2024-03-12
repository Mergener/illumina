#include "board.h"

#include "zobrist.h"

namespace illumina {

void Board::set_piece_at(Square s, Piece p) {
    ILLUMINA_ASSERT_VALID_SQUARE(s);

    Piece prev_piece        = piece_at(s);
    Bitboard& prev_piece_bb = piece_bb(prev_piece);
    Bitboard& new_piece_bb  = piece_bb(p);

    prev_piece_bb = pop_bit(prev_piece_bb, s);
    new_piece_bb  = set_bit(new_piece_bb, s);

    if (p != PIECE_NULL) {
        m_occ = set_bit(m_occ, s);
    }
    else {
        m_occ = pop_bit(m_occ, s);
    }

    m_state.hash_key ^= zob_piece_square_key(prev_piece, s);
    m_state.hash_key ^= zob_piece_square_key(p, s);
}

} // illumina