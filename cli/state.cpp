#include "state.h"

#include "uciserver.h"

namespace illumina {

//
// Global state
//

static State* s_global_state = nullptr;

void initialize_global_state() {
    if (s_global_state) {
        return;
    }

    if (!illumina::initialized()) {
        throw std::logic_error("Tried to initialize global state before initializing Illumina.");
    }

    s_global_state = new State();
}

State& global_state() {
    ILLUMINA_ASSERT(s_global_state != nullptr);
    return *s_global_state;
}

//
// State methods
//

const Board& State::board() const {
    return m_board;
}

void State::display_board() const {
    std::cout << m_board.pretty()         << std::endl;
    std::cout << "FEN: " << m_board.fen() << std::endl;
}

void State::set_board(const Board& board) {
    m_board = board;
}

void State::perft(int depth) const {
    illumina::perft(m_board, depth, { true });
}

void State::uci() {
    std::cout << "id name Illumina v1.0 " << std::endl;
    std::cout << "id author Thomas Mergener" << std::endl;

    for (const UCIOption& option: m_options.list_options()) {
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

void State::set_option(std::string_view opt_name,
                       std::string_view value_str) {
    m_options.option(opt_name).parse_and_set(value_str);
}

void State::check_if_ready() {
    std::cout << "readyok" << std::endl;
}

void State::quit() {
    std::exit(EXIT_SUCCESS);
}

State::State() = default;

}