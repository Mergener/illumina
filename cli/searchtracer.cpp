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

void SearchTracer::pop_node(bool discard) {
    if (m_node_stack.empty()) {
        return;
    }

    if (!discard) {
        if (m_node_batch.size() == m_target_node_count) {
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
    m_curr_tree = {};
}


void SearchTracer::finish_search() {
    save_search(m_curr_search);
    m_curr_search = {};
}

void SearchTracer::flush_nodes() {
    while (!m_node_batch.empty()) {
        const NodeInfo& node = m_node_batch.back();
        save_node(node);
        m_node_batch.pop_back();
    }
}

SearchTracer::SearchTracer(std::string db_path,
                           size_t batch_size_mb)
    : m_target_node_count(batch_size_mb / sizeof(NodeInfo)),
      m_db(db_path, SQLite::OPEN_CREATE) {
    m_node_batch.reserve(m_target_node_count);
    m_node_stack.reserve(MAX_DEPTH);

    bootstrap_db();
}

} // illumina
