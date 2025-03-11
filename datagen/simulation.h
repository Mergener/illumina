#ifndef ILLUMINA_SIMULATION_H
#define ILLUMINA_SIMULATION_H

#include <illumina.h>

#include <string>
#include <vector>

namespace illumina {

struct GamePlyData {
    Score white_pov_score;
    Move  best_move;
};

struct GameOptions {
    Board base_start_pos = Board::standard_startpos();
    ui64 search_node_limit = 16;
    int  min_random_plies = 8;
    int  max_random_plies = 16;
    int  win_adjudication_score = 1000;
    int  win_adjudication_plies = 6;
};

struct Game {
    BoardResult result;
    Board       start_pos;
    std::vector<GamePlyData> ply_data;
};

Game simulate(const GameOptions& options = {});

} // illumina

#endif // ILLUMINA_SIMULATION_H
