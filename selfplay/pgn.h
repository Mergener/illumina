#ifndef ILLUMINA_PGN_H
#define ILLUMINA_PGN_H

#include <string>
#include <map>
#include <memory>
#include <string_view>

#include "board.h"

namespace illumina {

struct PGNNode {
    Move move = MOVE_NULL;
    std::vector<PGNNode> variation {};
    std::string pre_annotation {};
    std::string post_annotation {};
};

struct PGNGame {
    std::vector<PGNNode> main_line;
    std::map<std::string, std::string> tags;
    int first_move_number = 1;
    BoardResult result;

    Board start_pos() const;

    void set_start_pos(std::string_view fen);

    void set_start_pos(const Board& board,
                       bool shredder_fen = false);

    PGNNode& push_moves(Move move);

    template <typename... TArgs>
    PGNNode& push_moves(Move move, TArgs... other_moves);

    PGNNode& push_moves(Board& board, Move move);

    template <typename... TArgs>
    PGNNode& push_moves(Board& board, Move move, TArgs... other_moves);
};

struct PGN {
    std::vector<PGNGame> games;
};

std::ostream& operator<<(std::ostream& stream, const PGN& pgn);
std::ostream& operator<<(std::ostream& stream, const PGNGame& game);

} // illumina

#endif // ILLUMINA_PGN_H
