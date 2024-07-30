#include "types.h"

#include <string>

#include "board.h"

namespace illumina {

std::string square_name(Square s) {
    ILLUMINA_ASSERT_VALID_SQUARE(s);

    BoardFile file = square_file(s);
    BoardRank rank = square_rank(s);

    return { file_to_char(file), rank_to_char(rank) };
}

int g_chebyshev[SQ_COUNT][SQ_COUNT];
int g_manhattan[SQ_COUNT][SQ_COUNT];
int g_center_manhattan[SQ_COUNT];

static void initialize_distances() {
    for (Square a = 0; a < SQ_COUNT; ++a) {
        for (Square b = 0; b < SQ_COUNT; ++b) {
            int file_dist = std::abs(square_file(a) - square_file(b));
            int rank_dist = std::abs(square_rank(a) - square_rank(b));

            g_manhattan[a][b] = file_dist + rank_dist;
            g_chebyshev[a][b] = std::max(file_dist, rank_dist);
        }
    }

    for (Square s = 0; s < SQ_COUNT; ++s) {
        int distance = g_manhattan[s][SQ_E4];
        distance = std::min(distance, g_manhattan[s][SQ_E5]);
        distance = std::min(distance, g_manhattan[s][SQ_D4]);
        distance = std::min(distance, g_manhattan[s][SQ_D5]);
        g_center_manhattan[s] = distance;
    }
}

Bitboard g_between[SQ_COUNT][SQ_COUNT];
Bitboard g_between_inclusive[SQ_COUNT][SQ_COUNT];

static void initialize_between() {
    g_between[SQ_COUNT - 1][SQ_COUNT - 1] = 0;
    for (Square a = 0; a < SQ_COUNT - 1; ++a) {
        g_between[a][a] = 0;
        for (Square b = a + 1; b < SQ_COUNT; ++b) {
            BoardFile file_a = square_file(a);
            BoardRank rank_a = square_rank(a);

            BoardFile file_b = square_file(b);
            BoardRank rank_b = square_rank(b);

            int x_delta = file_b - file_a;
            int y_delta = rank_b - rank_a;
            
            // Note that y_delta is guaranteed to be >=0
            // This premise allows us to not check for y_delta < 0 in
            // the conditionals below.

            Bitboard bb = 0;

            if (y_delta == 0) {
                // Horizontal
                for (Square s = a + DIR_EAST; s < b; s += DIR_EAST) {
                    bb = set_bit(bb, s);
                }
            }
            else if (x_delta == 0) {
                // Vertical
                for (Square s = a + DIR_NORTH; s < b; s += DIR_NORTH) {
                    bb = set_bit(bb, s);
                }
            }
            else if (std::abs(x_delta) == std::abs(y_delta)) {
                // Diagonal
                if (x_delta < 0) {
                    for (Square s = a + DIR_NORTHWEST; s < b; s += DIR_NORTHWEST) {
                        bb = set_bit(bb, s);
                    }
                }
                else { // deltaX > 0
                    for (Square s = a + DIR_NORTHEAST; s < b; s += DIR_NORTHEAST) {
                        bb = set_bit(bb, s);
                    }
                }
            }

            g_between[a][b] = bb;
            g_between[b][a] = bb;

            Bitboard bb_incl = bb | BIT(a) | BIT(b);
            g_between_inclusive[a][b] = bb_incl;
            g_between_inclusive[b][a] = bb_incl;
        }
    }
}

Bitboard g_adjacent[SQ_COUNT];

static void initialize_adjacent() {
    for (Square s = 0; s < SQ_COUNT; ++s) {
        Bitboard bb = 0;
        BoardFile f = square_file(s);

        if (f > FL_A) {
            bb = set_bit(bb, s + DIR_WEST);
        }
        if (f < FL_H) {
            bb = set_bit(bb, s + DIR_EAST);
        }

        g_adjacent[s] = bb;
    }
}

char Piece::to_char() const {
    return "--PpNnBbRrQqKk"[m_data & BITMASK(4)];
}

Piece Piece::from_char(char c) {
    switch (c) {
        case 'P': return WHITE_PAWN;
        case 'N': return WHITE_KNIGHT;
        case 'B': return WHITE_BISHOP;
        case 'R': return WHITE_ROOK;
        case 'Q': return WHITE_QUEEN;
        case 'K': return WHITE_KING;

        case 'p': return BLACK_PAWN;
        case 'n': return BLACK_KNIGHT;
        case 'b': return BLACK_BISHOP;
        case 'r': return BLACK_ROOK;
        case 'q': return BLACK_QUEEN;
        case 'k': return BLACK_KING;

        default: return  PIECE_NULL;
    }
}

std::string Move::to_uci(bool frc) const {
    switch (type()) {
        case MT_SIMPLE_PROMOTION:
        case MT_PROMOTION_CAPTURE:
            return square_name(source())
                 + square_name(destination())
                 + piece_type_to_char(promotion_piece_type());

        case MT_CASTLES:
            if (frc) {
                return square_name(source()) + square_name(castles_rook_src_square());
            }
            return square_name(source()) + square_name(destination());

        default:
            return square_name(source()) + square_name(destination());
    }
}

Move Move::parse_uci(const Board& board, std::string_view move_str) {
    if (move_str.size() < 4) {
        return MOVE_NULL;
    }

    Square src = parse_square(move_str);
    if (src == SQ_NULL) {
        return MOVE_NULL;
    }
    Square dst = parse_square(move_str.substr(2));
    if (dst == SQ_NULL) {
        return MOVE_NULL;
    }

    PieceType prom_piece_type = PT_NULL;
    if (move_str.size() > 4) {
        // Try to parse a promotion piece.
        Piece p = Piece::from_char(move_str[4]);
        if (p == PIECE_NULL) {
            return MOVE_NULL;
        }
        prom_piece_type = p.type();
    }

    return Move(board, src, dst, prom_piece_type);
}

Move::Move(const Board& board, Square src, Square dst, PieceType prom_piece_type) {
    Piece src_piece = board.piece_at(src);
    Piece dst_piece = board.piece_at(dst);

    if (src_piece.type() == PT_PAWN) {
        // For pawn moves, we'll assume every capture is a diagonal move and
        // every vertical move is a push.
        if (square_file(src) == square_file(dst)) {
            // Same file, vertical move. Assume it's a pawn push.
            int delta = std::abs(square_rank(dst) - square_rank(src));
            if (delta == 2) {
                m_data = new_double_push(src, src_piece.color()).raw();
            }
            else if (prom_piece_type != PT_NULL) {
                m_data = new_simple_promotion(src, dst, src_piece.color(), prom_piece_type).raw();
            }
            else {
                m_data = new_normal(src, dst, src_piece).raw();
            }
        }
        else {
            // Different file, diagonal move. Assume it's a capture.
            if (dst_piece == PIECE_NULL) {
                m_data = new_en_passant_capture(src, dst, src_piece.color()).raw();
            }
            else if (prom_piece_type == PT_NULL) {
                m_data = new_simple_capture(src, dst, src_piece, dst_piece).raw();
            }
            else {
                m_data = new_promotion_capture(src, dst, src_piece.color(), dst_piece, prom_piece_type).raw();
            }
        }
    }
    else if (src_piece.type() == PT_KING) {
        // For king moves, we must differentiate between regular moves
        // or castles. Also, we have two supported encodings for castling:
        //
        //  Encoding a: dst square is destination. Used for standard chess castling (e1g1, e1c1, e8c8, e8g8).
        //  Encoding b: dst square is castle rook source square. Used for FRC or standard.
        Color king_color = src_piece.color();
        int file_delta = square_file(dst) - square_file(src);

        if (file_delta > 1) {
            // file_delta > 1 means Encoding a.
            m_data = new_castles(src, king_color, SIDE_KING, board.castle_rook_square(king_color, SIDE_KING)).raw();
        }
        else if (file_delta < -1) {
            // file_delta < 1 also means Encoding a.
            m_data = new_castles(src, king_color, SIDE_QUEEN, board.castle_rook_square(king_color, SIDE_QUEEN)).raw();
        }
        else if (dst_piece.color() == src_piece.color() && dst_piece.type() == PT_ROOK) {
            // file_delta = 1.
            // In FRC, castling moves can occur with a single file distance.
            // However, in these scenarios, the dst_piece is set to a rook with the same color
            // as the moving king.
            Side side = dst == board.castle_rook_square(king_color, SIDE_KING)
                             ? SIDE_KING
                             : SIDE_QUEEN;

            m_data = new_castles(src, king_color, side, dst).raw();
        }
        else if (dst_piece != PIECE_NULL) {
            // King is capturing a piece.
            m_data = new_simple_capture(src, dst, src_piece, dst_piece).raw();
        }
        else {
            // King is simply moving.
            m_data = new_normal(src, dst, src_piece).raw();
        }
    }
    else if (dst_piece != PIECE_NULL) {
        // We have a piece on the target square, it's a capture.
        m_data = new_simple_capture(src, dst, src_piece, dst_piece).raw();
    }
    else {
        // No pieces on destination, normal move.
        m_data = new_normal(src, dst, src_piece).raw();
    }
}

void init_types() {
    initialize_distances();
    initialize_between();
    initialize_adjacent();
}


} // illumina