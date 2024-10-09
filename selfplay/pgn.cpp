#include "pgn.h"

#include <sstream>

namespace illumina {

Board PGNGame::start_pos() const {
    auto set_up = tags.find("SetUp");
    if (set_up == tags.end()) {
        return Board::standard_startpos();
    }

    if (set_up->second != "1") {
        return Board::standard_startpos();
    }

    auto fen = tags.find("FEN");
    if (fen == tags.end()) {
        return Board::standard_startpos();
    }

    return Board(fen->second);
}

void PGNGame::set_start_pos(const Board& board, bool shredder_fen) {
    set_start_pos(board.fen(shredder_fen));
}

void PGNGame::set_start_pos(std::string_view fen) {
    tags["SetUp"] = "1";
    tags["FEN"]   = fen;
}

std::ostream& operator<<(std::ostream& stream, const PGN& pgn) {
    for (const PGNGame& game: pgn.games) {
        stream << game << '\n';
    }
    return stream;
}

bool write_line(Board& board,
                std::ostream& stream,
                const std::vector<PGNNode>& nodes,
                int first_move_num) {
    if (nodes.empty()) {
        return true;
    }

    const PGNNode& first_node = nodes.front();
    Move first_move = first_node.move;

    int move_num = first_move_num;

    // Validate first move.
    if (!board.is_move_pseudo_legal(first_move) || !board.is_move_legal(first_move)) {
        return false;
    }

    // Write pre annotations.
    if (!first_node.pre_annotation.empty()) {
        stream << " {" << first_node.pre_annotation << "} ";
    }

    // If first move is made by a black piece, start with 'N... <move>'.
    // If first move is made by a white piece, start with 'N. <move>'
    if (board.color_to_move() == CL_WHITE) {
        stream << move_num << ". " << first_move.to_san(board) << ' ';
    }
    else {
        stream << move_num << "... " << first_move.to_san(board) << ' ';
    }

    // Write post annotations.
    if (!first_node.post_annotation.empty()) {
        stream << " {" << first_node.post_annotation << "} ";
    }

    if (!write_line(board, stream, first_node.variation, move_num)) {
        return false;
    }

    board.make_move(first_move);
    int n_moves_made = 1;

    // Write other moves.
    for (auto it = nodes.begin() + 1; it != nodes.end(); ++it) {
        // Validate first move.
        if (!board.is_move_pseudo_legal(it->move) || !board.is_move_legal(it->move)) {
            while (n_moves_made--) {
                board.undo_move();
            }
            return false;
        }

        // Write pre annotations.
        if (!it->pre_annotation.empty()) {
            stream << " {" << it->pre_annotation << "} ";
        }

        // Write move.
        if (board.color_to_move() == CL_WHITE) {
            move_num++;
            stream << move_num << ". " << it->move.to_san(board) << " ";
        }
        else {
            stream << it->move.to_san(board) << " ";
        }

        // Write post annotations.
        if (!it->post_annotation.empty()) {
            stream << " {" << it->post_annotation << "} ";
        }

        // Write variation.
        if (!write_line(board, stream, it->variation, move_num)) {
            while (n_moves_made--) {
                board.undo_move();
            }
            return false;
        }

        board.make_move(it->move);
        n_moves_made++;
    }

    // Undo everything.
    while (n_moves_made--) {
        board.undo_move();
    }

    return true;
}

std::ostream& operator<<(std::ostream& stream, const PGNGame& game) {
    // Write all tags.
    for (const auto& pair: game.tags) {
        stream << "[" << pair.first << " \"" << pair.second << "\"]" << '\n';
    }

    if (!game.tags.empty()) {
        stream << '\n';
    }

    if (game.main_line.empty()) {
        stream << "*" << std::endl;
        return stream;
    }

    Board board = game.start_pos();
    if (!write_line(board, stream, game.main_line, game.first_move_number)) {
        stream << "*\n";
        return stream;
    }

    if (!game.result.is_finished()) {
        stream << "*" << std::endl;
    }
    else if (game.result.is_draw()) {
        stream << "1/2-1/2" << std::endl;
    }
    else {
        stream << (game.result.winner == CL_WHITE ? "1-0" : "0-1") << std::endl;
    }

    return stream;
}

PGNNode& PGNGame::push_moves(Move move) {
    PGNNode& node = main_line.emplace_back();
    node.move = move;
    return node;
}

template<typename... TArgs>
PGNNode& PGNGame::push_moves(Move move, TArgs... other_moves) {
    push_moves(move);
    return push_moves(other_moves...);
}

} // illumina