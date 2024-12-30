#ifndef ILLUMINA_UTILS_H
#define ILLUMINA_UTILS_H

#include <cctype>
#include <string_view>
#include <stdexcept>

#include "types.h"

namespace illumina {

template <typename T = int, typename T_BASE = T>
bool try_parse_int(std::string_view sv, T& ret, T_BASE base = 10) {
    static_assert(std::is_integral<T>::value);

    if (sv.empty()) {
        // No characters.
        return false;
    }

    ret           = 0;
    T sign_factor = 1;
    size_t i      = 0;

    // We need to check for negative sign.
    if (sv[0] == '-') {
        sign_factor = -1;
        i++;
    }

    while (i < sv.size()) {
        if (std::isspace(sv[i])) {
            break;
        }

        ret *= base;

        char c = sv[i];
        T add;
        if (c >= '0' && c <= '9') {
            add = c - '0';
        }
        else {
            // Add 10 since a is the digit for 10.
            add = std::tolower(c) - 'a' + 10;
        }
        if (add >= base) {
            // Invalid character.
            return false;
        }
        ret += add;

        i++;
    }

    ret *= sign_factor;

    return true;
}

template <typename T = int, typename T_BASE = T>
T parse_int(std::string_view sv, T_BASE base = 10) {
    static_assert(std::is_integral<T>::value);

    T ret;

    if (!try_parse_int(sv, ret, base)) {
        throw std::runtime_error("Invalid integer");
    }

    return ret;
}

i64    random(i64 min_inclusive, i64 max_exclusive);
ui64   random(ui64 min_inclusive, ui64 max_exclusive);
i32    random(i32 min_inclusive, i32 max_exclusive);
ui32   random(ui32 min_inclusive, ui32 max_exclusive);
float  random(float min, float max);
double random(double min, double max);

inline float  random_float_01() { return random(0.0f, 1.0f); }
inline double random_double_01() { return random(0.0, 1.0); }

inline bool random_bool() {
    return random(0, 2);
}

inline Square random_square() {
    return random(0, SQ_COUNT);
}

Square random_square(Bitboard allowed_squares);

inline Color random_color() {
    return random(0, CL_COUNT);
}

std::string lower_case(std::string str);

std::string upper_case(std::string str);


} // illumina

#endif // ILLUMINA_UTILS_H
