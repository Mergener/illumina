#ifndef ILLUMINA_PERFT_H
#define ILLUMINA_PERFT_H

#include "board.h"

namespace illumina {

struct PerftArgs {
    bool log = false;
    bool sort_output = false;
};

ui64 perft(const Board& board, int depth, PerftArgs args = {});
ui64 move_picker_perft(const Board& board, int depth, PerftArgs args = {});

}

#endif // ILLUMINA_PERFT_H
