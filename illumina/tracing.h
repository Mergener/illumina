#ifndef ILLUMINA_TRACING_H
#define ILLUMINA_TRACING_H

#include <functional>

#include "board.h"

namespace illumina {

struct NodeInputInfo {
    std::reference_wrapper<Board> board;
};

enum TraceInt {
    TRACEV_BETA,
    TRACEV_ALPHA,
    TRACEV_STATIC_EVAL
};

enum TraceBool {
    TRACEV_PV_SEARCH,
    TRACEV_ZW_SEARCH
};

class ISearchTracer {
public:
    virtual void push_node(const NodeInputInfo& node_info) = 0;
    virtual void set_int_value(TraceInt which, i64 value) = 0;
    virtual void set_bool_value(TraceBool which, i64 value) = 0;
    virtual void pop_node() = 0;
};

}

#endif //ILLUMINA_TRACING_H
