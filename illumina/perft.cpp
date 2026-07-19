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

template <bool BULK, bool ROOT = false, bool LOG = false>
static ui64 perft(Board& board, int depth) {
    Move moves[MAX_GENERATED_MOVES];

    auto* end = generate_moves(board, moves);

    if constexpr (BULK) {
        if (depth <= 1) {
            if constexpr (ROOT && LOG) {
                for (Move* it = moves; it != end; ++it) {
                    auto move = *it;
                    log_move(move, 1);
                }
            }

            return end - moves;
        }
    }
    else {
        if (depth <= 0) {
            return 1;
        }
    }

    auto n = ui64(0);
    for (Move* it = moves; it != end; ++it) {
        auto move = *it;

        board.make_move(move);
        auto it_leaves = perft<BULK, false, false>(board, depth - 1);
        board.undo_move();

        if constexpr (ROOT && LOG) {
            log_move(move, it_leaves);
        }

        n += it_leaves;
    }

    return n;
}

ui64 perft(const Board& board, int depth, PerftArgs args) {
    auto replica = board;
    if (args.log) {
        log_init();

        auto before = now();
        ui64 res;
        if (args.bulk) {
            res = perft<true, true, true>(replica, depth);
        }
        else {
            res = perft<false, true, true>(replica, depth);
        }
        auto after  = now();
        auto time_delta  = delta_ms(after, before);

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

    auto move = Move{};
    auto move_picker = MovePicker<false>(board, 0, mv_hist);
    auto n = ui64(0);
    while ((move = move_picker.next()) != MOVE_NULL) {
        board.make_move(move);
        auto it_leaves = move_picker_perft_internal<false, false>(mv_hist, board, depth - 1);
        board.undo_move();

        if constexpr (ROOT && LOG) {
            log_move(move, it_leaves);
        }

        n += it_leaves;
    }

    return n;

}

ui64 move_picker_perft(const Board& board, int depth, PerftArgs args) {
    auto replica = board;
    MoveHistory mv_hist;
    if (args.log) {
        log_init();
        auto before = now();
        auto res = move_picker_perft_internal<true, true>(mv_hist, replica, depth);
        auto after = now();
        auto time_delta = delta_ms(after, before);
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
