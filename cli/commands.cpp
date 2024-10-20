#include "commands.h"

#include <iostream>

#include "illumina.h"
#include "state.h"

namespace illumina {

void register_commands(CLIApplication& app) {
    app.register_command("uci", [](const CommandContext& ctx) {
        global_state().uci();
    });

    app.register_command("setoption", [](const CommandContext& ctx) {
        std::string opt_name  = ctx.word_after("name");
        std::string value_str = ctx.word_after("value");

        global_state().set_option(opt_name, value_str);
    });

    app.register_command("option", [](const CommandContext& ctx) {
        std::string opt_name  = ctx.word_after("");

        global_state().display_option_value(opt_name);
    });

    app.register_command("ucinewgame", [](const CommandContext& ctx) {
        global_state().new_game();
    });

    app.register_command("position", [](const CommandContext& ctx) {
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
        else if (ctx.has_arg("frc")) {
            board = Board::random_frc_startpos();
            std::cout << "info string Selected FRC startpos " << board.fen(true) << std::endl;
            read_only = false;
        }
        else if (ctx.has_arg("dfrc")) {
            board = Board::random_frc_startpos(false);
            std::cout << "info string Selected DFRC startpos " << board.fen(true) << std::endl;
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

    app.register_command("domoves", [](const CommandContext& ctx) {
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

    app.register_command("bench", [](const CommandContext& ctx) {
        global_state().bench();
    });

    app.register_command("perft", [](const CommandContext& ctx) {
        global_state().perft(int(ctx.int_after("")));
    });

    app.register_command("mperft", [](const CommandContext& ctx) {
        global_state().mperft(int(ctx.int_after("")));
    });

    app.register_command("isready", [](const CommandContext& ctx) {
        global_state().check_if_ready();
    });

    app.register_command("eval", [](const CommandContext& ctx) {
        global_state().evaluate();
    });

    app.register_command("go", [](const CommandContext& ctx) {
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

        if (ctx.has_arg("nodes")) {
            settings.max_nodes = ctx.int_after("nodes");
        }

        if (ctx.has_arg("searchmoves")) {
            // Parse all searchmoves.
            std::vector<Move> search_moves;
            std::string search_moves_str = ctx.all_after("searchmoves");
            ParseHelper parser(search_moves_str);
            const Board& board = global_state().board();
            while (!parser.finished()) {
                Move move = Move::parse_uci(board, parser.read_chunk());
                if (move == MOVE_NULL) {
                    break;
                }
                search_moves.push_back(move);
            }
            settings.search_moves = search_moves;
        }

        global_state().search(settings);
    });

    app.register_command("stop", [](const CommandContext& ctx) {
        global_state().stop_search();
    });

    app.register_command("quit", [](const CommandContext& ctx) {
        global_state().quit();
    });
}

} // illumina