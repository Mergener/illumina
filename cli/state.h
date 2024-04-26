#ifndef ILLUMINA_STATE_H
#define ILLUMINA_STATE_H

#include <thread>

#include "illumina.h"

#include "ucioption.h"

namespace illumina {

class State {
public:
    // Board-related operations
    void new_game();
    void display_board() const;
    const Board& board() const;
    void set_board(const Board& board);

    // Debug
    void perft(int depth) const;

    // Evaluation
    void evaluate() const;

    // Search
    void search(SearchSettings settings);
    void stop_search();

    // Misc
    void uci();
    void display_option_value(std::string_view opt_name);
    void set_option(std::string_view opt_name,
                    std::string_view value_str);
    void check_if_ready();
    void quit();

    State();

private:
    Board m_board = Board::standard_startpos();
    UCIOptionManager m_options;
    Searcher         m_searcher;
    std::thread*     m_search_thread = nullptr;
    bool             m_searching = false;

    void setup_searcher();
    void register_options();
};

void initialize_global_state();
State& global_state();

}

#endif // ILLUMINA_STATE_H