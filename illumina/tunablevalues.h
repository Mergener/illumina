#ifndef ILLUMINA_TUNABLEVALUES_H
#define ILLUMINA_TUNABLEVALUES_H

#include <type_traits>

namespace illumina {

template <typename T, typename... TOther>
constexpr bool validate_tunable_value(const char* name, const char* type, T base) {
    return true;
}

template <typename T, typename... TOther>
constexpr bool validate_tunable_value(const char* name, const char* type, T base, T min) {
    return base >= min;
}

template <typename T, typename... TOther>
constexpr bool validate_tunable_value(const char* name, const char* type, T base, T min, T max, TOther... args) {
    return base >= min && base <= max;
}

#define VALIDATE_TUNABLE(name, type, ...) static_assert(validate_tunable_value<type>(#name, #type, __VA_ARGS__), "Invalid bounds for tunable " #name)

#ifdef TUNING_BUILD
#define TUNABLE_VALUE(name, type, value, ...) extern type name; VALIDATE_TUNABLE(name, type, value, __VA_ARGS__)
#else
#define TUNABLE_VALUE(name, type, value, ...) static constexpr type name = value; VALIDATE_TUNABLE(name, type, value, __VA_ARGS__)
#endif

#include "tunablevalues.def"

#undef TUNABLE_VALUE
#undef VALIDATE_TUNABLE

} // illumina

#endif //ILLUMINA_TUNABLEVALUES_H
