#include "types.h"

#include <string>

#include "board.h"

namespace illumina {

Square parse_square(std::string_view square_str) {
    ILLUMINA_ASSERT(square_str.size() >= 2);

    BoardFile file = std::tolower(square_str[0]) - 'a';
    BoardRank rank = square_str[1] - '1';

    return make_square(file, rank);
}

std::string square_name(Square s) {
    ILLUMINA_ASSERT_VALID_SQUARE(s);

    BoardFile file = square_file(s);
    BoardRank rank = square_rank(s);

    return { file_to_char(file), rank_to_char(rank) };
}

int g_chebyshev[SQ_COUNT][SQ_COUNT];
int g_manhattan[SQ_COUNT][SQ_COUNT];

static void initialize_distances() {
    for (Square a = 0; a < SQ_COUNT; ++a) {
        for (Square b = 0; b < SQ_COUNT; ++b) {
            int file_dist = std::abs(square_file(a) - square_file(b));
            int rank_dist = std::abs(square_rank(a) - square_rank(b));

            g_manhattan[a][b] = file_dist + rank_dist;
            g_chebyshev[a][b] = std::max(file_dist, rank_dist);
        }
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
                return square_name(source()) + square_name(castles_rook_square());
            }
            return square_name(source()) + square_name(destination());

        default:
            return square_name(source()) + square_name(destination());
    }
}

Move Move::parse_uci(const Board& board, std::string_view move_str) {
    ILLUMINA_ASSERT(move_str.size() >= 4);

    Square src = parse_square(move_str);
    Square dst = parse_square(move_str.substr(2));
    PieceType prom_piece_type = PT_NULL;

    if (move_str.size() > 4) {
        // Try to parse a promotion piece
        Piece p = Piece::from_char(move_str[4]);
        prom_piece_type = p.type();
    }

    return Move(board, src, dst, prom_piece_type);
}

Move::Move(const Board& board, Square src, Square dst, PieceType prom_piece_type) {
    Piece src_piece = board.piece_at(src);
    Piece dst_piece = board.piece_at(dst);

    if (src_piece.type() == PT_PAWN) {
        // Pawn moves
        if (square_file(src) == square_file(dst)) {
            // Pawn push
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
        // Check if castles
        Color king_color = src_piece.color();
        int file_delta = square_file(src) - square_file(dst);
        // If a king tries to move more than a single file away, we can assume
        // it's trying to castle.
        if (file_delta > 1) {
            m_data = new_castles(src, king_color, BoardSide::King, dst).raw();
        }
        else if (file_delta < 1) {
            m_data = new_castles(src, king_color, BoardSide::Queen, dst).raw();
        }
        else if (dst_piece.color() == src_piece.color() && dst_piece.type() == PT_ROOK) {
            // In chess960, castling moves can occur with a single file distance.
            // However, in these scenarios, the dst_piece is set to a rook with the same color
            // as the moving king.
            BoardSide side = dst == board.get_castle_rook_square(king_color, BoardSide::King)
                ? BoardSide::King
                : BoardSide::Queen;

            m_data = new_castles(src, king_color, side, dst).raw();
        }
        else if (dst_piece != PIECE_NULL) {
            m_data = new_simple_capture(src, dst, src_piece, dst_piece).raw();
        }
    }
    else if (dst_piece != PIECE_NULL) {
        m_data = new_simple_capture(src, dst, src_piece, dst_piece).raw();
    }
    else {
        m_data = new_normal(src, dst, src_piece).raw();
    }
}

void init_types() {
    initialize_distances();
}


} // illumina