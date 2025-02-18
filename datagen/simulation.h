#ifndef ILLUMINA_SIMULATOR_H
#define ILLUMINA_SIMULATOR_H

#include <illumina.h>

namespace illumina {

class SimulationListener {
public:
    virtual void on_new_game(const Board& board) = 0;
    virtual void on_pv(const PVResults& results) = 0;
    virtual void on_move(Move move, Score score) = 0;
    virtual void on_game_finished(BoardResult r) = 0;

    virtual ~SimulationListener() = default;
};

struct SimulationOptions {
    SimulationListener* listener = nullptr;
    SearchSettings player_search_settings {};
    SearchSettings validation_search_settings {};
    Board startpos = Board::standard_startpos();
    int min_random_plies = 0;
    int max_random_plies = 0;
    int rnd_play_max_imbalance = 0;
};

class Simulator {
public:
    Simulator();

    void simulate(const SimulationOptions& opt);

private:
    Searcher m_white_searcher;
    Searcher m_black_searcher;
    Searcher m_validation_searcher;
};

}

#endif // ILLUMINA_SIMULATOR_H
