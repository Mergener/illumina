#include "searchtracer.h"

#include <sstream>
#include "utils.h"

namespace illumina {

static ui64 random_ui64() {
    return random(ui64(0), UINT64_MAX);
}

template <typename T>
void bind_to_statement(SQLite::Statement& stmt,
                       int index,
                       const T& value) {
    stmt.bind(index, value);
}

template <>
void bind_to_statement(SQLite::Statement& stmt,
                       int index,
                       const Move& move) {
    stmt.bind(index, move.to_uci());
}

/**
 * Binds a traced value to a SQLite statement.
 * Typename T is the target type to bind the value, declared in traceables.def.
 * The traced value's current state might match T, in which we will store it.
 * If the traced value's current type doesn't match T, we assume the value is either
 * invalid or wasn't set, so we assign a default value.
 */
template <typename T>
void bind_to_statement(SQLite::Statement& stmt,
                       int index,
                       const TracedValue& value) {
    std::visit([&stmt, index, &value](const auto& v) {
        using TVal = std::decay_t<decltype(v)>;

        if constexpr (std::is_same_v<TVal, T>) {
            bind_to_statement(stmt, index, T(v));
        }
        else if constexpr (std::is_same_v<TVal, std::monostate>) {
            bind_to_statement(stmt, index, T());
        }
        else {
            std::stringstream ss;
            ss << "Unexpected traced value " << std::get<TVal>(value) << std::endl;
            throw std::runtime_error(ss.str());
        }
    }, value);
}

void SearchTracer::set(Traceable which, TracedValue value) {
    m_curr_node.traced_values[int(which)] = value;
}

template <typename T>
std::string sql_type_map() {
    if constexpr (std::is_same_v<T, i64>) {
        return "INTEGER";
    }
    else if constexpr (std::is_same_v<T, bool>) {
        return "BOOLEAN";
    }
    else if constexpr (std::is_same_v<T, std::string>) {
        return "TEXT";
    }
    else if constexpr (std::is_same_v<T, Move>) {
        return "TEXT";
    }
    else if constexpr (std::is_same_v<T, double>) {
        return "REAL";
    }
    else {
        static_assert(!std::is_same<T,T>::value, "Unsupported traceable type.");
    }
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

    m_curr_tree = {};
    m_curr_tree.search = m_curr_search.id;
}

void SearchTracer::new_tree(int root_depth, int multi_pv, int asp_alpha, int asp_beta) {
    m_curr_tree = {};
    m_curr_tree.id         = random_ui64();
    m_curr_tree.search     = m_curr_search.id;
    m_curr_tree.asp_alpha  = asp_alpha;
    m_curr_tree.asp_beta   = asp_beta;
    m_curr_tree.multipv    = multi_pv;
    m_curr_tree.root_depth = root_depth;

    push_node();
}

void SearchTracer::push_node() {
    NodeInfo new_node;
    new_node.index = m_curr_tree.next_node_index++;
    new_node.parent_index = m_curr_node.index;
    new_node.tree = m_curr_tree.id;

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

void SearchTracer::finish_tree() {
    while (!m_node_stack.empty()) {
        pop_node();
    }

    save_tree(m_curr_tree);
    std::cout << "info string Saved ID/MultiPV/AspWin search tree with id " << i64(m_curr_tree.id) << std::endl;
    m_curr_tree = {};
}

void SearchTracer::finish_search() {
    flush_nodes();
    save_search(m_curr_search);
    std::cout << "info string Saved search with id " << i64(m_curr_search.id) << std::dec << std::endl;
    m_curr_search = {};
}

void SearchTracer::flush_nodes() {
    // Log flushing to console, but don't spam it when flushing
    // few amounts of nodes.
    if (m_target_node_count >= 16384) {
        std::cout << "info string Saving " << m_node_batch.size() << " nodes to trace db" << std::endl;
    }

    try {
        SQLite::Transaction transaction(m_db);

        // Build SQL insert statement from traceables.
        std::stringstream ss;
        ss << "INSERT INTO NODES (node_index, parent_index, tree";

#define TRACEABLE(name, type) ss << ", " << lower_case(#name);
#include "traceables.def"

        ss << ") VALUES (?, ?, ?";

#define TRACEABLE(name, type) ss << ", ?";
#include "traceables.def"

        ss << ");";

        SQLite::Statement statement(m_db, ss.str());

        for (const NodeInfo& node : m_node_batch) {
            int i = 0;

            // Bind ever-present attributes to statement.
            statement.bind(++i, i64(node.index));
            statement.bind(++i, i64(node.parent_index));
            statement.bind(++i, i64(node.tree));

#define TRACEABLE(name, type) bind_to_statement<type>(statement, ++i, node.traced_values[int(Traceable:: name)]);
#include "traceables.def"

            statement.exec();
            statement.reset();
        }

        transaction.commit();
    } catch (const std::exception& e) {
        std::cerr << "Tracing SQLite3 error when saving nodes: " << e.what() << std::endl;
        throw;
    }
    m_node_batch.clear();
}

static bool column_exists(const SQLite::Database& db,
                          const std::string& table_name,
                          const std::string& column_name) {
    SQLite::Statement query(db, "PRAGMA table_info(" + table_name + ")");
    while (query.executeStep()) {
        if (query.getColumn(1).getString() == column_name) {
            return true;
        }
    }
    return false;
}

void SearchTracer::bootstrap_db() {
    try {
        // Create metadata table.
        if (!m_db.tableExists("meta")) {
            m_db.exec(
                    "CREATE TABLE meta (\n"
                    "    id INTEGER PRIMARY KEY CHECK(id = 1), \n"
                    "    version_id TEXT\n"
                    ");\n"
                    "INSERT INTO meta (version_id) VALUES (\'" + std::string(TRACER_VERSION) + "\');"
                                                                                               "\n");
        } else {
            // Metadata table existed, check for version.
            SQLite::Column col = m_db.execAndGet("SELECT version_id FROM meta");
            std::string version_id = col.getText();
            if (version_id != TRACER_VERSION) {
                throw std::runtime_error("Specified trace DB has incompatible version.");
            }
        }

        // Create searches table.
        m_db.exec("CREATE TABLE IF NOT EXISTS searches (\n"
                  "    id INTEGER PRIMARY KEY, \n"
                  "    time_of_creation TIMESTAMP DEFAULT CURRENT_TIMESTAMP,\n"
                  "    limits_depth INTEGER,\n"
                  "    limits_nodes INTEGER,\n"
                  "    limits_wtime INTEGER,\n"
                  "    limits_btime INTEGER,\n"
                  "    limits_winc INTEGER,\n"
                  "    limits_binc INTEGER,\n"
                  "    limits_movetime INTEGER,\n"
                  "    hash INTEGER,\n"
                  "    root_fen TEXT\n"
                  ");\n");

        // Create trees table.
        m_db.exec("CREATE TABLE IF NOT EXISTS trees (\n"
                  "    id INTEGER,\n"
                  "    search INTEGER,\n"
                  "    root_depth INTEGER,\n"
                  "    asp_alpha INTEGER,\n"
                  "    asp_beta INTEGER,\n"
                  "    multipv INTEGER,\n"
                  "    PRIMARY KEY (id, search),  \n"
                  "    FOREIGN KEY (search) REFERENCES searches(id) ON DELETE CASCADE  \n"
                  ");\n");

        // Create nodes table.
        m_db.exec("CREATE TABLE IF NOT EXISTS nodes (\n"
                  "    node_index INTEGER REQUIRED,\n"
                  "    parent_index INTEGER REQUIRED,\n"
                  "    tree INTEGER REQUIRED,"
                  "    PRIMARY KEY (node_index, tree, parent_index),\n"
                  "    FOREIGN KEY (tree) REFERENCES trees(id) ON DELETE CASCADE,\n"
                  "    FOREIGN KEY (parent_index) REFERENCES nodes(node_index) ON DELETE SET NULL\n"
                  ");");

        // Create node indexes;
        m_db.exec("CREATE INDEX IF NOT EXISTS idx_node_index ON nodes(node_index);");
        m_db.exec("CREATE INDEX IF NOT EXISTS idx_parent ON nodes(parent_index);");

        // Add each necessary node column.
        std::stringstream ss;
#define TRACEABLE(name, type) \
    if (!column_exists(m_db, "nodes", lower_case(#name))) { \
        std::string query = std::string("ALTER TABLE nodes ADD ") + lower_case(#name) + " " + sql_type_map<type>() + ";"; \
        m_db.exec(query);\
    }

#include "traceables.def"

        std::string query = ss.str();
        m_db.exec(query);
    }
    catch (const std::exception& e) {
        std::cerr << "Tracing/SQLite3 error when bootstrapping trace DB: " << e.what() << std::endl;
        throw;
    }
}

void SearchTracer::save_tree(const TreeInfo& tree) {
    try {
        SQLite::Statement statement(m_db, "INSERT INTO trees (id, search, root_depth,"
                                          "asp_alpha, asp_beta, multipv) VALUES"
                                          "(?, ?, ?, ?, ?, ?);");
        int i = 0;
        statement.bind(++i, i64(tree.id));
        statement.bind(++i, i64(tree.search));
        statement.bind(++i, i64(tree.root_depth));
        statement.bind(++i, i64(tree.asp_alpha));
        statement.bind(++i, i64(tree.asp_beta));
        statement.bind(++i, i64(tree.multipv));

        statement.exec();
    }
    catch (const std::exception& e) {
        std::cerr << "Tracing/SQLite3 error when saving tree: " << e.what() << std::endl;
        throw;
    }
}

void SearchTracer::save_search(const SearchInfo& search) {
    try {
        SQLite::Statement statement(m_db, "INSERT INTO searches (id,"
                                          "limits_depth, limits_nodes, limits_wtime,"
                                          "limits_btime, limits_winc, limits_binc,"
                                          "limits_movetime, hash, root_fen) VALUES"
                                          "(?, ?, ?, ?, ?, ?, ?, ?, ?, ?);");
        int i = 0;
        statement.bind(++i, i64(search.id));
        statement.bind(++i, search.limits_depth);
        statement.bind(++i, i64(search.limits_nodes));
        statement.bind(++i, search.limits_wtime);
        statement.bind(++i, search.limits_btime);
        statement.bind(++i, search.limits_winc);
        statement.bind(++i, search.limits_binc);
        statement.bind(++i, search.limits_movetime);
        statement.bind(++i, i64(search.hash));
        statement.bind(++i, search.root_fen);

        statement.exec();
    }
    catch (const std::exception& e) {
        std::cerr << "Tracing/SQLite3 error when saving search: " << e.what() << std::endl;
        throw;
    }
}

SearchTracer::SearchTracer(const std::string& db_path,
                           size_t batch_size_mib)
    : m_db(db_path, SQLite::OPEN_CREATE | SQLite::OPEN_READWRITE),
      m_target_node_count((batch_size_mib * 1024 * 1024) / sizeof(NodeInfo)){
    m_node_batch.reserve(m_target_node_count + 3);
    m_node_stack.reserve(MAX_DEPTH);

    bootstrap_db();
}

} // illumina
