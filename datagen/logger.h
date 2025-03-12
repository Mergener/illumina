#ifndef ILLUMINA_LOGGER_H
#define ILLUMINA_LOGGER_H

#include <ostream>

#include "datagen_types.h"

namespace illumina {

//
// Idea taken from Stockfish.
// We send a token that locks/unlocks an internal IO mutex.
//

enum class IOSync {
    IO_LOCK,
    IO_UNLOCK
};
std::ostream& operator<<(std::ostream&, IOSync);

std::ostream& sync_cout(const ThreadContext& thread_context);

#define sync_endl  std::endl << illumina::IOSync::IO_UNLOCK
#define sync_flush std::flush << illumina::IOSync::IO_UNLOCK

} // illumina

#endif // ILLUMINA_LOGGER_H
