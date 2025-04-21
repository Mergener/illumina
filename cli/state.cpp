#include "state.h"

#include <type_traits>
#include <climits>
#include <iomanip>

#include "bench.h"
#include "cliapplication.h"
#include "evaluation.h"
#include "endgame.h"
#include "transpositiontable.h"
#include "tunablevalues.h"

#ifdef TRACING_ENABLED
#include "searchtracer.h"
#endif

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

bool State::searching() const {
    return m_searching.load(std::memory_order_relaxed);
}

State& global_state() {
    ILLUMINA_ASSERT(s_global_state != nullptr);
    return *s_global_state;
}

//
// State methods
//

static std::string score_string(Score score) {
    if (!is_mate_score(score)) {
        return "cp " + std::to_string(score);
    }
    int n_moves = moves_to_mate(score);
    return "mate " + std::to_string(score > 0 ? n_moves : -n_moves);
}

static std::string wdl_string(const WDL& wdl) {
    return std::to_string(wdl.w) + " " + std::to_string(wdl.d) + " " + std::to_string(wdl.l);
}

void State::new_game() {
    m_searcher.tt().clear();
    m_eval_random_seed = random(ui64(1), UINT64_MAX);
}

void State::display_board() const {
    std::cout << m_board.pretty()         << std::endl;
    std::cout << "FEN: " << m_board.fen(m_options.option<UCIOptionCheck>("UCI_Chess960").value()) << std::endl;
    std::cout << "Zob Key: 0x" << std::hex << m_board.hash_key() << std::dec << std::endl;
    std::cout << "Zob Key (i64, base 10): " << i64(m_board.hash_key()) << std::endl;
}

const Board& State::board() const {
    return m_board;
}

void State::set_board(const Board& board) {
    m_board = board;
}

void State::bench() const {
#ifndef OPENBENCH_COMPLIANCE
    BenchSettings settings = default_bench_settings();

    std::cout << "Starting bench...\n" << std::endl;
    std::cout << "Bench hash size is " << settings.hash_size_mb << " MiB (" << settings.hash_size_mb * 1024 * 1024 << " bytes)" << std::endl;
    std::cout << "Bench search depth is " << settings.search_settings.max_depth.value() << std::endl;
    std::cout << std::endl;

    settings.on_board_searched = [](const Board& board,
                                    Score score,
                                    Move move) {
        std::cout << "position " << board.fen()
                  << " bestmove " << move.to_uci()
                  << " score " << score_string(score)
                  << std::endl;
    };
    BenchResults results = illumina::bench(settings);

    std::cout << "\nBench finished." << std::endl;
    std::cout << "\tTotal bench time:     "  << results.bench_time_ms << " ms" << std::endl;
    std::cout << "\tTotal search time:    "  << results.search_time_ms << " ms" << std::endl;
    std::cout << "\tTotal searched nodes: "  << results.total_nodes << std::endl;
    std::cout << "\tNodes/sec:            "  << results.nps << std::endl;
#else
    BenchSettings settings = default_bench_settings();
    BenchResults results = illumina::bench(settings);
    std::cout << results.total_nodes << " nodes " << results.nps << " nps" << std::endl;
#endif
}

void State::perft(int depth, bool bulk) const {
    illumina::perft(m_board, depth, { true, false, bulk });
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
    Score score = normalize_score_if_desired(eval.get(), repl);

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
                Score score_without_piece = normalize_score_if_desired(eval.get(), repl);
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

static std::string pv_to_string(const std::vector<Move>& line,
                                const Board& board,
                                bool frc) {
    if (line.empty()) {
        return "";
    }

    Board repl = board;

    std::stringstream stream;
    stream << line[0].to_uci(frc);
    repl.make_move(line[0]);
    for (auto it = line.begin() + 1; it != line.end(); ++it) {
        Move m = *it;
        if (!repl.is_move_pseudo_legal(m) || !repl.is_move_legal(m)) {
            break;
        }
        stream << ' ' << m.to_uci(frc);
        repl.make_move(m);
    }
    return stream.str();

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
        bool show_wdl = m_options.option<UCIOptionCheck>("UCI_ShowWDL").value();
        bool has_pv_line = res.line.size() >= 1 && res.line[0] != MOVE_NULL;

        std::cout << "info"
                  << multipv_string(opt_multi_pv.value() > 1, res.pv_idx)
                  << " depth "    << res.depth
                  << " seldepth " << res.sel_depth
                  << " score "    << score_string(normalize_score_if_desired(res.score, m_board))
                  << (show_wdl ? " wdl " + wdl_string(wdl_from_score(res.score, m_board)) : "")
                  << bound_type_string(res.bound_type)
                  << " pv "       << (has_pv_line ? pv_to_string(res.line, m_board, m_frc) : "")
                  << " hashfull " << m_searcher.tt().hash_full()
                  << " nodes "    << res.nodes
                  << " nps "      << ui64((double(res.nodes) / (double(res.time) / 1000.0)))
                  << " time "     << res.time
                  << std::endl;
    });
}

