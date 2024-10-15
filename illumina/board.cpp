#include "board.h"

#include <sstream>

#include "movegen.h"
#include "utils.h"
#include "parsehelper.h"

namespace illumina {

Board::Board(std::string_view fen_str) {
    ParseHelper parse_helper(fen_str);

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

                set_piece_at_internal<true, false>(s, p);
                file++;
            }
        }
    }
    compute_pins();
    compute_checkers();

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

std::string Board::fen(bool shredder_fen) const {
    std::stringstream stream;

    // Write piece placements.
    for (BoardRank r: RANKS_REVERSE) {
        int n_empty = 0;

        for (BoardFile f: FILES) {
            Square s = make_square(f, r);
            Piece p  = piece_at(s);

            if (p != PIECE_NULL) {
                if (n_empty > 0) {
                    stream << n_empty;
                    n_empty = 0;
                }

                stream << p.to_char();
            }
            else {
                n_empty++;
            }
        }

        if (n_empty > 0) {
            stream << n_empty;
            n_empty = 0;
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
            stream << (!shredder_fen ? 'K' : char(std::toupper(file_to_char(square_file(castle_rook_square(CL_WHITE, SIDE_KING))))));
        }
        if (has_castling_rights(CL_WHITE, SIDE_QUEEN)) {
            stream << (!shredder_fen ? 'Q' : char(std::toupper(file_to_char(square_file(castle_rook_square(CL_WHITE, SIDE_QUEEN))))));
        }
        if (has_castling_rights(CL_BLACK, SIDE_KING)) {
            stream << (!shredder_fen ? 'k' : file_to_char(square_file(castle_rook_square(CL_BLACK, SIDE_KING))));
        }
        if (has_castling_rights(CL_BLACK, SIDE_QUEEN)) {
            stream << (!shredder_fen ? 'q' : file_to_char(square_file(castle_rook_square(CL_BLACK, SIDE_QUEEN))));
        }
    }
    stream << ' ';

    // Write en-passant square.
    Square ep_sq = ep_square();
    if (ep_sq != SQ_NULL) {
        stream << square_name(ep_sq);
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
    if (m_listener.on_make_move) {
        m_listener.on_make_move(*this, move);
    }

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

        // If we're capturing one of the opponent's rooks, we take
        // away their castling rights on the respective rook's side.
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
            if (piece_at(prev_rook_square).type() != PT_KING) {
                set_piece_at_internal<true, false>(prev_rook_square, PIECE_NULL);
            }
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
    Move move = previous_move();
    if (m_listener.on_undo_move) {
        m_listener.on_undo_move(*this, move);
    }

    set_color_to_move(opposite_color(color_to_move()));
    Color moving_color = color_to_move();

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
            // FRC castles can sometimes just leave the king on the same
            // square.
            if (destination != source) {
                set_piece_at_internal<true, false>(destination, PIECE_NULL);
            }

            Square prev_rook_square = move.castles_rook_src_square();
            Side castling_side      = move.castles_side();

            // Move the rook.
            Square castled_rook_sq = castled_rook_square(moving_color, castling_side);
            if (piece_at(castled_rook_sq).type() != PT_KING) {
                set_piece_at_internal<true, false>(castled_rook_sq,
                                                   PIECE_NULL);
            }
            set_piece_at_internal<true, false>(prev_rook_square, Piece(moving_color, PT_ROOK));
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
    if (m_listener.on_make_null_move) {
        m_listener.on_make_null_move(*this);
    }

    m_prev_states.push_back(m_state);

    set_color_to_move(opposite_color(color_to_move()));
    set_ep_square(SQ_NULL);

    compute_checkers();
    compute_pins();
}

void Board::undo_null_move() {
    if (m_listener.on_undo_null_move) {
        m_listener.on_undo_null_move(*this);
    }

    set_color_to_move(opposite_color(color_to_move()));

    m_state = m_prev_states.back();
    m_prev_states.pop_back();

    compute_pins();
}

void Board::compute_pins() {
    m_pinned_bb = 0;

    for (Color c: COLORS) {
        Color them      = opposite_color(c);
        if (piece_bb(Piece(c, PT_KING)) == 0) {
            continue;
        }
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


BoardResult Board::result() const {
    BoardResult result;

    if (is_50_move_rule_draw()) {
        result.outcome = BoardOutcome::DRAW_BY_50_MOVES_RULE;
    }
    else if (is_repetition_draw()) {
        result.outcome = BoardOutcome::DRAW_BY_REPETITION;
    }
    else if (is_insufficient_material_draw()) {
        result.outcome = BoardOutcome::DRAW_BY_INSUFFICIENT_MATERIAL;
    }
    else {
        // Check for stalemate or checkmate.
        Move moves[MAX_GENERATED_MOVES];
        Move* begin = moves;
        Move* end   = generate_moves(*this, moves);

        if (begin == end) {
            // No moves generated, we either have stalemate or checkmate.
            if (in_check()) {
                result.outcome = BoardOutcome::CHECKMATE;
                result.winner  = opposite_color(color_to_move());
            }
            else {
                result.outcome = BoardOutcome::STALEMATE;
            }
        }
    }

    return result;
}

bool Board::legal() const {
    if (popcount(piece_bb(WHITE_KING)) != 1 || popcount(piece_bb(BLACK_KING)) != 1) {
        return false;
    }

    return !is_attacked_by(color_to_move(), king_square(opposite_color(color_to_move())));
}

bool Board::is_castles_pseudo_legal(Square king_square, Color c, Side castling_side) const {
    if (!has_castling_rights(c, castling_side)) {
        // No castling rights
        return false;
    }

    if (in_check()) {
        return false;
    }

    Square rook_square = castle_rook_square(c, castling_side);
    if (piece_at(rook_square) != Piece(c, PT_ROOK)) {
        return false;
    }

    Bitboard occ = occupancy();
    Bitboard inner_castle_path = between_bb(king_square, rook_square);
    if (inner_castle_path & occ) {
        // There cannot be any pieces between the king and the rook.
        return false;
    }

    Bitboard king_path = unset_bit(between_bb_inclusive(king_square, castled_king_square(c, castling_side)), king_square);
    while (king_path) {
        Square s = lsb(king_path);
        if (is_attacked_by(opposite_color(c), s)) {
            return false;
        }
        king_path = unset_lsb(king_path);
    }

    return true;
}

bool Board::is_move_movement_valid(Move move) const {
    Bitboard occ    = occupancy();
    Square src      = move.source();
    Square dest     = move.destination();
    Piece src_piece = move.source_piece();

    Bitboard piece_movements;
    if (src_piece.type() != PT_PAWN) {
        piece_movements = piece_attacks(src_piece, src, occ);
    }
    else {
        piece_movements = (pawn_attacks(src, src_piece.color()) & (occ | ep_square()))
                        | pawn_pushes(move.source(), src_piece.color(), occ);
    }

    return bit_is_set(piece_movements, dest);
}

bool Board::is_move_pseudo_legal(Move move) const {
    Square src               = move.source();
    Square dest              = move.destination();
    Piece src_piece          = move.source_piece();
    Piece dst_piece          = move.captured_piece();
    Color src_piece_color    = src_piece.color();
    Color dst_piece_color    = dst_piece.color();
    PieceType src_piece_type = src_piece.type();

    if (src == dest && move.type() != MT_CASTLES) {
        // Destination can't be equal to source, except on some
        // FRC castling moves.
        return false;
    }

    if (src_piece != piece_at(src)) {
        // Source piece must match the piece at the source square.
        return false;
    }
    if (src_piece_color != color_to_move()) {
        // Source piece must have the color of the current player to move.
        return false;
    }
    if (dst_piece != piece_at(dest)) {
        return false;
    }

    // Check if move is a capture that isn't en passant.
    if (move.is_capture() && move.type() != MT_EN_PASSANT) {
        if (dst_piece == PIECE_NULL) {
            // Non en passant captures must have a dst_piece.
            return false;
        }
        if (dst_piece_color == src_piece_color) {
            // We can only capture opposing pieces.
            return false;
        }
    }
    else {
        if (dst_piece != PIECE_NULL) {
            // Non-captures (or en passants) cannot have 'dest' pieces.
            return false;
        }
    }

    // Do specific logic for special moves.
    switch (move.type()) {
        case MT_CASTLES:
            if (src_piece_type != PT_KING) {
                // Castling can only be performed by a king.
                return false;
            }
            return is_castles_pseudo_legal(src, src_piece_color, move.castles_side());

        case MT_PROMOTION_CAPTURE:
        case MT_SIMPLE_PROMOTION:
            if (src_piece_type != PT_PAWN) {
                // Promotions can only be performed by a pawn.
                return false;
            }
            if (square_rank(dest) != promotion_rank(src_piece_color)) {
                // Destination rank must be the pawn's promotion rank.
                // Note that after this condition, 'isNormalMovePseudoLegal' is already going to cover
                // the requirement for 'src' to be located on the 7th rank.
                return false;
            }

            return is_move_movement_valid(move);

        case MT_EN_PASSANT:
            if (src_piece_type != PT_PAWN) {
                // En passant can only be performed by a pawn.
                return false;
            }
            if (dest != ep_square()) {
                // Destination square must be the en passant square.
                return false;
            }
            if (piece_at(dest - pawn_push_direction(src_piece_color)) != Piece(opposite_color(src_piece_color), PT_PAWN)) {
                // There must be an enemy pawn to be captured in en passant.
                return false;
            }
            return is_move_movement_valid(move);

        case MT_DOUBLE_PUSH:
            if (src_piece_type != PT_PAWN) {
                // Double pushes can only be performed by a pawn.
                return false;
            }
            if (dst_piece != PIECE_NULL) {
                // Pawn pushes cannot capture pieces
                return false;
            }

            if (std::abs(square_rank(src) - square_rank(dest)) != 2) {
                // Pawn didn't move two squares.
                return false;
            }

            return is_move_movement_valid(move);

        case MT_NORMAL:
            if (src_piece_type == PT_PAWN && dst_piece != PIECE_NULL) {
                return false;
            }
            return is_move_movement_valid(move);

        case MT_SIMPLE_CAPTURE:
            return is_move_movement_valid(move);

        default:
            return false;
    }
}

void Board::compute_checkers() {
    Color us   = color_to_move();
    Color them = opposite_color(us);
    if (piece_bb(Piece(them, PT_KING)) == 0) {
        // No king, no checkers.
        m_state.n_checkers = 0;
        return;
    }

    Square king_sq     = king_square(us);
    Bitboard checkers  = all_attackers_of<false, true>(them, king_sq);
    m_state.n_checkers = popcount(checkers);
}

void Board::set_listener(BoardListener listener) {
    m_listener = std::move(listener);
}

Board Board::standard_startpos() {
    return Board("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

template <Color C>
static void distribute_frc_pieces(Board& board) {
    // Determine color-specific constants.
    constexpr BoardRank BACK_RANK = C == CL_WHITE ? RNK_1 : RNK_8;
    constexpr BoardRank PAWN_RANK = C == CL_WHITE ? RNK_2 : RNK_7;

    // Initialize our back-rank squares with all bits set to 1.
    Bitboard remaining_squares = rank_bb(BACK_RANK);

    // Determine king position.
    BoardFile king_file = random(FL_B, FL_G + 1);
    Square king_square  = make_square(king_file, BACK_RANK);
    remaining_squares   = unset_bit(remaining_squares, king_square);

    // Determine rook positions.
    BoardFile q_rook_file = random(FL_A, king_file);
    Square q_rook_square  = make_square(q_rook_file, BACK_RANK);
    remaining_squares     = unset_bit(remaining_squares, q_rook_square);

    BoardFile k_rook_file = random(king_file + 1, FL_H + 1);
    Square k_rook_square  = make_square(k_rook_file, BACK_RANK);
    remaining_squares     = unset_bit(remaining_squares, k_rook_square);

    // Determine bishop positions.
    Square bishop_a_square = random_square(remaining_squares);
    remaining_squares = unset_bit(remaining_squares, bishop_a_square);
    Square bishop_b_square = random_square(remaining_squares & ~color_complex_of(bishop_a_square));
    remaining_squares = unset_bit(remaining_squares, bishop_b_square);

    // Determine knight positions.
    Square knight_a_square = random_square(remaining_squares);
    remaining_squares = unset_bit(remaining_squares, knight_a_square);
    Square knight_b_square = random_square(remaining_squares);
    remaining_squares = unset_bit(remaining_squares, knight_b_square);

    Square queen_square = lsb(remaining_squares);

    // Place pieces.
    board.set_piece_at(king_square, Piece(C, PT_KING));
    board.set_piece_at(k_rook_square, Piece(C, PT_ROOK));
    board.set_piece_at(q_rook_square, Piece(C, PT_ROOK));
    board.set_piece_at(bishop_a_square, Piece(C, PT_BISHOP));
    board.set_piece_at(bishop_b_square, Piece(C, PT_BISHOP));
    board.set_piece_at(knight_a_square, Piece(C, PT_KNIGHT));
    board.set_piece_at(knight_b_square, Piece(C, PT_KNIGHT));
    board.set_piece_at(queen_square, Piece(C, PT_QUEEN));

    // Place pawns.
    for (BoardFile f: FILES) {
        board.set_piece_at(make_square(f, PAWN_RANK), Piece(C, PT_PAWN));
    }

    // Set castle rooks and castling rights.
    board.set_castle_rook_square(C, SIDE_KING, k_rook_square);
    board.set_castle_rook_square(C, SIDE_QUEEN, q_rook_square);
    board.set_castling_rights(C, SIDE_KING, true);
    board.set_castling_rights(C, SIDE_QUEEN, true);
}

template <Color C>
static void mirror_frc_pieces(Board& board) {
    Color THEM = opposite_color(C);

    Bitboard their_pieces = board.color_bb(THEM);
    while (their_pieces) {
        Square s = lsb(their_pieces);
        Square new_square = mirror_vertical(s);
        board.set_piece_at(new_square, Piece(C, board.piece_at(s).type()));
        their_pieces = unset_lsb(their_pieces);
    }

    board.set_castle_rook_square(C, SIDE_KING, mirror_vertical(board.castle_rook_square(THEM, SIDE_KING)));
    board.set_castle_rook_square(C, SIDE_QUEEN, mirror_vertical(board.castle_rook_square(THEM, SIDE_QUEEN)));
    board.set_castling_rights(C, SIDE_KING, board.has_castling_rights(THEM, SIDE_KING));
    board.set_castling_rights(C, SIDE_QUEEN, board.has_castling_rights(THEM, SIDE_KING));
}

Board Board::random_frc_startpos(bool mirrored) {
    Board board;

    distribute_frc_pieces<CL_WHITE>(board);

    if (mirrored) {
        mirror_frc_pieces<CL_BLACK>(board);
    }
    else {
        distribute_frc_pieces<CL_BLACK>(board);
    }

    return board;
}

// Board constructors below.
// We don't use default copy/assignment constructor implementations since
// we don't want listeners to be copied from a board object to another.

Board::Board(const illumina::Board &rhs)
        : m_pieces(rhs.m_pieces),
          m_bbs(rhs.m_bbs),
          m_ctm(rhs.m_ctm),
          m_occ(rhs.m_occ),
          m_pinners(rhs.m_pinners),
          m_pinned_bb(rhs.m_pinned_bb),
          m_base_ply_count(rhs.m_base_ply_count),
          m_castle_rook_squares(rhs.m_castle_rook_squares),
          m_prev_states(rhs.m_prev_states),
          m_state(rhs.m_state),
          m_listener({}) { }

Board& Board::operator=(const illumina::Board &rhs) {
    if (this != &rhs) {
        m_pieces              = rhs.m_pieces;
        m_bbs                 = rhs.m_bbs;
        m_ctm                 = rhs.m_ctm;
        m_occ                 = rhs.m_occ;
        m_pinners             = rhs.m_pinners;
        m_pinned_bb           = rhs.m_pinned_bb;
        m_base_ply_count      = rhs.m_base_ply_count;
        m_castle_rook_squares = rhs.m_castle_rook_squares;
        m_state               = rhs.m_state;
        m_prev_states         = rhs.m_prev_states;

        m_listener = {};
    }
    return *this;
}

Board::Board(Board&& rhs) noexcept
    : m_pieces(rhs.m_pieces),
      m_bbs(rhs.m_bbs),
      m_ctm(rhs.m_ctm),
      m_occ(rhs.m_occ),
      m_pinners(rhs.m_pinners),
      m_pinned_bb(rhs.m_pinned_bb),
      m_base_ply_count(rhs.m_base_ply_count),
      m_castle_rook_squares(rhs.m_castle_rook_squares),
      m_prev_states(std::move(rhs.m_prev_states)),
      m_state(rhs.m_state),
      m_listener(std::move(rhs.m_listener)) { }

} // illumina