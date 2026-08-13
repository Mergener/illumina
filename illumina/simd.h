#ifndef ILLUMINA_SIMD_H
#define ILLUMINA_SIMD_H

#include <cstddef>
#include <immintrin.h>
#include <cstdint>

#include "types.h"

namespace illumina {
class SimdVecI32;

class SimdVecI16 {
public:
    void store_aligned(i16* dst) const;
    i32 hadd() const;

    SimdVecI16& operator+=(const SimdVecI16& rhs);
    SimdVecI16& operator-=(const SimdVecI16& rhs);
    SimdVecI16& operator*=(const SimdVecI16& rhs);
    SimdVecI16 operator+(const SimdVecI16& rhs) const;
    SimdVecI16 operator-(const SimdVecI16& rhs) const;
    SimdVecI16 operator*(const SimdVecI16& rhs) const;
    SimdVecI16 operator-() const;

    static SimdVecI16 zero();
    static SimdVecI16 broadcast(i16 scalar);
    static SimdVecI16 load_aligned(const i16* src);
    static SimdVecI16 min(SimdVecI16 a, SimdVecI16 b);
    static SimdVecI16 max(SimdVecI16 a, SimdVecI16 b);
    static SimdVecI16 clamp(SimdVecI16 v, SimdVecI16 lo, SimdVecI16 hi);
    static SimdVecI32 madd(SimdVecI16 a, SimdVecI16 b);

private:
#ifdef HAS_AVX2
    __m256i m_v;

    explicit SimdVecI16(__m256i v) : m_v(v) {
    }
#else
    i16 m_v;
    explicit SimdVecI16(i16 v) : m_v(v) {
    }
#endif

public:
    static constexpr size_t STRIDE = sizeof(m_v) / sizeof(i16);
};

class SimdVecI32 {
    friend class SimdVecI16;

public:
    i32 hadd() const;

    SimdVecI32& operator+=(const SimdVecI32& rhs);
    SimdVecI32 operator+(const SimdVecI32& rhs) const;

    static SimdVecI32 zero();

private:
#ifdef HAS_AVX2
    __m256i m_v;

