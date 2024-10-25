#ifndef ILLUMINA_SEARCHTRACER_H
#define ILLUMINA_SEARCHTRACER_H

#include <SQLiteCpp/SQLiteCpp.h>

#include "tracing.h"
#include "search.h"

namespace illumina {

class SearchTracer : public ISearchTracer {
public:
    void new_search(const Board& root,
                    size_t hash_size_mb,
                    const SearchSettings& settings) override;

    void finish_search() override;

    void new_tree(int root_depth,
                  int multi_pv,
                  int asp_alpha,
                  int asp_beta) override;

    void finish_tree() override;

    void push_node(Move move) override;

    void set_int_value(TraceInt which, i64 value) override;

    void set_flag(TraceFlag which) override;

    void set_best_move(Move move) override;

    void pop_node(bool discard = false) override;

    explicit SearchTracer(std::string db_path,
                          size_t batch_size_mb = 128 * 1024 * 1024);

private:
    SQLite::Database m_db;
    size_t m_target_node_count;

    struct NodeInfo {
        ui64 index;
        ui64 parent_index;
        ui32 tree;
        Move last_move;
        Move best_move;
        i32  alpha = 0;
        i32  beta = 0;
        i32  score = 0;
        i16  static_eval = 0;
        ui8  depth = 0;
        i8   node_order;
        i8   next_child_order = 0;
        bool qsearch = false;
    };
    NodeInfo m_curr_node;
    std::vector<NodeInfo> m_node_batch;
    std::vector<NodeInfo> m_node_stack;

    struct TreeInfo {
        ui64 id;
        ui64 search;
        int asp_alpha;
        int asp_beta;
        int multipv;
        int root_depth;
        int next_node_index = 0;
    };
    TreeInfo m_curr_tree;

    struct SearchInfo {
        ui64 id;
        ui64 limits_nodes;
        ui64 hash;
        int limits_depth;
        int limits_wtime;
        int limits_btime;
        int limits_winc;
        int limits_binc;
        int limits_movetime;
        std::string root_fen;
    };
    SearchInfo m_curr_search;

    void bootstrap_db();
    void flush_nodes();
    void save_node(const NodeInfo& node);
    void save_tree(const TreeInfo& tree);
    void save_search(const SearchInfo& search);
};

} // illumina

#endif // ILLUMINA_SEARCHTRACER_H
