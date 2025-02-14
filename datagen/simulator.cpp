#include "simulation.h"

namespace illumina {

void Simulator::simulate(const SimulationOptions& opt) {
    m_validation_searcher.tt().clear();

    Board board = opt.startpos;

    // Play random plies.
    size_t random_plies = random(opt.min_random_plies, opt.max_random_plies + 1);
    for (size_t i = 0; i < random_plies; ++i) {
        Move legal_moves[MAX_GENERATED_MOVES];
        Move* begin = &legal_moves[0];
        Move* end   = generate_moves(board, legal_moves);
        size_t n_legal = end - begin;
        if (n_legal == 0) {
            if (i == 0) {
                // We got ourselves an invalid position.
                return;
            }

            // Position is already checkmate or stalemate. Rewind.
            i = -1;
            continue;
        }

        Move selected_move = legal_moves[random(size_t(0), n_legal)];
        board.make_move(selected_move);

        SearchResults val_results = m_validation_searcher.search(board,
                                                                 opt.validation_search_settings);
        if (std::abs(val_results.score) > opt.rnd_play_max_imbalance) {
            // Position is too imbalanced.
            i = -1;
            continue;
        }
    }

    // We generated a starting position. Start the simulation.
    m_white_searcher.tt().clear();
    m_black_searcher.tt().clear();
    m_white_searcher.set_pv_finish_listener([&](const PVResults& res) {
        if (opt.listener) {
            opt.listener->on_pv(res);
        }
    });
    m_black_searcher.set_pv_finish_listener([&](const PVResults& res) {
        if (opt.listener) {
            opt.listener->on_pv(res);
        }
    });

    if (opt.listener) {
        opt.listener->on_new_game(board);
    }

    BoardResult result;
    while (true) {
        result = board.result();
        if (result.is_finished()) {
            break;
        }

        SearchResults res;
        if (board.color_to_move() == CL_WHITE) {
            res = m_white_searcher.search(board, opt.player_search_settings);
        }
        else {
            res = m_black_searcher.search(board, opt.player_search_settings);
        }

        board.make_move(res.best_move);
        if (opt.listener) {
            opt.listener->on_move(res.best_move, res.score);
        }
    }

    if (opt.listener) {
        opt.listener->on_game_finished(result);
    }
}

} // illumina