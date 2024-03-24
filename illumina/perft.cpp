#include "perft.h"

#include "movegen.h"
#include "clock.h"

namespace illumina {

template <bool ROOT = false, bool LOG = false>
static ui64 perft(Board& board, int depth) {
    Move moves[MAX_GENERATED_MOVES];

    Move* end = generate_moves(board, moves);

    if (depth <= 1) {
        if constexpr (ROOT && LOG) {
            for (Move* it = moves; it != end; ++it) {
                Move move = *it;
                std::cout << move.to_uci() << ": " << 1 << std::endl;
            }
        }

        return end - moves;
    }

    ui64 n = 0;
    for (Move* it = moves; it != end; ++it) {
        Move move = *it;

        board.make_move(move);
        ui64 it_leaves = perft<false, false>(board, depth - 1);
        board.undo_move();

        if constexpr (ROOT && LOG) {
            std::cout << move.to_uci() << ": " << it_leaves << std::endl;
        }

        n += it_leaves;
    }

    return n;
}

ui64 perft(const Board& board, int depth, PerftArgs args) {
    Board replica = board;
    if (args.log) {
        TimePoint before = now();
        ui64 res = perft<true, true>(replica, depth);
        TimePoint after = now();
        ui64 time_delta = delta_ms(after, before);

        std::cout << "\nResult: " << res << std::endl;
        std::cout << "Time: " << time_delta << "ms" << std::endl;
        std::cout << "NPS: " << ui64(res / double(time_delta / 1000.0)) << std::endl;

        return res;
    }
    else {
        return perft<true, false>(replica, depth);
    }
}

}
