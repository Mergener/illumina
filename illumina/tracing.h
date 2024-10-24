#ifndef ILLUMINA_TRACING_H
#define ILLUMINA_TRACING_H

#include <functional>

#include "board.h"

namespace illumina {

struct NodeInputInfo {
    const Board* board;
};

enum TraceInt {
    TRACEV_BETA,
    TRACEV_ALPHA,
    TRACEV_STATIC_EVAL,
    TRACEV_SCORE
};

enum TraceFlag {
    TRACEV_PV_SEARCH,
    TRACEV_ZW_SEARCH,
    TRACEV_QSEARCH
};

class ISearchTracer {
public:
    virtual void new_tree(const Board& root,
                          int root_depth,
                          int multi_pv);
    virtual void push_node(const NodeInputInfo& node_info) = 0;
    virtual void set_int_value(TraceInt which, i64 value) = 0;
    virtual void set_flag(TraceFlag which) = 0;
    virtual void set_best_move(Move move) = 0;
    virtual void pop_node() = 0;
};

}

#endif //ILLUMINA_TRACING_H
