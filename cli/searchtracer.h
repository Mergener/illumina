#ifndef ILLUMINA_SEARCHTRACER_H
#define ILLUMINA_SEARCHTRACER_H

#include <SQLiteCpp/SQLiteCpp.h>

#include "tracing.h"
#include "search.h"

namespace illumina {

/** Size, in MiB, to fill up the nodes batch before flushing them into the trace DB. */
static constexpr size_t DEFAULT_TRACER_BATCH_SIZE = 256;

/**
 * Version used to check whether our current implementation of the search tracer is compatible
 * with an existing traces DB.
 * Uses semantic versioning, so non-breaking changes should increment the minor number, whereas
 * breaking changes to the DB format must increment the major number.
 */
static const char* TRACER_VERSION = "v1.0";

class SearchTracer : public ISearchTracer {
public:
    void new_search(const Board& root,
                    size_t hash_size_mb,
                    const SearchSettings& settings) override;

    void new_tree(int root_depth,
                  int multi_pv,
                  int asp_alpha,
                  int asp_beta) override;

    void finish_tree() override;
    void finish_search() override;

    void push_node() override;
    void push_sibling_node() override;
    void pop_node(bool discard = false) override;

    void set(Traceable which, TracedValue value) override;

    explicit SearchTracer(const std::string& db_path,
                          size_t batch_size_mib = DEFAULT_TRACER_BATCH_SIZE);

private:
    SQLite::Database m_db;
    size_t m_target_node_count;

    struct NodeInfo {
        ui64 index = 1;
        ui64 parent_index = 0;
        ui64 tree;

        TracedValue traced_values[int(Traceable::N)];

        // Not saved in DB
        i8 next_child_order = 0;
    };
    NodeInfo m_curr_node;
    std::vector<NodeInfo> m_node_batch;
    std::vector<NodeInfo> m_node_stack;

    struct TreeInfo {
        ui64 id;
        ui64 search;
        int  asp_alpha;
        int  asp_beta;
        int  multipv;
        int  root_depth;

        // Not saved in DB
        int next_node_index = 1;
    };
    TreeInfo m_curr_tree;

    struct SearchInfo {
        ui64 id;
        ui64 limits_nodes;
        ui64 hash;
        int  limits_depth;
        int  limits_wtime;
        int  limits_btime;
        int  limits_winc;
        int  limits_binc;
        int  limits_movetime;
        std::string root_fen;
    };
    SearchInfo m_curr_search;

    void bootstrap_db();
    void flush_nodes();
    void save_tree(const TreeInfo& tree);
    void save_search(const SearchInfo& search);
};

} // illumina

#endif // ILLUMINA_SEARCHTRACER_H
