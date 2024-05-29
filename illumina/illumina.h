#ifndef ILLUMINA_ILLUMINA_H
#define ILLUMINA_ILLUMINA_H

#include "attacks.h"
#include "board.h"
#include "clock.h"
#include "debug.h"
#include "movegen.h"
#include "parsehelper.h"
#include "perft.h"
#include "search.h"
#include "searchdefs.h"
#include "staticlist.h"
#include "types.h"
#include "utils.h"
#include "zobrist.h"

namespace illumina {

void init();
bool initialized();

} // illumina

#endif // ILLUMINA_ILLUMINA_H