    explicit SimdVecI32(__m256i v) : m_v(v) {
    }
#else
    i32 m_v;
    explicit SimdVecI32(i32 v) : m_v(v) {
    }
#endif
};

#ifdef HAS_AVX2

inline SimdVecI16 SimdVecI16::zero() {
    return SimdVecI16(_mm256_setzero_si256());
}

inline SimdVecI16 SimdVecI16::broadcast(i16 scalar) {
    return SimdVecI16(_mm256_set1_epi16(scalar));
}

inline SimdVecI16 SimdVecI16::load_aligned(const i16* src) {
    return SimdVecI16(_mm256_load_si256(reinterpret_cast<const __m256i*>(src)));
}

inline void SimdVecI16::store_aligned(i16* dst) const {
    _mm256_store_si256(reinterpret_cast<__m256i*>(dst), m_v);
}

inline SimdVecI16& SimdVecI16::operator+=(const SimdVecI16& rhs) {
    m_v = _mm256_add_epi16(m_v, rhs.m_v);
    return *this;
}

inline SimdVecI16& SimdVecI16::operator-=(const SimdVecI16& rhs) {
    m_v = _mm256_sub_epi16(m_v, rhs.m_v);
    return *this;
}

inline SimdVecI16& SimdVecI16::operator*=(const SimdVecI16& rhs) {
    m_v = _mm256_mullo_epi16(m_v, rhs.m_v);
    return *this;
}

inline SimdVecI16 SimdVecI16::operator+(const SimdVecI16& rhs) const {
    SimdVecI16 t = *this;
    t += rhs;
    return t;
}

inline SimdVecI16 SimdVecI16::operator-(const SimdVecI16& rhs) const {
    SimdVecI16 t = *this;
    t -= rhs;
    return t;
}

inline SimdVecI16 SimdVecI16::operator*(const SimdVecI16& rhs) const {
    SimdVecI16 t = *this;
    t *= rhs;
    return t;
}

inline SimdVecI16 SimdVecI16::operator-() const { return zero() - *this; }

inline i32 SimdVecI16::hadd() const {
    const __m256i ones = _mm256_set1_epi16(1);
    __m256i s = _mm256_madd_epi16(m_v, ones);
    s = _mm256_hadd_epi32(s, _mm256_setzero_si256());
    s = _mm256_hadd_epi32(s, _mm256_setzero_si256());
    return _mm256_extract_epi32(s, 0) + _mm256_extract_epi32(s, 4);
}

inline SimdVecI32 SimdVecI32::zero() {
    return SimdVecI32(_mm256_setzero_si256());
}

inline SimdVecI32& SimdVecI32::operator+=(const SimdVecI32& rhs) {
    m_v = _mm256_add_epi32(m_v, rhs.m_v);
    return *this;
}

inline SimdVecI32 SimdVecI32::operator+(const SimdVecI32& rhs) const {
    SimdVecI32 t = *this;
    t += rhs;
    return t;
}

inline i32 SimdVecI32::hadd() const {
    __m128i lo = _mm256_castsi256_si128(m_v);
    __m128i hi = _mm256_extracti128_si256(m_v, 1);
    lo = _mm_add_epi32(lo, hi);
    hi = _mm_unpackhi_epi64(lo, lo);
    lo = _mm_add_epi32(lo, hi);
    hi = _mm_shuffle_epi32(lo, _MM_SHUFFLE(2, 3, 0, 1));
    lo = _mm_add_epi32(lo, hi);
    return _mm_cvtsi128_si32(lo);
}

inline SimdVecI16 SimdVecI16::min(SimdVecI16 a, SimdVecI16 b) {
    return SimdVecI16(_mm256_min_epi16(a.m_v, b.m_v));
}

inline SimdVecI16 SimdVecI16::max(SimdVecI16 a, SimdVecI16 b) {
    return SimdVecI16(_mm256_max_epi16(a.m_v, b.m_v));
}

inline SimdVecI16 SimdVecI16::clamp(SimdVecI16 v, SimdVecI16 lo, SimdVecI16 hi) {
    return min(max(v, lo), hi);
}

inline SimdVecI32 SimdVecI16::madd(SimdVecI16 a, SimdVecI16 b) {
    return SimdVecI32(_mm256_madd_epi16(a.m_v, b.m_v));
}

#else

inline SimdVecI16 SimdVecI16::zero() {
    return SimdVecI16(0);
}

inline SimdVecI16 SimdVecI16::broadcast(i16 scalar) {
    return SimdVecI16(scalar);
}

inline SimdVecI16 SimdVecI16::load_aligned(const i16* src) {
    return SimdVecI16(*src);
}

inline void SimdVecI16::store_aligned(i16* dst) const {
    *dst = m_v;
}

inline SimdVecI16& SimdVecI16::operator+=(const SimdVecI16& rhs) {
    m_v = static_cast<i16>(m_v + rhs.m_v);
    return *this;
}

inline SimdVecI16& SimdVecI16::operator-=(const SimdVecI16& rhs) {
    m_v = static_cast<i16>(m_v - rhs.m_v);
    return *this;
}

inline SimdVecI16& SimdVecI16::operator*=(const SimdVecI16& rhs) {
    m_v = static_cast<i16>(m_v * rhs.m_v);
    return *this;
}

inline SimdVecI16 SimdVecI16::operator+(const SimdVecI16& rhs) const {
    SimdVecI16 t = *this;
    t += rhs;
    return t;
}

inline SimdVecI16 SimdVecI16::operator-(const SimdVecI16& rhs) const {
    SimdVecI16 t = *this;
    t -= rhs;
    return t;
}

inline SimdVecI16 SimdVecI16::operator*(const SimdVecI16& rhs) const {
    SimdVecI16 t = *this;
    t *= rhs;
    return t;
}

inline SimdVecI16 SimdVecI16::operator-() const {
    return zero() - *this;
}

inline i32 SimdVecI16::hadd() const {
    return static_cast<i32>(m_v);
}

inline SimdVecI16 SimdVecI16::min(SimdVecI16 a, SimdVecI16 b) {
    return SimdVecI16(a.m_v < b.m_v ? a.m_v : b.m_v);
}

inline SimdVecI16 SimdVecI16::max(SimdVecI16 a, SimdVecI16 b) {
    return SimdVecI16(a.m_v > b.m_v ? a.m_v : b.m_v);
}

inline SimdVecI16 SimdVecI16::clamp(SimdVecI16 v, SimdVecI16 lo, SimdVecI16 hi) {
    return min(max(v, lo), hi);
}

inline SimdVecI32 SimdVecI16::madd(SimdVecI16 a, SimdVecI16 b) {
    return SimdVecI32(i32(a.m_v) * i32(b.m_v));
}

inline SimdVecI32 SimdVecI32::zero() {
    return SimdVecI32(0);
}

inline SimdVecI32& SimdVecI32::operator+=(const SimdVecI32& rhs) {
    m_v += rhs.m_v;
    return *this;
}

inline SimdVecI32 SimdVecI32::operator+(const SimdVecI32& rhs) const {
    SimdVecI32 t = *this;
    t += rhs;
    return t;
}

inline i32 SimdVecI32::hadd() const {
    return m_v;
}

#endif

} // illumina

#endif // ILLUMINA_SIMD_H
