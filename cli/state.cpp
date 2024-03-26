#include "state.h"

#include "movepicker.h"
#include "uciserver.h"
#include "evaluation.h"
#include "transpositiontable.h"

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

void State::new_game() const {
}

void State::display_board() const {
    std::cout << m_board.pretty()         << std::endl;
    std::cout << "FEN: " << m_board.fen() << std::endl;
}

const Board& State::board() const {
    return m_board;
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

void State::display_option_value(std::string_view opt_name) {
    std::cout << m_options.option(opt_name).current_value_str() << std::endl;
}

void State::set_option(std::string_view opt_name,
                       std::string_view value_str) {
    m_options.option(opt_name).parse_and_set(value_str);
}

void State::check_if_ready() {
    while (m_searching);
    std::cout << "readyok" << std::endl;
}

void State::evaluate() const {
    Evaluation eval;
    eval.on_new_board(m_board);
    std::cout << "info score cp " << eval.get() << std::endl;
}

static std::ostream& operator<<(std::ostream& stream, const std::vector<Move>& line) {
    for (Move m: line) {
        stream << m.to_uci() << ' ';
    }
    return stream;
}

static std::string score_string(Score score) {
    if (!is_mate_score(score)) {
        return "cp " + std::to_string(score);
    }
    int n_moves = moves_to_mate(score);
    return "mate " + std::to_string(score > 0 ? n_moves : -n_moves);
}

void State::setup_searcher() {
    m_searcher.set_pv_finish_listener([](const PVResults& res) {
        std::cout << "info"
                  << " depth " << res.depth
                  << " score " << score_string(res.score)
                  << " pv "    << res.line
                  << "nodes "  << res.nodes
                  << " nps "   << ui64((double(res.nodes) / (double(res.time) / 1000.0)))
                  << " time "  << res.time
                  << std::endl;
    });
}

void State::search(SearchSettings settings) {
    if (m_searching) {
        std::cerr << "Already searching." << std::endl;
        return;
    }
    m_searching = true;
    if (m_search_thread != nullptr) {
        m_search_thread->join();
        delete m_search_thread;
    }

    m_search_thread = new std::thread([this, settings]() {
        try {
            SearchResults results = m_searcher.search(m_board, settings);

            std::cout << "bestmove " << results.best_move.to_uci() << std::endl;

            m_searching = false;
        }
        catch (const std::exception& e) {
            std::cerr << "Unhandled exception during search: " << std::endl
                      << e.what() << std::endl;
            std::exit(EXIT_FAILURE);
        }
    });
}

void State::stop_search() {
    m_searcher.stop();
}

void State::quit() {
    std::exit(EXIT_SUCCESS);
}

void State::register_options() {
    m_options.register_option<UCIOptionSpin>("Hash", TT_DEFAULT_SIZE_MB, 1, 1024 * 1024)
        .add_update_handler([this](const UCIOption& opt) {
            const auto& spin = dynamic_cast<const UCIOptionSpin&>(opt);
            m_searcher.tt().resize(spin.value() * 1024 * 1024);
        });
}

State::State() {
    setup_searcher();
    register_options();
}

}