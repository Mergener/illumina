#include "board.h"

#include <sstream>

#include "utils.h"
#include "parsehelper.h"

namespace illumina {

Board::Board(std::string_view fen_str, bool force_frc) {
    ParseHelper parse_helper(fen_str);

    // If force_frc = true, let's already set m_frc here.
    m_frc = force_frc;

    // Read and parse piece list.
    {
        std::string_view chunk = parse_helper.read_chunk();

        BoardFile file = FL_A;
        BoardRank rank = RNK_8;
        for (char c: chunk) {
            if (c == '/') {
                rank--;
                file = FL_A;
            }
            else if (std::isdigit(c)) {
                file += c - '0';
            }
            else {
                // Invalid piece characters will simply make empty squares.
                Square s = make_square(file, rank);
                Piece p  = Piece::from_char(c);
                if (p == PIECE_NULL) {
                    throw std::invalid_argument(std::string("Invalid piece '") + c + "'");
                }

                set_piece_at(s, p);
                file++;
            }
        }
    }

    // Read and parse color to move.
    {
        std::string_view chunk = parse_helper.read_chunk();
        if (chunk.empty()) {
            return;
        }

        char color_char = std::tolower(chunk[0]);
        if (color_char != 'w' && color_char != 'b') {
            throw std::invalid_argument(std::string("Invalid color '") + color_char + "'");
        }

        set_color_to_move(color_from_char(color_char));
    }

    // Read and parse castling rights.
    {
        std::string_view chunk = parse_helper.read_chunk();
        if (chunk.empty()) {
            return;
        }

        if (chunk != "-") {
            for (char c: chunk) {
                // We have two possible castling rights formats:
                //  Traditional - uses KQkq (uppercase for white, lowercase for black)
                //  ShredderFen - uses file identifiers, with same capitalization rules as above
                // We need to cater to both of them.
                Color king_color = std::isupper(c) ? CL_WHITE : CL_BLACK;
                Piece rook  = Piece(king_color, PT_ROOK);

                switch (c) {
                    case 'K':
                    case 'k':
                        set_castling_rights(king_color, SIDE_KING, true);
                        break;

                    case 'Q':
                    case 'q':
                        set_castling_rights(king_color, SIDE_QUEEN, true);
                        break;

                    default:
                        // Here comes the ShredderFen type of castling rights notation for
                        // fischer random chess.
                        BoardFile file = file_from_char(c);
                        if (file == FL_NULL) {
                            throw std::invalid_argument(std::string("Invalid castling rights token '") + c + "'");
                        }

                        BoardRank rank = king_color == CL_WHITE ? RNK_1 : RNK_8;
                        Square expected_rook_square = make_square(file, rank);
                        if (piece_at(expected_rook_square) != rook) {
                            throw std::invalid_argument(std::string("Expected ") + rook.to_char() + " on "
                                                        + square_name(expected_rook_square) + ", got " + piece_at(expected_rook_square).to_char()
                                                        + " instead.");
                        }

                        // We need to detect whether this rook is a king-side or queen-side rook.
                        Side side = file > square_file(king_square(king_color))
                                         ? SIDE_KING
                                         : SIDE_QUEEN;

                        set_castling_rights(king_color, side, true);

                        // If we're changing the original castle rook squares, we're on a
                        // Fischer Random position.
                        m_frc = m_frc || (m_castle_rook_squares[king_color][side] != expected_rook_square);
                        m_castle_rook_squares[king_color][side] = expected_rook_square;

                        break;
                }

                // We have figured out our castling rights. Now we need to find the corresponding castle rooks.
                Square king_sq          = king_square(king_color);
                BoardRank rank          = square_rank(king_sq);
                Bitboard eligible_rooks = rank_bb(rank) & piece_bb(rook);

                for (size_t i = 0; i < SIDE_COUNT; ++i) {
                    Side side = Side(i);
                    if (!has_castling_rights(king_color, side)) {
                        // No castling rights, we don't need to worry about the rook on
                        // this side.
                        continue;
                    }

                    Square& rook_square = m_castle_rook_squares[king_color][i];
                    if (piece_at(rook_square) == rook) {
                        // We're good, m_castle_rook_squares is already pointing to a valid
                        // rook in this position.
                        continue;
                    }

                    // We need to select valid rooks from the eligible rooks.
                    if (eligible_rooks == 0) {
                        set_castling_rights(king_color, side, false);
                        continue;
                    }

                    // We need to figure out a rook in the king's rank that could be eligible
                    // for castling.
                    BoardFile king_file = square_file(king_sq);
                    if (side == SIDE_QUEEN) {
                        rook_square = lsb(eligible_rooks);
                        eligible_rooks &= -1;

                        if (square_file(rook_square) > king_file) {
                            set_castling_rights(king_color, side, false);
                            continue;
                        }
                    }
                    else {
                        rook_square    = msb(eligible_rooks);
                        eligible_rooks = unset_bit(eligible_rooks, rook_square);

                        if (square_file(rook_square) < king_file) {
                            set_castling_rights(king_color, side, false);
                            continue;
                        }
                    }

                    // If we've reached this point, we know that we're definitely on a
                    // Fischer random position.
                    m_frc = true;
                }
            }
        }
    }

    // Read and parse en passant square.
    {
        std::string_view chunk = parse_helper.read_chunk();
        if (chunk.empty()) {
            return;
        }

        if (chunk != "-") {
            Square ep_square = parse_square(chunk);
            if (ep_square == SQ_NULL) {
                throw std::invalid_argument(std::string("Invalid en passant square '") + std::string(chunk) + "'");
            }
            set_ep_square(ep_square);
        }
    }

    // Read and parse rule 50 counter.
    {
        std::string_view chunk = parse_helper.read_chunk();
        if (chunk.empty()) {
            return;
        }

        int rule50;
        bool ok = try_parse_int(chunk, rule50);
        if (!ok) {
            throw std::invalid_argument(std::string("Invalid half-move clock '") + std::string(chunk) + "'");
        }
        m_state.rule50 = rule50;
    }

    // Read and parse move counter.
    {
        std::string_view chunk = parse_helper.read_chunk();
        if (chunk.empty()) {
            return;
        }

        int move_counter;
        bool ok = try_parse_int(chunk, move_counter);
        if (!ok) {
            throw std::invalid_argument(std::string("Invalid move counter number '") + std::string(chunk) + "'");
        }
        m_base_ply_count = move_counter - 1 + (color_to_move() == CL_BLACK);
    }
}

} // illumina