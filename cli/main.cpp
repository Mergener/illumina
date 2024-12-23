#include "illumina.h"

#include "commands.h"
#include "cliapplication.h"
#include "state.h"

#include "rang.h"

using namespace illumina;

static bool has_cli_arg(int argc, char* argv[], std::string_view arg);
static void display_hello_text();

int main(int argc, char* argv[]) {
    try {
        // Setup some C++ stuff.
        std::ios::sync_with_stdio(false);
        std::cin.tie();

        // Initialize illumina.
        illumina::init();

        // Perform UCI setup.
        CLIApplication server;
        register_commands(server);

        server.set_error_handler([](CLIApplication& server, const std::exception& err) {
            std::cerr << "Error:\n" << err.what() << std::endl;
        });

        // Initialize program state.
        initialize_global_state();

        if (has_cli_arg(argc, argv, "bench")) {
            server.handle("bench");
            server.handle("quit");
            return 0;
        }

        // Finally, run.
        display_hello_text();

#ifdef TUNING_BUILD
        std::cout << "This is a tuning build. Engine constants can be changed using UCI options." << std::endl;
#endif
#ifdef USE_ASSERTS
        std::cout << "Assertions are enabled for this build." << std::endl;
#endif

        server.listen();

        return EXIT_SUCCESS;
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal:\n" << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}

static bool has_cli_arg(int argc, char* argv[], std::string_view arg) {
    for (int i = 0; i < argc; ++i) {
        if (std::string(argv[i]) == arg) {
            return true;
        }
    }
    return false;
}

static void display_hello_text() {
    using namespace rang;

    std::cout << style::bold
              << fg::yellow;

    std::cout << R"(
  ___ _ _                 _
 |_ _| | |_   _ _ __ ___ (_)_ __   __ _
  | || | | | | | '_ ` _ \| | '_ \ / _` |
  | || | | |_| | | | | | | | | | | (_| |
 |___|_|_|\__,_|_| |_| |_|_|_| |_|\__,_|

)"
    << " by Thomas Mergener" << std::endl
    << " version " << ILLUMINA_VERSION_NAME << std::endl
    << style::reset
    << fg::reset
    << std::endl;
}