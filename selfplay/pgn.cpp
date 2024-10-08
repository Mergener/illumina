#include "pgn.h"

#include <sstream>

namespace illumina {

std::ostream& operator<<(std::ostream& stream, const PGN& pgn) {
    for (const PGNGame& game: pgn.games) {
        stream << game << '\n';
    }
    return stream;
}

std::ostream& operator<<(std::ostream& stream, const PGNGame& game) {
    // Write all tags.
    for (const auto& pair: game.tags) {
        stream << "[" << pair.first << " \"" << pair.second << "\"]" << '\n';
    }
    stream << '\n';

    if (game.main_line.empty()) {
        stream << "*\n";
        return stream;
    }

    // Write all moves.
    int move_num = game.first_move_number;
    Board board = game.start_pos;

    // First node is written a little differently.

    return stream;
}

std::ostream& operator<<(std::ostream& stream, const PGNNode& node) {
}


} // illumina