#include "utils.h"

#include <random>
#include <mutex>
#include <algorithm>

namespace illumina {

using RandomEngine = std::mt19937_64;
static RandomEngine s_random (std::random_device{}());
static std::mutex s_rand_mutex;

template <typename TInt>
TInt random_integer(TInt min_inclusive, TInt max_exclusive) {
    static_assert(std::is_integral_v<TInt>);

    std::uniform_int_distribution<TInt> gen(min_inclusive, std::max(max_exclusive - 1, min_inclusive));

    return gen(s_random);
}

template <typename TFloat>
TFloat random_fp(TFloat min, TFloat max) {
    static_assert(std::is_floating_point_v<TFloat>);

    std::uniform_real_distribution<TFloat> gen(min, max);

    return gen(s_random);
}

float random(float min, float max) {
    return random_fp(min, max);
}

double random(double min, double max) {
    return random_fp(min, max);
}

i64 random(i64 min_inclusive, i64 max_exclusive) {
    return random_integer(min_inclusive, max_exclusive);
}

ui64 random(ui64 min_inclusive, ui64 max_exclusive) {
    return random_integer(min_inclusive, max_exclusive);
}

i32 random(i32 min_inclusive, i32 max_exclusive) {
    return random_integer(min_inclusive, max_exclusive);
}

ui32 random(ui32 min_inclusive, ui32 max_exclusive) {
    return random_integer(min_inclusive, max_exclusive);
}

Square random_square(Bitboard allowed_squares) {
    int n_allowed = popcount(allowed_squares);
    int rnd = random(0, n_allowed);
    int idx = 0;
    Square ret = SQ_NULL;

    while (allowed_squares) {
        Square s = lsb(allowed_squares);
        if (idx == rnd) {
            ret = s;
            break;
        }
        else {
            allowed_squares = unset_lsb(allowed_squares);
            idx++;
        }
    }

    return ret;
}

std::string lower_case(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return str;
}

std::string upper_case(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(),
                   [](unsigned char c){ return std::toupper(c); });
    return str;
}

} // illumina