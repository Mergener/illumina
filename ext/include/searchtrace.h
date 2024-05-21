#ifndef SEARCHTRACE_H
#define SEARCHTRACE_H

#include <stack>
#include <string>
#include <stdint.h>
#include <vector>
#include <functional>
#include <iterator>

/**
 * SearchTrace is intended to be a standalone single-header module that allows tracing a chess
 * Minimax search tree in a space and time efficient, most generic way possible.
 *
 * The search tree can be then serialized and explored.
 */

namespace searchtrace {

//
// Declarations
//

enum {
    NO_PIECE,
    PAWN,
    KNIGHT,
    BISHOP,
    ROOK,
    QUEEN
};

/** Piece types, as defined in the enum above. */
using PieceType = int;

/** Chess squares. 0 is A1, 63 is H8. */
using Square = int;

enum ReservedFlags {
    BETA_CUTOFF,
    TT_CUTOFF,
    PV,
    QSEARCH,
    N_RESERVED_FLAGS,
};

static const char* RESERVED_FLAG_NAMES[] = {
    "beta_cutoff",
    "tt_cutoff",
    "pv",
    "qsearch"
};

class ShortMove {
public:
    Square source() const;
    Square destination() const;
    PieceType promotion_piece_type() const;

    ShortMove(Square source, Square destination, PieceType prom_pt = NO_PIECE);
    ShortMove(const ShortMove& other) = default;
    ShortMove(ShortMove&& other) = default;
    ShortMove& operator=(const ShortMove& other) = default;
    ShortMove() = default;

private:
    uint16_t m_data = 0;
};

struct SearchTreeNode {
    uint64_t    key = 0;
    uint64_t    parent_key = 0;
    uint32_t    flags = 0;
    ShortMove last_move;
    uint16_t    alpha = 0;
    uint16_t    beta = 0;
    uint16_t    static_eval = 0;
    uint8_t     depth = 0;

    SearchTreeNode(uint64_t key, uint64_t parent_key, ShortMove last_move);
};

class SearchTree;

class SearchTreeVisitor {
    friend class SearchTree;
    using NodeIterator = std::vector<SearchTreeNode>::const_iterator;

public:
    void clear_visited();

    /**
     * Returns to the last n visited nodes.
     */
    void previous(size_t n_prev = 1);

    void go_to(uint64_t node_key);
    void go_to(ShortMove move);

    struct NodeData {
        uint64_t key;
        uint64_t parent_key;
        int alpha;
        int beta;
        int static_eval;
        int depth;
        std::vector<ShortMove> moves;
        std::vector<std::string> flags;
    };

    void current_node_data(NodeData& data) const;
    NodeData current_node_data() const;

private:
    const SearchTree* m_tree;
    NodeIterator m_curr_node_it;
    std::vector<NodeIterator> m_node_it_stack;

    SearchTreeVisitor(const SearchTree& tree);
    const SearchTreeNode& current_node() const;

    NodeIterator first_child(uint64_t parent_key);
};

class SearchTree {
    friend class SearchTreeVisitor;
public:
    SearchTree(const std::string& root_fen,
               const std::vector<std::string>& flags,
               std::vector<SearchTreeNode>&& nodes);

    SearchTreeVisitor visit() const;

    uint32_t flag_bit(const std::string& flag_name) const;
    std::string flag_name(uint32_t flag_bit) const;

private:
    std::vector<SearchTreeNode> m_nodes;
    std::vector<std::string> m_flag_names;
    std::string m_root_fen;
    std::vector<uint32_t> m_idx;
};

struct TracerArgs {
    /**
     * The number of nodes we shall reserve before searching at
     * a given depth.
     */
    std::function<size_t(int)> reserve_by_depth_policy = [](int depth) {
        return 20 + 30 * (1 << (depth * 4 / 5));
    };
};

class SearchTracer {
public:
    // Global operations.
    void       clear_nodes();
    void       preallocate_nodes(int depth);
    uint32_t   register_flag(const std::string& flag);
    void       set_root(const std::string& fen);
    SearchTree finish_tree();

    // Node operations.
    void push(uint64_t key, ShortMove move);
    void mark(uint32_t flag_id);
    void pop();

    SearchTracer(TracerArgs args = {});

private:
    std::vector<SearchTreeNode> m_tree_nodes;
    std::vector<std::string> m_flag_names;
    std::string m_root_fen;
    std::vector<size_t> m_node_it_stack;
    size_t m_curr_node_it;
    TracerArgs m_args;

