#include "illumina.h"

#include "commands.h"
#include "cliapplication.h"
#include "state.h"

using namespace illumina;

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

        // Finally, run.
        std::cout << "Illumina " << ILLUMINA_VERSION_NAME << std::endl;
#ifdef TUNING_BUILD
        std::cout << "This is a tuning build. Engine constants can be changed using UCI options." << std::endl;
#endif

        server.listen();

        return EXIT_SUCCESS;
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal:\n" << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}