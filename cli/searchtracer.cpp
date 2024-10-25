#include "searchtracer.h"

#include "utils.h"

namespace illumina {

static ui64 random_ui64() {
    return random(ui64(0), UINT64_MAX);
}

void SearchTracer::new_search(const Board& root,
                              size_t hash_size_mb,
                              const SearchSettings& settings) {
    m_curr_search = {};

    m_curr_search.id = random_ui64();
    m_curr_search.hash     = hash_size_mb;
    m_curr_search.root_fen = root.fen(root.detect_frc());

    m_curr_search.limits_wtime = settings.white_time.value_or(0);
    m_curr_search.limits_winc  = settings.white_inc.value_or(0);
    m_curr_search.limits_btime = settings.black_time.value_or(0);
    m_curr_search.limits_binc  = settings.black_inc.value_or(0);
    m_curr_search.limits_nodes = settings.max_nodes;
    m_curr_search.limits_movetime = settings.move_time.value_or(0);
}

void SearchTracer::new_tree(int root_depth, int multi_pv, int asp_alpha, int asp_beta) {
    m_curr_tree = {};
    m_curr_tree.id        = random_ui64();
    m_curr_tree.search    = m_curr_search.id;
    m_curr_tree.asp_alpha = asp_alpha;
    m_curr_tree.asp_beta  = asp_beta;
    m_curr_tree.multipv   = multi_pv;
}

void SearchTracer::push_node(Move move) {
    NodeInfo new_node;
    new_node.index = m_curr_tree.next_node_index++;
    new_node.parent_index = m_curr_node.index;
    new_node.tree = m_curr_tree.id;
    new_node.last_move = move;
    new_node.next_child_order = m_curr_node.next_child_order++;

    m_node_stack.push_back(m_curr_node);
    m_curr_node = new_node;
}

void SearchTracer::push_sibling_node() {
    NodeInfo new_node = m_curr_node;
    new_node.index = m_curr_tree.next_node_index++;
    m_node_stack.push_back(m_curr_node);
    m_curr_node = new_node;
}

void SearchTracer::pop_node(bool discard) {
    if (m_node_stack.empty()) {
        return;
    }

    if (!discard) {
        if (m_node_batch.size() >= m_target_node_count) {
            flush_nodes();
        }
        m_node_batch.push_back(m_curr_node);
    }

    m_curr_node = m_node_stack.back();
    m_node_stack.pop_back();
}

void SearchTracer::set_int_value(TraceInt which, i64 value) {
    switch (which) {
        case TRACE_V_BETA:
            m_curr_node.beta = i32(value);
            break;
        case TRACE_V_ALPHA:
            m_curr_node.alpha = i32(value);
            break;
        case TRACE_V_STATIC_EVAL:
            m_curr_node.static_eval = i16(value);
            break;
        case TRACE_V_SCORE:
            m_curr_node.score = i32(value);
            break;
        case TRACE_V_DEPTH:
            m_curr_node.depth = i8(value);
            break;
        default:
            break;
    }
}

void SearchTracer::set_flag(TraceFlag which) {
    switch (which) {
        case TRACE_F_QSEARCH:
            m_curr_node.qsearch = true;
            break;

        default:
            break;
    }

}

void SearchTracer::set_best_move(Move move) {
    m_curr_node.best_move = move;
}

void SearchTracer::finish_tree() {
    save_tree(m_curr_tree);
    std::cout << "info string Saved ID/MultiPV/AspWin search tree with id " << m_curr_tree.id << std::endl;
    m_curr_tree = {};

    // Push root node to batch.
    ILLUMINA_ASSERT(m_curr_node.parent_index == 0);
    m_node_batch.push_back(m_curr_node);
}

void SearchTracer::finish_search() {
    flush_nodes();
    save_search(m_curr_search);
    std::cout << "info string Saved search with id " << m_curr_search.id << std::endl;
    m_curr_search = {};
}

struct FastMoveSerializer {
    char chars[6];

