#include "perft.h"

#include "movegen.h"
#include "movepicker.h"
#include "clock.h"

namespace illumina {

std::vector<std::string> s_logs;

static void log_init() {
    s_logs.clear();
}

static void log_move(Move move, ui64 nodes) {
    s_logs.push_back(move.to_uci() + ": " + std::to_string(nodes));
}

static void flush_logs(bool sort) {
    if (sort) {
        std::sort(s_logs.begin(), s_logs.end());
    }
    for (const auto& l: s_logs) {
        std::cout << l << std::endl;
    }
}

template <bool ROOT = false, bool LOG = false>
static ui64 perft(Board& board, int depth) {
    Move moves[MAX_GENERATED_MOVES];

    Move* end = generate_moves(board, moves);

    if (depth <= 1) {
        if constexpr (ROOT && LOG) {
            for (Move* it = moves; it != end; ++it) {
                Move move = *it;
                log_move(move, 1);
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
            log_move(move, it_leaves);
        }

        n += it_leaves;
    }

    return n;
}

ui64 perft(const Board& board, int depth, PerftArgs args) {
    Board replica = board;
    if (args.log) {
        log_init();

        TimePoint before = now();
        ui64 res         = perft<true, true>(replica, depth);
        TimePoint after  = now();
        ui64 time_delta  = delta_ms(after, before);

        flush_logs(args.sort_output);
        std::cout << "\nResult: " << res << std::endl;
        std::cout << "Time: "     << time_delta << "ms" << std::endl;
        std::cout << "NPS: "      << ui64(res / double(time_delta / 1000.0)) << std::endl;

        return res;
    }
    else {
        return perft<true, false>(replica, depth);
    }
}

template <bool ROOT = false, bool LOG = false>
static ui64 move_picker_perft_internal(MoveHistory& mv_hist, Board& board, int depth) {
    if (depth <= 0) {
        return 1;
    }

    Move move {};
    MovePicker<false> move_picker(board, 0, mv_hist);
    ui64 n = 0;
    while ((move = move_picker.next()) != MOVE_NULL) {
        board.make_move(move);
        ui64 it_leaves = move_picker_perft_internal<false, false>(mv_hist, board, depth - 1);
        board.undo_move();

        if constexpr (ROOT && LOG) {
            log_move(move, it_leaves);
        }

        n += it_leaves;
    }

    return n;

}

ui64 move_picker_perft(const Board& board, int depth, PerftArgs args) {
    Board replica = board;
    MoveHistory mv_hist;
    if (args.log) {
        log_init();
        TimePoint before = now();
        ui64 res = move_picker_perft_internal<true, true>(mv_hist, replica, depth);
        TimePoint after = now();
        ui64 time_delta = delta_ms(after, before);
        flush_logs(args.sort_output);

        std::cout << "\nResult: " << res << std::endl;
        std::cout << "Time: "     << time_delta << "ms" << std::endl;
        std::cout << "NPS: "      << ui64(res / double(time_delta / 1000.0)) << std::endl;
        return res;
    }
    else {
        return move_picker_perft_internal<true, false>(mv_hist, replica, depth);
    }
}

}
