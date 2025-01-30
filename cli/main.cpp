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

        // If command line arguments were specified, execute them.
        if (argc > 1) {
            for (int i = 1; i < argc; ++i) {
                server.handle(argv[i]);
                while (global_state().searching());
            }
            server.handle("quit");
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