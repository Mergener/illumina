#ifndef ILLUMINA_BOARDUTILS_H
#define ILLUMINA_BOARDUTILS_H

#include "board.h"

namespace illumina {

bool has_good_see(const Board& board,
                  Square source,
                  Square dest,
                  int threshold = 0);

} // illumina

#endif // ILLUMINA_BOARDUTILS_H
