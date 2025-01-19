#ifndef ILLUMINA_SIMULATOR_H
#define ILLUMINA_SIMULATOR_H

#include <illumina.h>

namespace illumina {

class SimulationListener {
    virtual void on_new_game(const Board& board) = 0;
    virtual void on_pv(const PVResults& results) = 0;
    virtual void on_move(Move move, Score score) = 0;
    virtual void on_game_finished(BoardResult r) = 0;
};

struct SimulationOptions {
    SimulationListener* listener;
    int min_random_plies = 0;
    int max_random_plies = 0;
    int rnd_play_max_imbalance = 0;
};

class Simulator {
public:
    Simulator();

private:

};

}

#endif // ILLUMINA_SIMULATOR_H
