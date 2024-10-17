#ifndef ILLUMINA_BOARDUTILS_H
#define ILLUMINA_BOARDUTILS_H

#include "board.h"

namespace illumina {

bool has_good_see(const Board& board,
                  Square source,
                  Square dest,
                  int threshold = 0);

/**
 * Returns true if a piece from a source square can be placed at a determined square
 * and cannot be immediately captured by a piece of lesser value (anyhow)
 * or by a piece of same or higher value without having a defender.
 */
bool has_good_see_simple(const Board& board,
                         Square source,
                         Square dest);

bool attacks_vulnerable_pieces(const Board& board,
                               Square source,
                               Square dest);

Bitboard discovered_attacks(const Board& board,
                            Square source,
                            Square dest);

} // illumina

#endif // ILLUMINA_BOARDUTILS_H
