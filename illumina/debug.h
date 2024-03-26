#ifndef ILLUMINA_DEBUG_H
#define ILLUMINA_DEBUG_H

#include <cassert>

#define ILLUMINA_ASSERT(cond) assert(cond)

#define DEBUG() std::cout << "File " << __FILE__ << " Line " << __LINE__ << "." << std::endl; std::cin.get()

#endif // ILLUMINA_DEBUG_H
