#include "uciserver.h"
#include "illumina.h"

using namespace illumina;

struct State {

} s_state;

void cmd_uci(UCICommandContext& ctx) {
    std::cout << "id name Illumina v1.0" << std::endl;
    std::cout << "id author Thomas Mergener" << std::endl;

    for (const UCIOption& option: ctx.server().list_options()) {
        std::cout << "option name " << option.name() << " type " << option.type_name();
        if (option.has_value()) {
            std::cout << " default " << option.default_value_str();
        }
        if (option.has_min_value()) {
            std::cout << " min " << option.min_value_str();
        }
        if (option.has_max_value()) {
            std::cout << " max " << option.max_value_str();
        }
        std::cout << std::endl;
    }

    std::cout << "uciok" << std::endl;
}

void cmd_setoption(UCICommandContext& ctx) {
    std::string opt_name =  ctx.word_after("name");
    std::string value_str = ctx.word_after("value");

    ctx.server().option(opt_name).parse_and_set(value_str);
}

void register_commands(UCIServer& server) {
    server.register_command("uci", cmd_uci);
    server.register_command("setoption", cmd_setoption);
}

void register_options(UCIServer& server) {
    server.register_option<UCIOptionSpin>("Threads", 1, 1, 4);
}

int main(int argc, char* argv[]) {
    try {
        // Setup some C++ stuff.
        std::ios::sync_with_stdio(false);
        std::cin.tie();

        // Initialize illumina.
        illumina::init();

        // Perform UCI setup.
        UCIServer server;
        register_commands(server);
        register_options(server);

        server.set_error_handler([](UCIServer& server, const std::exception& err) {
            std::cerr << "Error:\n" << err.what() << std::endl;
        });

        // Finally, run.
        server.serve();

        return EXIT_SUCCESS;
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal:\n" << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}