void State::search(SearchSettings settings, bool trace) {
    // Prepare some search settings before searching based on UCI options.
    settings.contempt  = m_options.option<UCIOptionSpin>("Contempt").value();
    settings.n_pvs     = m_options.option<UCIOptionSpin>("MultiPV").value();
    settings.n_threads = m_options.option<UCIOptionSpin>("Threads").value();
    settings.eval_random_margin = m_options.option<UCIOptionSpin>("EvalRandomMargin").value();
    settings.eval_rand_seed     = m_eval_random_seed;

    // User might want to override number of search nodes.
    // This is useful when performing node-odds testing on a GUI that
    // doesn't support this type of odds. The GUI can send go nodes X
    // and OverrideNodesLimit allows Illumina to bypass this instruction.
    ui64 node_option = m_options.option<UCIOptionSpin>("OverrideNodesLimit").value();
    if (node_option != 0) {
        settings.max_nodes = node_option;
    }

    // Setup search tracing (if build supports it).
    std::shared_ptr<ISearchTracer> tracer = nullptr;
    if (trace) {
#ifndef TRACING_ENABLED
        std::cout << "info string Tracing is not enabled in this version -- option skipped." << std::endl;
#else
        try {
            std::string trace_path = m_options.option<UCIOptionString>("TraceFile").value();
            size_t trace_batch_size = m_options.option<UCIOptionSpin>("TraceBatchSize").value();
            tracer = std::make_shared<SearchTracer>(trace_path, trace_batch_size);
            settings.tracer = tracer.get();
        }
        catch (const std::exception& e) {
            std::cerr << "Caught exception when initializing tracer (search will continue but tracing will be disabled):\n" << e.what() << std::endl;
            settings.tracer = nullptr;
        }
#endif
    }

    // Prevent invoking two simultaneous searches.
    if (m_searching.exchange(true, std::memory_order_acquire)) {
        stop_search();
    }

    // Finally, fire the search thread.
    // Note that we need to capture the tracer in the lambda in order
    // to keep the tracer object alive.
    m_search_thread = new std::thread([this, settings, tracer]() {
        try {
            m_search_start = Clock::now();
            SearchResults results = m_searcher.search(m_board, settings);

            std::cout << "bestmove " << results.best_move.to_uci(m_frc);

            if (results.ponder_move != MOVE_NULL) {
                std::cout << " ponder " << results.ponder_move.to_uci(m_frc);
            }

            std::cout << std::endl;

            m_searching.store(false, std::memory_order_release);
        }
        catch (const std::exception& e) {
            std::cerr << "Unhandled exception during search: " << std::endl
                      << e.what() << std::endl;
            std::exit(EXIT_FAILURE);
        }
    });
}

Score State::normalize_score_if_desired(Score score, const Board& board) const {
    if (!m_options.option<UCIOptionCheck>("NormalizeScores").value()) {
        return score;
    }
    return normalize_score(score, board);
}

void State::stop_search() {
    m_searcher.stop();
    if (m_search_thread != nullptr) {
        m_search_thread->join();
        delete m_search_thread;
    }
}

void State::quit() {
    std::exit(EXIT_SUCCESS);
}

#ifdef TUNING_BUILD

template <typename T>
struct TuningOption {
    std::string name;
    T base;
    T min;
    T max;
    T step;
};

static std::vector<TuningOption<double>> s_tunable_doubles;
static std::vector<TuningOption<i64>> s_tunable_ints;

