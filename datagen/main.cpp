#include <iostream>
#include <functional>
#include <string_view>
#include <unordered_map>

#include <illumina.h>

namespace illumina {

int run_normal_datagen(int argc, char* argv[]);
int run_qnet_datagen(int argc, char* argv[]);

static std::unordered_map<std::string, std::function<int(int, char**)>> s_modes = {
    { "normal", run_normal_datagen },
    { "qnet", run_qnet_datagen },
};

static const auto s_default_mode = run_normal_datagen;


void run_bench() {
    std::cout << "Running bench..." << std::endl;
    BenchResults res = bench();
    std::cout << "Finished bench."
              << "\n\tNodes: " << res.total_nodes
              << "\n\tNPS:   " << res.nps
              << std::endl;
}

static void print_usage(const char* program_name) {
    std::cout << "Usage:\n";

    for (const auto& mode: s_modes) {
        std::cout << "  " << program_name << " " << mode.first << " [options]\n";
    }

    std::cout << "  " << program_name << " [options]\n"
              << "\n"
              << "Scripts:\n"
              << "  normal    Main eval-net datagen.\n";
}

static bool is_help_token(std::string_view token) {
    return token == "-h" || token == "--help" || token == "help";
}

} // namespace illumina

int main(int argc, char* argv[]) {
    std::ios_base::sync_with_stdio(false);

    try {
        using namespace illumina;

        if (argc <= 1) {
            init();
            run_bench();
            return s_default_mode(argc, argv);
        }

        const std::string_view first_arg = argv[1];
        if (is_help_token(first_arg)) {
            print_usage(argv[0]);
            return EXIT_SUCCESS;
        }

        auto it = s_modes.find(std::string(first_arg));
        if (it != s_modes.end()) {
            if (argc > 2 && is_help_token(argv[2])) {
                return it->second(argc - 1, argv + 1);
            }

            init();
            run_bench();
            return it->second(argc - 1, argv + 1);
        }

        if (!first_arg.empty() && first_arg.front() == '-') {
            init();
            run_bench();
            return s_default_mode(argc, argv);
        }

        std::cerr << "Unknown datagen script: " << argv[1] << "\n\n";
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal:\n" << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