    SearchTreeNode& current_node();
};

//
// Implementation
//

// Utils
template <typename TIter, typename TCompar>
TIter bin_search(TIter begin, TIter end, TCompar compar_func) {
    TIter left  = begin;
    TIter right = end;

    while (left < right) {
        TIter mid = left + std::distance(left, right) / 2;

        int comparison = compar_func(*mid);

        if (comparison == 0) {
            return mid;
        }
        else if (comparison < 0) {
            right = mid;
        }
        else {
            left = ++mid;
        }
    }

    return end;
}

// ShortMove
inline Square ShortMove::source() const {
    return (m_data >> 0) & 0x3f;
}

inline Square ShortMove::destination() const {
    return (m_data >> 6) & 0x3f;
}

inline PieceType ShortMove::promotion_piece_type() const {
    return (m_data >> 12) & 0x7;
}

inline ShortMove::ShortMove(Square source,
                                Square destination,
                                PieceType prom_pt)
    : m_data((source & 0x3f) |
             ((destination & 0x3f) << 6) |
             ((prom_pt & 0x7) << 12)) {
}

// SearchTreeNode
inline SearchTreeNode::SearchTreeNode(uint64_t key, uint64_t parent_key, ShortMove last_move)
    : key(key), parent_key(parent_key), last_move(last_move) {
}

// SearchTree
inline SearchTree::SearchTree(const std::string& root_fen,
                              const std::vector<std::string>& flags,
                              std::vector<SearchTreeNode>&& nodes)
                              : m_nodes(std::move(nodes)),
                                m_flag_names(flags),
                                m_root_fen(root_fen) {
    // Make sure all nodes are sorted by parent_key.
    // This allows us to find all the children of a node by performing
    // binary search.
    std::sort(m_nodes.begin(), m_nodes.end(), [](const SearchTreeNode& a, const SearchTreeNode& b) {
        return a.parent_key < b.parent_key;
    });

    // Now, create the IDX array that will serve as an index for searching nodes using their
    // own keys.
    for (size_t i = 0; i < m_nodes.size(); ++i) {
        m_idx.push_back(i);
    }

    std::sort(m_idx.begin(), m_idx.end(), [this](uint32_t a, uint32_t b) {
        return m_nodes[a].key < m_nodes[b].key;
    });
}

inline SearchTreeVisitor SearchTree::visit() const {
    return SearchTreeVisitor(*this);
}

inline std::string SearchTree::flag_name(uint32_t bit) const {
    if (bit < N_RESERVED_FLAGS) {
        return RESERVED_FLAG_NAMES[bit];
    }
    size_t idx = bit - N_RESERVED_FLAGS;
    return idx < m_flag_names.size()
           ? m_flag_names[idx]
           : "unknown_flag";
}

inline uint32_t SearchTree::flag_bit(const std::string& name) const {
    // Search in reserved flag names.
    for (size_t i = 0; i < N_RESERVED_FLAGS; ++i) {
        const std::string& flag_name = RESERVED_FLAG_NAMES[i];
        if (flag_name == name) {
            return i;
        }
    }
    // Search in custom flag names.
    for (size_t i = 0; i < m_flag_names.size(); ++i) {
        const std::string& flag_name = m_flag_names[i];
        if (flag_name == name) {
            return i + N_RESERVED_FLAGS;
        }
    }
    return -1;
}

// SearchTreeVisitor
inline SearchTreeVisitor::SearchTreeVisitor(const SearchTree& tree)
    : m_tree(&tree) {
}

SearchTreeVisitor::NodeIterator SearchTreeVisitor::first_child(uint64_t parent_key) {
    NodeIterator it = bin_search(m_tree->m_nodes.cbegin(), m_tree->m_nodes.cend(),
                      [parent_key](const SearchTreeNode& mid) {
        if (parent_key > mid.parent_key) {
            return 1;
        }
        if (parent_key < mid.parent_key) {
            return -1;
        }
        return 0;
    });

    while (it >= m_tree->m_nodes.begin() &&
           it->parent_key == parent_key) {
        it--;
    }

    return it + 1;
}

void SearchTreeVisitor::current_node_data(NodeData& data) const {
    const SearchTreeNode& node = current_node();
    data.key         = node.key;
    data.parent_key  = node.parent_key;
    data.alpha       = node.alpha;
    data.beta        = node.beta;
    data.static_eval = node.static_eval;
    data.depth       = node.depth;
    data.moves.clear();
    data.flags.clear();

    for (size_t i = 0; i < sizeof(node.flags); ++i) {
        data.flags.push_back(m_tree->flag_name(i));
    }


}

const SearchTreeNode& SearchTreeVisitor::current_node() const {
    return m_tree->m_nodes[m_curr_node_it];
}

SearchTreeVisitor::NodeData SearchTreeVisitor::current_node_data() const {
    NodeData data;
    current_node_data(data);
    return data;
}

// SearchTracer
inline void SearchTracer::clear_nodes() {
    m_tree_nodes.clear();
    m_tree_nodes.shrink_to_fit();
    m_node_it_stack.clear();
    m_curr_node_it = 0;
}

inline void SearchTracer::preallocate_nodes(int depth) {
    m_tree_nodes.reserve(m_args.reserve_by_depth_policy(depth));
}

inline uint32_t SearchTracer::register_flag(const std::string& flag) {
    uint32_t flag_id = static_cast<uint32_t>(m_flag_names.size()) + N_RESERVED_FLAGS;
    m_flag_names.push_back(flag);
    return flag_id;
}

inline void SearchTracer::set_root(const std::string& fen) {
    m_root_fen = fen;
}

inline SearchTree SearchTracer::finish_tree() {
    SearchTree tree(m_root_fen, m_flag_names, std::move(m_tree_nodes));
    clear_nodes();
    return tree;
}

inline void SearchTracer::push(uint64_t key, ShortMove move) {
    m_node_it_stack.push_back(m_curr_node_it);
    SearchTreeNode& parent = current_node();
    m_curr_node_it = m_tree_nodes.size();
    m_tree_nodes.emplace_back(key, parent.key, move);
}

inline void SearchTracer::mark(uint32_t flag_id) {
    current_node().flags |= (1 << flag_id);
}

inline void SearchTracer::pop() {
    m_curr_node_it = m_node_it_stack.back();
    m_node_it_stack.pop_back();
}

inline SearchTreeNode& SearchTracer::current_node() {
    return m_tree_nodes[m_curr_node_it];
}

inline SearchTracer::SearchTracer(TracerArgs args)
    : m_args(args) {
}

} // searchtrace

#endif // SEARCHTRACE_H