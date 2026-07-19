#ifndef ILLUMINA_CLOCK_H
#define ILLUMINA_CLOCK_H

#include <chrono>

#include "types.h"

namespace illumina {

using Clock     = std::chrono::steady_clock;
using TimePoint = std::chrono::time_point<Clock, Clock::duration>;

inline TimePoint now() {
    return Clock::now();
}

inline i64 delta_ms(TimePoint later, TimePoint earlier) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(later - earlier).count();
}

} // illumina

#endif // ILLUMINA_CLOCK_H