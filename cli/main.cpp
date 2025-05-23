#include "illumina.h"

#include "commands.h"
#include "cliapplication.h"
#include "state.h"

#include "rang.h"

using namespace illumina;

static void display_hello_text();

int main(int argc, char* argv[]) {
    try {
        // Initialize illumina.
        illumina::init();

        // Perform UCI setup.
        CLIApplication app;
        register_commands(app);

        app.set_error_handler([](CLIApplication& server, const std::exception& err) {
            std::cerr << "Error:\n" << err.what() << std::endl;
        });

        // Initialize program state.
        initialize_global_state();

        // If command line arguments were specified, execute them.
        if (argc > 1) {
            for (int i = 1; i < argc; ++i) {
                app.handle(argv[i]);
                while (global_state().searching());
            }
            app.handle("quit");
        }

        // Finally, run.
        display_hello_text();

#ifdef TUNING_BUILD
        std::cout << "This is a tuning build. Engine constants can be changed using UCI options." << std::endl;
#endif
#ifdef USE_ASSERTS
        std::cout << "Assertions are enabled for this build." << std::endl;
#endif

        app.listen();

        return EXIT_SUCCESS;
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal:\n" << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}

static void display_hello_text() {
    using namespace rang;

    std::cout << fg::blue;

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