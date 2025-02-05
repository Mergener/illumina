#ifndef ILLUMINA_TRACING_H
#define ILLUMINA_TRACING_H

#include <functional>
#include <variant>
#include <type_traits>

#include "board.h"

namespace illumina {

/**
 * Enum for every value that can be traced in the search.
 */
enum class Traceable {
#define TRACEABLE(name, cpp_type) name,
#include "traceables.def"
N
};

/**
 * Type for values that can be traced.
 * At the moment, only i64, double and bools are supported.
 */
using TracedValue = std::variant<std::monostate, i64, bool, Move, double>;
#define TRACEABLE(name, cpp_type) static_assert(  std::is_same_v<cpp_type, i64>         \
                                                | std::is_same_v<cpp_type, bool>        \
                                                | std::is_same_v<cpp_type, Move>        \
                                                | std::is_same_v<cpp_type, double>,     \
                                                "Unsupported traceable type.");
#include "traceables.def"

struct SearchSettings;

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
    virtual void set(Traceable which, TracedValue value) = 0;
    virtual void pop_node(bool discard = false) = 0;

    virtual ~ISearchTracer() = default;
};

} // illumina

#endif // ILLUMINA_TRACING_H
