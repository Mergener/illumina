#ifndef ILLUMINA_DEBUG_H
#define ILLUMINA_DEBUG_H

#include <cassert>
#include <stdexcept>
#include <string>
#include <utility>

#ifndef NDEBUG
#define USE_ASSERTS
#endif

#ifndef USE_ASSERTS
#define ILLUMINA_ASSERT(...)
#else
#define ILLUMINA_ASSERT(...) ::illumina::do_assert(__FILE_NAME__, __LINE__, __VA_ARGS__)
#endif

namespace illumina {

struct AssertionFailure : public std::logic_error {
    std::string file_name;
    int line;
    explicit AssertionFailure(const std::string& str,
                              std::string        file_name,
                              int line);
};

inline AssertionFailure::AssertionFailure(const std::string& str,
                                          std::string  file_name,
                                          int line)
    : std::logic_error(str), file_name(std::move(file_name)), line(line) {}

inline constexpr void do_assert(const char* file_name,
                      int line,
                      bool condition) {
    if (!condition) {
        throw AssertionFailure(std::string("Assertion failure in file ") + file_name + ", line " + std::to_string(line),
                               file_name,
                               line);
    }
}

}

#endif // ILLUMINA_DEBUG_H