void State::display_ob_tuning_params() {
    for (const auto& tunable: s_tunable_ints) {
        std::cout << tunable.name
                  << ", " << "int"
                  << ", " << tunable.base
                  << ", " << tunable.min
                  << ", " << tunable.max
                  << ", " << tunable.step
                  << ", 0.002" << std::endl; // R_END
    }
    for (const auto& tunable: s_tunable_doubles) {
        std::cout << tunable.name
                  << ", " << "float"
                  << ", " << tunable.base
                  << ", " << tunable.min
                  << ", " << tunable.max
                  << ", " << tunable.step
                  << ", 0.002" << std::endl; // R_END
    }
}

static void add_tuning_option(UCIOptionManager& options,
                              const std::string& opt_name,
                              int& opt_ref,
                              int default_value,
                              int min = INT_MIN,
                              int max = INT_MAX,
                              int step = 1) {
    auto& option = options.register_option<UCIOptionSpin>(opt_name, default_value, min, max);

    option.add_update_handler([&opt_ref](const UCIOption& opt) {
       const auto& spin = dynamic_cast<const UCIOptionSpin&>(opt);
       opt_ref = spin.value();
       recompute_search_constants();
   });

    s_tunable_ints.push_back(
        { option.name(), default_value, min, max, step }
    );
}

static void add_tuning_option(UCIOptionManager& options,
                              const std::string& opt_name,
                              double& opt_ref,
                              double default_value,
                              double min = -0x100000,
                              double max = 0x100000,
                              double step = 0.1) {
#ifndef OPENBENCH_COMPLIANCE
    auto& option = options.register_option<UCIOptionSpin>(opt_name + std::string("_FP"),
                                           default_value * 1000,
                                           min * 1000,
                                           max * 1000);
    option.add_update_handler([&opt_ref](const UCIOption& opt) {
        const auto& spin = dynamic_cast<const UCIOptionSpin&>(opt);
        opt_ref = double(spin.value()) / 1000.0;
        recompute_search_constants();
    });

#else
    auto& option = options.register_option<UCIOptionString>(opt_name + std::string("_FP"),
                                             std::to_string(default_value));
    option.add_update_handler([&opt_ref](const UCIOption& opt) {
        const auto& spin = dynamic_cast<const UCIOptionString&>(opt);
        opt_ref = std::stod(spin.value());
        recompute_search_constants();
    });
#endif
    s_tunable_doubles.push_back(
        { option.name(), default_value, min, max, step }
    );
}
#endif

void State::register_options() {
    m_options.register_option<UCIOptionSpin>("Hash", TT_DEFAULT_SIZE_MB, 1, 1024 * 1024)
        .add_update_handler([this](const UCIOption& opt) {
            const auto& spin = dynamic_cast<const UCIOptionSpin&>(opt);
            m_searcher.tt().resize(spin.value() * 1024 * 1024);
        });

    m_options.register_option<UCIOptionSpin>("Threads", 1, 1, UINT16_MAX);
    m_options.register_option<UCIOptionSpin>("MultiPV", 1, 1, MAX_PVS);
    m_options.register_option<UCIOptionSpin>("Contempt", 0, -MAX_SCORE, MAX_SCORE);
    m_options.register_option<UCIOptionCheck>("UCI_Chess960", false)
        .add_update_handler([this](const UCIOption& opt) {
            const auto& check = dynamic_cast<const UCIOptionCheck&>(opt);
            m_frc = check.value();
        });
    m_options.register_option<UCIOptionSpin>("EvalRandomMargin", 0, 0, 1024);
    m_options.register_option<UCIOptionSpin>("OverrideNodesLimit", 0, 0, INT32_MAX);
    m_options.register_option<UCIOptionCheck>("NormalizeScores", true);
    m_options.register_option<UCIOptionCheck>("UCI_ShowWDL", false);

#ifdef TUNING_BUILD
#define TUNABLE_VALUE(name, type, ...) add_tuning_option(m_options, \
                                                         std::string("TUNABLE_") + #name, \
                                                         name, \
                                                         __VA_ARGS__);
#include "tunablevalues.def"
#endif

#ifdef TRACING_ENABLED
    m_options.register_option<UCIOptionString>("TraceFile", "traces.db");
    m_options.register_option<UCIOptionSpin>("TraceBatchSize", DEFAULT_TRACER_BATCH_SIZE, 0, INT32_MAX);
#endif
}

State::State() {
    m_eval_random_seed = random(ui64(1), UINT64_MAX);
    setup_searcher();
    register_options();
}

}
