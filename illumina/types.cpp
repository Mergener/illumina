#include "types.h"

#include <string>

namespace illumina {

Square parse_square(std::string_view square_str) {
    ILLUMINA_ASSERT(square_str.size() >= 2);

    BoardFile file = std::tolower(square_str[0]) - 'a';
    BoardRank rank = square_str[1] - '0';

    return make_square(file, rank);
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

Move Move::parse_uci(const Board& board, std::string_view move_str) {
    // TODO
    Square src = parse_square(move_str);
    Square dst = parse_square(move_str.substr(2));
    return Move(board, src, dst);
}

void init_types() {
    initialize_distances();
}


} // illumina