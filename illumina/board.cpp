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
            compute_pins();
            compute_checkers();
            return;
        }

        char color_char = std::tolower(chunk[0]);
        if (color_char != 'w' && color_char != 'b') {
            throw std::invalid_argument(std::string("Invalid color '") + color_char + "'");
        }

        set_color_to_move(color_from_char(color_char));
    }

    // We know the color to move and the piece placements.
    // We now need to compute all pins and checkers.
    compute_pins();
    compute_checkers();

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

std::string Board::fen() const {
    std::stringstream stream;

    // Write piece placements.
    for (BoardRank r: RANKS_REVERSE) {
        int emptyCount = 0;

        for (BoardFile f: FILES) {
            Square s = make_square(f, r);
            Piece p  = piece_at(s);

            if (p != PIECE_NULL) {
                if (emptyCount > 0) {
                    stream << emptyCount;
                    emptyCount = 0;
                }

                stream << p.to_char();
            }
            else {
                emptyCount++;
            }
        }

        if (emptyCount > 0) {
            stream << emptyCount;
            emptyCount = 0;
        }

        if (r > RNK_1) {
            stream << '/';
        }
    }
    stream << ' ';

    // Write color to move.
    stream << (color_to_move() == CL_WHITE ? 'w' : 'b') << ' ';

    // Write castling rights.
    if (castling_rights() == CR_NONE) {
        stream << '-';
    }
    else {
        if (has_castling_rights(CL_WHITE, SIDE_KING)) {
            stream << 'K';
        }
        if (has_castling_rights(CL_WHITE, SIDE_QUEEN)) {
            stream << 'Q';
        }
        if (has_castling_rights(CL_BLACK, SIDE_KING)) {
            stream << 'k';
        }
        if (has_castling_rights(CL_BLACK, SIDE_QUEEN)) {
            stream << 'q';
        }
    }
    stream << ' ';

    // Write en-passant square.
    Square epSquare = ep_square();
    if (epSquare != SQ_NULL) {
        stream << square_name(ep_square());
    }
    else {
        stream << '-';
    }
    stream << ' ';

    // Write half-move clock.
    stream << rule50() << ' ';

    // Write full move number.
    stream << ply_count() / 2 + 1;

    return stream.str();
}

std::string Board::pretty() const {
    std::stringstream stream;
    stream << "    A B C D E F G H" << std::endl;

    for (BoardRank r: RANKS_REVERSE) {
        stream << static_cast<int>(r) + 1 << " [";

        for (BoardFile f = FL_A; f <= FL_H; f++) {
            Square s = make_square(f, r);

            stream << " " << piece_at(s).to_char();
        }

        stream << " ]\n";
    }
    return stream.str();
}

void Board::make_move(Move move) {
    Color moving_color  = color_to_move();
    Color opponent      = opposite_color(color_to_move());
    Square source       = move.source();
    Square destination  = move.destination();
    Piece source_piece  = move.source_piece();
    PieceType source_pt = source_piece.type();

    m_prev_states.push_back(m_state);
    m_state.rule50++;
    m_state.last_move = move;

    set_piece_at_internal<true, false>(source, PIECE_NULL);

    // We need to update castling rights accordingly whenever a king
    // or a rook moves.
    if (source_pt == PT_KING) {
        set_castling_rights(castling_rights() & ~(BITMASK(2) << (moving_color * 2)));
    }
    else if (source_pt == PT_ROOK) {
        if (source == castle_rook_square(moving_color, SIDE_KING)) {
            set_castling_rights(moving_color, SIDE_KING, false);
        }
        else if (source == castle_rook_square(moving_color, SIDE_QUEEN)) {
            set_castling_rights(moving_color, SIDE_QUEEN, false);
        }
    }
    else if (source_pt == PT_PAWN) {
        m_state.rule50 = 0;
    }

    if (move.is_capture()) {
        m_state.rule50 = 0;
        if (move.captured_piece().type() == PT_ROOK) {
            if (destination == castle_rook_square(opponent, SIDE_KING)) {
                set_castling_rights(opponent, SIDE_KING, false);
            }
            else if (destination == castle_rook_square(opponent, SIDE_QUEEN)) {
                set_castling_rights(opponent, SIDE_QUEEN, false);
            }
        }
    }

    // Special moves must perform some extra actions.
    switch (move.type()) {
        case MT_PROMOTION_CAPTURE:
        case MT_SIMPLE_PROMOTION:
            set_piece_at_internal<true, false>(destination, Piece(moving_color, move.promotion_piece_type()));
            set_ep_square(SQ_NULL);
            break;

        case MT_DOUBLE_PUSH:
            set_ep_square(destination - pawn_push_direction(moving_color));
            set_piece_at_internal<true, false>(destination, source_piece);
            break;

        case MT_EN_PASSANT:
            set_ep_square(SQ_NULL);
            set_piece_at_internal<true, false>(destination, source_piece);
            set_piece_at_internal<true, false>(destination - pawn_push_direction(moving_color), PIECE_NULL);
            break;

        case MT_CASTLES: {
            set_ep_square(SQ_NULL);
            set_piece_at_internal<true, false>(destination, source_piece);

            Square prev_rook_square = move.castles_rook_src_square();
            Side castling_side      = move.castles_side();

            // Move the rook.
            set_piece_at_internal<true, false>(prev_rook_square, PIECE_NULL);
            set_piece_at_internal<true, false>(castled_rook_square(moving_color, castling_side),
                                               Piece(moving_color, PT_ROOK));
            break;
        }

        default:
            set_ep_square(SQ_NULL);
            set_piece_at_internal<true, false>(destination, source_piece);
            break;
    }

    set_color_to_move(opposite_color(moving_color));

    compute_pins();
    compute_checkers();
}

