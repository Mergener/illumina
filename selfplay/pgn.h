#ifndef ILLUMINA_PGN_H
#define ILLUMINA_PGN_H

#include <string>
#include <map>
#include <memory>
#include <string_view>

#include "board.h"

namespace illumina {

struct PGNNode {
    Move move;
    std::vector<PGNNode> variation;
    std::string annotation;
};

struct PGNGame {
    Board start_pos = Board::standard_startpos();
    std::vector<PGNNode> main_line;
    std::map<std::string, std::string> tags;
    int first_move_number = 1;
    BoardResult result;
};

struct PGN {
    std::vector<PGNGame> games;
};

std::ostream& operator<<(std::ostream& stream, const PGN& pgn);
std::ostream& operator<<(std::ostream& stream, const PGNGame& game);

} // illumina

#endif // ILLUMINA_PGN_H
