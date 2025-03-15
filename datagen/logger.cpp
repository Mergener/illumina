#include "logger.h"

#include <mutex>

namespace illumina {

std::ostream& operator<<(std::ostream& stream, IOSync token) {
    static std::mutex io_mutex;

    if (token == IOSync::LOCK) {
        io_mutex.lock();
    }
    else {
        io_mutex.unlock();
    }

    return stream;
}

std::ostream& sync_cout() {
    return std::cout << IOSync::LOCK;
}

std::ostream& sync_cout(const ThreadContext& thread_context) {
    if (thread_context.thread_index == 0) {
        return std::cout << IOSync::LOCK << "[Main Thread]: ";
    }
    else {
        return std::cout << IOSync::LOCK << "[Helper #" << thread_context.thread_index << "]: ";
    }
}

} // illumina