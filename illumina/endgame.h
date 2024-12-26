#ifndef ILLUMINA_ENDGAME_H
#define ILLUMINA_ENDGAME_H

#include "types.h"
#include "board.h"
#include "searchdefs.h"

namespace illumina {

enum EndgameType {
    // Winning endgames.
    EG_KQ_K,
    EG_KR_K,
    EG_KBN_K,
    EG_KQ_KR,
    EG_KQ_KB,
    EG_KQ_KN,

    // Drawn endgames.
    EG_KR_KB,
    EG_KR_KN,
    EG_KRN_KR,
    EG_KRB_KR,
    EG_KQ_KNN,
    EG_KQ_KBB,
    EG_KQB_KQ,

    // Unknown endgame.
    EG_UNKNOWN
};

struct Endgame {
    EndgameType type = EG_UNKNOWN;
    Color stronger_player;
    Score evaluation;
};

Endgame identify_endgame(const Board& board);

} // illumina

#endif // ILLUMINA_ENDGAME_H
