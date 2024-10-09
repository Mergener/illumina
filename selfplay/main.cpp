#include <iostream>

#include "pgn.h"

int main() {
    using namespace illumina;

    PGN pgn;

    PGNGame& game = pgn.games.emplace_back();

    game.push_moves()


    std::cout << pgn;

    return 0;
}