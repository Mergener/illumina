#ifndef ILLUMINA_TRACING_H
#define ILLUMINA_TRACING_H

#include <functional>

#include "board.h"

namespace illumina {

enum TraceInt {
    TRACE_V_BETA,
    TRACE_V_ALPHA,
    TRACE_V_STATIC_EVAL,
    TRACE_V_SCORE,
    TRACE_V_DEPTH
};

enum TraceFlag {
    TRACE_F_QSEARCH,
    TRACE_F_TESTING_SINGULAR
};

class SearchSettings;

class ISearchTracer {
public:
    virtual void new_search(const Board& root,
                            size_t hash_size_mb,
                            const SearchSettings& settings) = 0;
    virtual void finish_search() = 0;
    virtual void new_tree(int root_depth,
                          int multi_pv,
                          int asp_alpha,
                          int asp_beta) = 0;
    virtual void finish_tree() = 0;
    virtual void push_node(Move move) = 0;
    virtual void push_sibling_node() = 0;
    virtual void set_int_value(TraceInt which, i64 value) = 0;
    virtual void set_flag(TraceFlag which) = 0;
    virtual void set_best_move(Move move) = 0;
    virtual void pop_node(bool discard = false) = 0;
};

} // illumina

#endif // ILLUMINA_TRACING_H
