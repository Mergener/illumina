#ifndef ILLUMINA_TRANSPOSITIONTABLE_H
#define ILLUMINA_TRANSPOSITIONTABLE_H

#include "searchdefs.h"

namespace illumina {

class TranspositionTable {
public:

    TranspositionTable() = default;
    ~TranspositionTable() = default;
    TranspositionTable(TranspositionTable&& rhs) = default;
    TranspositionTable(const TranspositionTable& rhs) = delete;
    TranspositionTable& operator=(const TranspositionTable& rhs) = delete;

private:

};

} // illumina

#endif // ILLUMINA_TRANSPOSITIONTABLE_H
