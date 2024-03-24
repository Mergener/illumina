#include "commands.h"

#include <iostream>

#include "illumina.h"
#include "state.h"

namespace illumina {

static void cmd_uci(UCICommandContext& ctx) {
    global_state().uci();
}

static void cmd_setoption(UCICommandContext& ctx) {
    std::string opt_name  = ctx.word_after("name");
    std::string value_str = ctx.word_after("value");

    global_state().set_option(opt_name, value_str);
}

static void cmd_position(UCICommandContext& ctx) {
    if (ctx.has_arg("startpos")) {
        // Get a starting position board.
        Board board = Board::standard_startpos();

        // User might want to start with some moves played.
        // Validate each move, simulate them, and then send
        // the resulting board to the state object.
        if (ctx.has_arg("moves")) {
            // Parse every move contained in the command string.
            std::vector<Move> moves;
            std::string move_list_str = ctx.all_after("moves");

            ParseHelper parser(move_list_str);
            while (!parser.finished()) {
                Move move = Move::parse_uci(board, parser.read_chunk());
                board.make_move(move);
            }
        }

        global_state().set_board(board);
    }
    else if (ctx.has_arg("fen")) {
        global_state().set_board(Board(ctx.all_after("fen")));
    }
    else {
        // Assume the user just wants to output the current position.
        global_state().display_board();
    }
}

static void cmd_perft(UCICommandContext& ctx) {
    global_state().perft(int(ctx.int_after("")));
}

static void cmd_isready(UCICommandContext& ctx) {
    global_state().check_if_ready();
}

static void cmd_quit(UCICommandContext& ctx) {
    global_state().quit();
}

void register_commands(UCIServer& server) {
    server.register_command("uci", cmd_uci);
    server.register_command("setoption", cmd_setoption);
    server.register_command("position", cmd_position);
    server.register_command("isready", cmd_isready);
    server.register_command("perft", cmd_perft);
    server.register_command("quit", cmd_quit);
}

} // illumina