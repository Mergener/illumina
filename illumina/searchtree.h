#ifndef ILLUMINA_SEARCHTREE_H
#define ILLUMINA_SEARCHTREE_H

#include <memory>
#include <iostream>

#include "board.h"
#include "types.h"

namespace illumina {

class CompactMove {
public:
    Square source() const;
    Square destination() const;
    PieceType promotion_piece_type() const;
    Move to_move(const Board& board) const;

    CompactMove(Move move);
    CompactMove(const CompactMove& other) = default;
    CompactMove(CompactMove&& other) = default;
    CompactMove& operator=(const CompactMove& other) = default;
    CompactMove() = default;

private:
    ui16 m_data = 0;
};

struct SearchTreeNode {
    ui64        id = 0;
    ui64        parent_id = 0;
    ui32        flags = 0;
    CompactMove last_move;
    ui16        alpha = 0;
    ui16        beta = 0;
    ui16        static_eval = 0;
    ui8         depth = 0;
};

class SearchTracer {
public:
    void set_root_board(const Board& board);
    void new_tree();


private:
    std::vector<SearchTreeNode> m_curr_tree_nodes;
};

} // illumina

#endif // ILLUMINA_SEARCHTREE_H
