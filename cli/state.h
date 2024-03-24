#ifndef ILLUMINA_STATE_H
#define ILLUMINA_STATE_H

#include "illumina.h"

#include "ucioption.h"

namespace illumina {

class State {
public:
    // Board-related operations
    const Board& board() const;
    void display_board() const;
    void set_board(const Board& board);

    // Debug
    void perft(int depth) const;

    // Misc
    void uci();
    void set_option(std::string_view opt_name,
                    std::string_view value_str);
    void check_if_ready();
    void quit();

    State();

private:
    Board m_board = Board::standard_startpos();
    UCIOptionManager m_options;
};

void initialize_global_state();
State& global_state();

}

#endif // ILLUMINA_STATE_H
