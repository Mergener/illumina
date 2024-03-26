#include "commands.h"

#include <iostream>

#include "illumina.h"
#include "state.h"

namespace illumina {

void register_commands(UCIServer& server) {
    server.register_command("uci", [](const UCICommandContext& ctx) {
        global_state().uci();
    });

    server.register_command("setoption", [](const UCICommandContext& ctx) {
        std::string opt_name  = ctx.word_after("name");
        std::string value_str = ctx.word_after("value");

        global_state().set_option(opt_name, value_str);
    });

    server.register_command("option", [](const UCICommandContext& ctx) {
        std::string opt_name  = ctx.word_after("");

        global_state().display_option_value(opt_name);
    });

    server.register_command("ucinewgame", [](const UCICommandContext& ctx) {
        global_state().new_game();
    });

    server.register_command("position", [](const UCICommandContext& ctx) {
        bool read_only = true;
        Board board;
        if (ctx.has_arg("startpos")) {
            // Get a starting position board.
            board = Board::standard_startpos();
            read_only = false;
        }
        else if (ctx.has_arg("fen")) {
            board = Board(ctx.all_after("fen"));
            read_only = false;
        }

        if (read_only) {
            // User only wants to see the current state of the board.
            global_state().display_board();
            return;
        }

        // User might want to start with some moves played.
        // Validate each move, simulate them, and then send
        // the resulting board to the state object.
        if (ctx.has_arg("moves")) {
            // Parse every move contained in the command string.
            std::vector<Move> moves;
            std::string move_list_str = ctx.all_after("moves");

            ParseHelper parser(move_list_str);
            while (!parser.finished()) {
                std::string_view chunk = parser.read_chunk();
                Move move = Move::parse_uci(board, chunk);
                if (move != MOVE_NULL) {
                    board.make_move(move);
                }
            }
        }

        global_state().set_board(board);
    });

    server.register_command("domoves", [](const UCICommandContext& ctx) {
        Board board = global_state().board();
        std::vector<Move> moves;
        std::string move_list_str = ctx.all_after("");

        ParseHelper parser(move_list_str);
        while (!parser.finished()) {
            Move move = Move::parse_uci(board, parser.read_chunk());
            if (move != MOVE_NULL) {
                board.make_move(move);
            }
        }
        global_state().set_board(board);
    });

    server.register_command("perft", [](const UCICommandContext& ctx) {
        global_state().perft(int(ctx.int_after("")));
    });

    server.register_command("isready", [](const UCICommandContext& ctx) {
        global_state().check_if_ready();
    });

    server.register_command("evaluate", [](const UCICommandContext& ctx) {
        global_state().evaluate();
    });

    server.register_command("go", [](const UCICommandContext& ctx) {
        SearchSettings settings;
        settings.max_depth = ctx.int_after("depth", MAX_DEPTH);

        if (ctx.has_arg("wtime")) {
            settings.white_time = ctx.int_after("wtime");
        }

        if (ctx.has_arg("winc")) {
            settings.white_inc = ctx.int_after("winc");
        }

        if (ctx.has_arg("btime")) {
            settings.black_time = ctx.int_after("btime");
        }

        if (ctx.has_arg("binc")) {
            settings.black_inc = ctx.int_after("binc");
        }

        if (ctx.has_arg("movetime")) {
            settings.move_time = ctx.int_after("movetime");
        }

        global_state().search(settings);
    });

    server.register_command("stop", [](const UCICommandContext& ctx) {
        global_state().stop_search();
    });

    server.register_command("quit", [](const UCICommandContext& ctx) {
        global_state().quit();
    });

}

} // illumina