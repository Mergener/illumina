#ifndef ILLUMINA_TRACING_H
#define ILLUMINA_TRACING_H

#include <functional>

#include "board.h"

namespace illumina {

enum class TraceableInt {
#define TRACEABLE_INT(name, default) name,
#include "traceables.def"
#undef TRACEABLE_INT
N
};

enum class TraceableBool {
#define TRACEABLE_BOOL(name, default) name,
#include "traceables.def"
N
};

enum class TraceableMove {
#define TRACEABLE_MOVE(name) name,
#include "traceables.def"
N
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
    virtual void push_node() = 0;
    virtual void push_sibling_node() = 0;
    virtual void set(TraceableInt which, i64 value) = 0;
    virtual void set(TraceableBool which, bool value) = 0;
    virtual void set(TraceableMove which, Move value) = 0;
    virtual void pop_node(bool discard = false) = 0;
};

} // illumina

#endif // ILLUMINA_TRACING_H