void Board::undo_move() {
    set_color_to_move(opposite_color(color_to_move()));
    Color moving_color = color_to_move();

    Move move = last_move();

    Square source       = move.source();
    Square destination  = move.destination();
    Piece source_piece  = move.source_piece();

    set_piece_at_internal<true, false>(source, source_piece);

    switch (move.type()) {
        case MT_PROMOTION_CAPTURE:
        case MT_SIMPLE_CAPTURE:
            set_piece_at_internal<true, false>(destination, move.captured_piece());
            break;

        case MT_EN_PASSANT:
            set_piece_at_internal<true, false>(destination - pawn_push_direction(moving_color),
                                               Piece(opposite_color(moving_color), PT_PAWN));
            set_piece_at_internal<true, false>(destination, PIECE_NULL);
            break;

        case MT_CASTLES: {
            set_piece_at_internal<true, false>(destination, PIECE_NULL);

            Square prev_rook_square = move.castles_rook_src_square();
            Side castling_side      = move.castles_side();

            // Move the rook.
            set_piece_at_internal<true, false>(prev_rook_square, Piece(moving_color, PT_ROOK));
            set_piece_at_internal<true, false>(castled_rook_square(moving_color, castling_side),
                                               PIECE_NULL);
            break;
        }

        default:
            set_piece_at_internal<true, false>(destination, PIECE_NULL);
            break;
    }

    m_state = m_prev_states.back();
    m_prev_states.pop_back();

    compute_pins();
}

bool Board::is_attacked_by(Color c, Square s) const {
    return is_attacked_by(c, s, occupancy());
}

bool Board::is_attacked_by(Color c, Square s, Bitboard occ) const {
    Bitboard their_bishops   = piece_bb(Piece(c, PT_BISHOP));
    Bitboard bishop_atks     = bishop_attacks(s, occ);
    Bitboard bishop_attacker = bishop_atks & their_bishops;
    if (bishop_attacker) {
        return true;
    }

    Bitboard their_rooks   = piece_bb(Piece(c, PT_ROOK));
    Bitboard rook_atks     = rook_attacks(s, occ);
    Bitboard rook_attacker = rook_atks & their_rooks;
    if (rook_attacker) {
        return true;
    }

    Bitboard their_queens   = piece_bb(Piece(c, PT_QUEEN));
    Bitboard queen_atks     = rook_atks | bishop_atks;
    Bitboard queen_attacker = queen_atks & their_queens;
    if (queen_attacker) {
        return true;
    }

    Bitboard their_knights   = piece_bb(Piece(c, PT_KNIGHT));
    Bitboard knight_atks     = knight_attacks(s);
    Bitboard knight_attacker = knight_atks & their_knights;
    if (knight_attacker) {
        return true;
    }

    Bitboard their_pawns   = piece_bb(Piece(c, PT_PAWN));
    Bitboard pawn_atks     = pawn_attacks(s, opposite_color(c));
    Bitboard pawn_attacker = pawn_atks & their_pawns;
    if (pawn_attacker) {
        return true;
    }

    Bitboard their_king_bb  = piece_bb(Piece(c, PT_KING));
    Bitboard king_atks      = king_attacks(s);
    Bitboard king_attacker  = king_atks & their_king_bb;
    if (king_attacker) {
        return true;
    }

    return false;
}

void Board::make_null_move() {
    m_prev_states.push_back(m_state);

    set_ep_square(SQ_NULL);
    set_color_to_move(opposite_color(color_to_move()));
}

void Board::undo_null_move() {
    m_state = m_prev_states.back();
    m_prev_states.pop_back();

    set_color_to_move(opposite_color(color_to_move()));
}

void Board::compute_pins() {
    m_pinned_bb = 0;

    for (Color c: COLORS) {
        Color them      = opposite_color(c);
        Square our_king = king_square(c);

        Bitboard their_bishops = piece_bb(Piece(them, PT_BISHOP));
        Bitboard their_rooks   = piece_bb(Piece(them, PT_ROOK));
        Bitboard their_queens  = piece_bb(Piece(them, PT_QUEEN));

        Bitboard their_diagonal_atks = (their_bishops | their_queens) & bishop_attacks(our_king, 0);
        Bitboard their_line_atks     = (their_rooks   | their_queens) & rook_attacks(our_king, 0);

        scan_pins(their_diagonal_atks, our_king, c);
        scan_pins(their_line_atks, our_king, c);
    }
}

void Board::scan_pins(Bitboard attackers, Square king_square, Color pinned_color) {
    Bitboard occ = occupancy();

    while (attackers) {
        Square s  = lsb(attackers);
        attackers = unset_lsb(attackers);

        Bitboard between = between_bb(s, king_square) & occ;

        if (between == 0 || unset_lsb(between) != 0) {
            // More than one piece (or none) in between, none being pinned just yet.
            continue;
        }

        // A piece might be pinned
        Square pinned_sq = lsb(between);
        Piece piece      = piece_at(pinned_sq);
        if (piece.color() == pinned_color) {
            // Piece is being pinned
            m_pinned_bb          = set_bit(m_pinned_bb, pinned_sq);
            m_pinners[pinned_sq] = s;
        }
    }
}

bool Board::legal() const {
    return !is_attacked_by(color_to_move(), king_square(opposite_color(color_to_move())));
}

void Board::compute_checkers() {
    Color us           = color_to_move();
    Color them         = opposite_color(us);
    Square king_sq     = king_square(us);
    Bitboard checkers  = all_attackers_of<false, true>(them, king_sq);
    m_state.n_checkers = popcount(checkers);
}

} // illumina