    FastMoveSerializer(Move move) {
        Square src = move.source();
        Square dst = move.destination();

        chars[0] = file_to_char(square_file(src));
        chars[1] = rank_to_char(square_rank(src));
        chars[2] = file_to_char(square_file(dst));
        chars[3] = rank_to_char(square_rank(dst));
        chars[4] = move.is_promotion() ? piece_type_to_char(move.promotion_piece_type()) : '\0';
        chars[5] = '\0';
    }
};

void SearchTracer::flush_nodes() {
    if (m_target_node_count >= 16384) {
        std::cout << "info string Saving " << m_node_batch.size() << " nodes to trace db" << std::endl;
    }

    try {
        SQLite::Transaction transaction(m_db);

        SQLite::Statement insert(m_db,
                                 "INSERT OR IGNORE INTO NODES (node_index, node_order, parent_index, tree, last_move, "
                                 "alpha, beta, depth, static_eval, score, best_move) "
                                 "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");

        for (const NodeInfo& node : m_node_batch) {
            insert.bind(1, i64(node.index));
            insert.bind(2, int(node.node_order));
            insert.bind(3, i64(node.parent_index));
            insert.bind(4, node.tree);
            insert.bind(5, FastMoveSerializer(node.last_move).chars);
            insert.bind(6, node.alpha);
            insert.bind(7, node.beta);
            insert.bind(8, int(node.depth));
            insert.bind(9, node.static_eval);
            insert.bind(10, node.score);
            insert.bind(11, FastMoveSerializer(node.best_move).chars);

            insert.exec();
            insert.reset();
        }

        transaction.commit();
    } catch (const std::exception& e) {
        std::cerr << "Query failed: " << e.what() << std::endl;
        throw;
    }
    m_node_batch.clear();
}

void SearchTracer::bootstrap_db() {
    std::stringstream ss;

    // Create metadata table.
    if (!m_db.tableExists("meta")) {
        m_db.exec(
            "CREATE TABLE meta (\n"
            "    id INTEGER PRIMARY KEY CHECK(id = 1), \n"
            "    version_id TEXT\n"
            ");\n"
            "INSERT INTO meta (version_id) VALUES (\'" + std::string(TRACER_VERSION) + "\');"
            "\n");
    }
    else {
        // Metadata table existed, check for version.
        SQLite::Column col = m_db.execAndGet("SELECT version_id FROM meta");
        std::string version_id = col.getText();
        if (version_id != TRACER_VERSION) {
            throw std::runtime_error("Specified trace DB has incompatible version.");
        }
    }

    // Create searches table.
    ss << "CREATE TABLE IF NOT EXISTS searches (\n"
          "    id INTEGER PRIMARY KEY, \n"
          "    time_of_creation TIMESTAMP DEFAULT CURRENT_TIMESTAMP,\n"
          "    depth INTEGER,\n"
          "    nodes INTEGER,\n"
          "    wtime INTEGER,\n"
          "    btime INTEGER,\n"
          "    winc INTEGER,\n"
          "    binc INTEGER,\n"
          "    movetime INTEGER,\n"
          "    hash INTEGER,\n"
          "    root_fen TEXT\n"
          ");\n"
       << std::endl;

    // Create trees table.
    ss << "CREATE TABLE IF NOT EXISTS trees (\n"
          "    id INTEGER,\n"
          "    search INTEGER,\n"
          "    root_depth INTEGER,\n"
          "    asp_alpha INTEGER,\n"
          "    asp_beta INTEGER,\n"
          "    multipv INTEGER,\n"
          "    PRIMARY KEY (id, search),  \n"
          "    FOREIGN KEY (search) REFERENCES searches(id) ON DELETE CASCADE  \n"
          ");\n"
       << std::endl;

    // Create nodes table.
    ss << "CREATE TABLE IF NOT EXISTS nodes (\n"
          "    node_index INTEGER,\n"
          "    node_order INTEGER,\n"
          "    parent_index INTEGER,\n"
          "    tree INTEGER,\n"
          "    last_move TEXT CHECK(length(last_move) <= 5),\n"
          "    alpha INTEGER,\n"
          "    beta INTEGER,\n"
          "    depth INTEGER,\n"
          "    static_eval INTEGER,\n"
          "    score INTEGER,\n"
          "    best_move TEXT,\n"
          "    PRIMARY KEY (node_index, tree, parent_index),\n"
          "    FOREIGN KEY (tree) REFERENCES trees(id) ON DELETE CASCADE,\n"
          "    FOREIGN KEY (parent_index) REFERENCES nodes(node_index) ON DELETE SET NULL\n"
          ");\n"
          "\n"
          "CREATE INDEX IF NOT EXISTS idx_node_index ON nodes(node_index);\n"
          "CREATE INDEX IF NOT EXISTS idx_parent ON nodes(parent_index);"
          << std::endl;

    std::string query = ss.str();
    m_db.exec(query);
}

void SearchTracer::save_tree(const TreeInfo& tree) {

}

void SearchTracer::save_search(const SearchInfo& search) {

}

SearchTracer::SearchTracer(const std::string& db_path,
                           size_t batch_size_mib)
    : m_target_node_count((batch_size_mib * 1024 * 1024) / sizeof(NodeInfo)),
      m_db(db_path, SQLite::OPEN_CREATE | SQLite::OPEN_READWRITE) {
    m_node_batch.reserve(m_target_node_count + 3);
    m_node_stack.reserve(MAX_DEPTH);

    bootstrap_db();
}

} // illumina
