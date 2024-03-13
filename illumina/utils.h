#ifndef ILLUMINA_UTILS_H
#define ILLUMINA_UTILS_H

#include <cctype>
#include <string_view>
#include <stdexcept>

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

} // illumina

#endif // ILLUMINA_UTILS_H
