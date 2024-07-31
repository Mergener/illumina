#include "state.h"

#include <type_traits>
#include <limits>
#include <iomanip>

#include "cliapplication.h"
#include "evaluation.h"
#include "endgame.h"
#include "transpositiontable.h"
#include "tunablevalues.h"

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

void State::new_game() {
    m_searcher.tt().clear();
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

void State::mperft(int depth) const {
    illumina::move_picker_perft(m_board, depth, { true });
}

void State::uci() {
    std::cout << "id name Illumina " << ILLUMINA_VERSION_NAME << std::endl;
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
    std::cout << "readyok" << std::endl;
}

void State::evaluate() const {
    if (m_board.in_check()) {
        // Positions in check don't have a static evaluation.
        std::cout << "Final evaluation: None (check)" << std::endl;
        return;
    }

    Endgame endgame = identify_endgame(m_board);
    if (endgame.type != EG_UNKNOWN) {
        std::cout << "Using endgame evaluation." << std::endl;
        std::cout << "\n\nFinal evaluation ("
                  << (m_board.color_to_move() == CL_WHITE ? "white" : "black") << "'s perspective): "
                  << double(endgame.evaluation) / 100.0 << " (" << endgame.evaluation << " cp)"
                  << std::endl;
        return;
    }

    Evaluation eval;
    Board repl = m_board;
    eval.on_new_board(repl);
    Score score = eval.get();

    std::cout << "      ";

    for (BoardFile f: FILES) {
        std::cout << " " << file_to_char(f) << "    ";
    }
    std::cout << "\n    -------------------------------------------------";

    for (BoardRank r: RANKS_REVERSE) {
        std::cout << std::endl;
        std::cout << " " << rank_to_char(r) << " |";
        for (BoardFile f: FILES) {
            Square s = make_square(f, r);
            Piece p = repl.piece_at(s);
            if (p == PIECE_NULL) {
                std::cout << "      ";
            }
            else if (p.type() == PT_KING) {
                std::cout << "   " << '*' << "  ";
            }
            else {
                repl.set_piece_at(s, PIECE_NULL);
                eval.on_new_board(repl);
                Score score_without_piece = eval.get();
                repl.set_piece_at(s, p);

                std::cout << std::setw(6)
                          << std::fixed
                          << std::setprecision(2)
                          << double(score - score_without_piece) / 100;
            }
        }

        std::cout << " |" << std::endl << "   |";
        for (BoardFile f: FILES) {
            Square s = make_square(f, r);
            Piece p = repl.piece_at(s);
            if (p == PIECE_NULL) {
                std::cout << "      ";
            }
            else {
                std::cout << "   " << p.to_char() << "  ";
            }
        }

        std::cout << " |";
    }
    std::cout << "\n    -------------------------------------------------";

    std::cout << "\n\nFinal evaluation ("
              << (m_board.color_to_move() == CL_WHITE ? "white" : "black") << "'s perspective): "
              << double(score) / 100
              << " (" << score << " cp)"
              << std::endl;
}

static std::string pv_to_string(const std::vector<Move>& line, bool frc) {
    if (line.empty()) {
        return "";
    }

    std::stringstream stream;
    stream << line[0].to_uci(frc);
    for (auto it = line.begin() + 1; it != line.end(); ++it) {
        Move m = *it;
        if (m == MOVE_NULL) {
            break;
        }
        stream << ' ' << m.to_uci(frc);
    }
    return stream.str();

}

static std::string score_string(Score score) {
    if (!is_mate_score(score)) {
        return "cp " + std::to_string(score);
    }
    int n_moves = moves_to_mate(score);
    return "mate " + std::to_string(score > 0 ? n_moves : -n_moves);
}

static std::string bound_type_string(BoundType boundType) {
    switch (boundType) {
        case BT_EXACT:      return "";
        case BT_LOWERBOUND: return " lowerbound";
        case BT_UPPERBOUND: return " upperbound";
    }
    return "";
}

static std::string multipv_string(bool multi_pv, int pv_idx) {
    if (!multi_pv) {
        return "";
    }
    return " multipv " + std::to_string(pv_idx + 1);
}

void State::setup_searcher() {
    m_searcher.set_currmove_listener([this](Depth depth, Move move, int move_num) {
        if (depth < 6 || delta_ms(Clock::now(), m_search_start) <= 3000) {
            // Don't flood stdout on the first few seconds.
            return;
        }

        std::cout << "info"
                  << " depth "       << depth
                  << " currmove "    << move.to_uci(m_frc)
                  << " currmovenumber " << move_num
                  << std::endl;
    });

    m_searcher.set_pv_finish_listener([this](const PVResults& res) {
        // Check if we need to log multipv.
        UCIOptionSpin& opt_multi_pv = m_options.option<UCIOptionSpin>("MultiPV");

        std::cout << "info"
                  << multipv_string(opt_multi_pv.value() > 1, res.pv_idx)
                  << " depth "    << res.depth
                  << " seldepth " << res.sel_depth
                  << " score "    << score_string(res.score)
                  << bound_type_string(res.bound_type);
        if (res.line.size() >= 1 && res.line[0] != MOVE_NULL)
        std::cout << " pv "       << pv_to_string(res.line, m_frc);
        std::cout << " hashfull " << m_searcher.tt().hash_full()
                  << " nodes "    << res.nodes
                  << " nps "      << ui64((double(res.nodes) / (double(res.time) / 1000.0)))
                  << " time "     << res.time
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

    settings.contempt = m_options.option<UCIOptionSpin>("Contempt").value();
    settings.n_pvs    = m_options.option<UCIOptionSpin>("MultiPV").value();

    m_search_thread = new std::thread([this, settings]() {
        try {
            m_search_start = Clock::now();
            SearchResults results = m_searcher.search(m_board, settings);

            std::cout << "bestmove " << results.best_move.to_uci(m_frc);

            if (results.ponder_move != MOVE_NULL) {
                std::cout << " ponder " << results.ponder_move.to_uci(m_frc);
            }

            std::cout << std::endl;

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

#ifdef TUNING_BUILD
static void add_tuning_option(UCIOptionManager& options,
                              const std::string& opt_name,
                              int& opt_ref,
                              int default_value,
                              int min = INT_MIN,
                              int max = INT_MAX) {
    options.register_option<UCIOptionSpin>(opt_name, default_value, min, max)
            .add_update_handler([&opt_ref](const UCIOption& opt) {
                const auto& spin = dynamic_cast<const UCIOptionSpin&>(opt);
                opt_ref = spin.value();
                recompute_search_constants();
            });
}

static void add_tuning_option(UCIOptionManager& options,
                              const std::string& opt_name,
                              double& opt_ref,
                              double default_value,
                              double min = -0x100000,
                              double max = 0x100000) {
    options.register_option<UCIOptionSpin>(opt_name + std::string("_FP"),
                                           default_value * 1000,
                                           min * 1000,
                                           max * 1000)
            .add_update_handler([&opt_ref](const UCIOption& opt) {
                const auto& spin = dynamic_cast<const UCIOptionSpin&>(opt);
                opt_ref = double(spin.value()) / 1000.0;
                recompute_search_constants();
            });
}
#endif

void State::register_options() {
    m_options.register_option<UCIOptionSpin>("Hash", TT_DEFAULT_SIZE_MB, 1, 1024 * 1024)
        .add_update_handler([this](const UCIOption& opt) {
            const auto& spin = dynamic_cast<const UCIOptionSpin&>(opt);
            m_searcher.tt().resize(spin.value() * 1024 * 1024);
        });

    m_options.register_option<UCIOptionSpin>("Threads", 1, 1, 1);
    m_options.register_option<UCIOptionSpin>("MultiPV", 1, 1, MAX_PVS);
    m_options.register_option<UCIOptionSpin>("Contempt", 0, -MAX_SCORE, MAX_SCORE);
    m_options.register_option<UCIOptionCheck>("UCI_Chess960", false)
        .add_update_handler([this](const UCIOption& opt) {
            const auto& check = dynamic_cast<const UCIOptionCheck&>(opt);
            m_frc = check.value();
        });

#ifdef TUNING_BUILD
#define TUNABLE_VALUE(name, type, ...) add_tuning_option(m_options, \
                                                         std::string("TUNABLE_") + #name, \
                                                         name, \
                                                         __VA_ARGS__);
#include "tunablevalues.def"
#endif
}

State::State() {
    setup_searcher();
    register_options();
}

}