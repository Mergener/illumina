#ifndef ILLUMINA_TRACING_H
#define ILLUMINA_TRACING_H

#include <functional>
#include <variant>
#include <type_traits>

#include "board.h"

namespace illumina {

using TracedValue = std::variant<std::monostate, i64, std::string, bool>;
#define TRACEABLE(name, cpp_type) static_assert(  std::is_same_v<cpp_type, i64>   \
                                                | std::is_same_v<cpp_type, std::string>  \
                                                | std::is_same_v<cpp_type, bool>, \
                                                "Unsupported traceable type.");
#include "traceables.def"

enum class Traceable {
#define TRACEABLE(name, cpp_type) name,
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
    virtual void set(Traceable which, TracedValue value) = 0;
    virtual void pop_node(bool discard = false) = 0;
};

} // illumina

#endif // ILLUMINA_TRACING_H
