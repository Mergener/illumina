#ifndef ILLUMINA_STATICLIST_H
#define ILLUMINA_STATICLIST_H

#include <type_traits>
#include <cstdlib>
#include <stdexcept>

#include "debug.h"

namespace illumina {

template <typename T, size_t N, size_t ALIGN = alignof(T)>
class StaticList {
public:
    using Iterator      = T*;
    using ConstIterator = const T*;

    bool   empty() const;
    bool   full() const;
    size_t size() const;
    size_t capacity() const;

    T&       operator[](size_t idx);
    T&       at(size_t idx);
    const T& operator[](size_t idx) const;
    const T& at(size_t idx) const;

    void push_back(const T& elem);
    void pop_back();

    Iterator      begin();
    Iterator      end();
    ConstIterator cbegin() const;
    ConstIterator cend() const;

    StaticList();
    ~StaticList();
    StaticList(const StaticList& other);
    StaticList(StaticList&& other);
    StaticList& operator=(const StaticList& other);

private:
    std::aligned_storage<T, ALIGN> m_elems[N];
    T* m_end;
};

template <typename T, size_t N, size_t ALIGN>
inline bool StaticList<T, N, ALIGN>::empty() const {
    return begin() == end();
}

template <typename T, size_t N, size_t ALIGN>
inline bool StaticList<T, N, ALIGN>::full() const {
    return size() >= N;
}

template <typename T, size_t N, size_t ALIGN>
inline size_t StaticList<T, N, ALIGN>::size() const {
    return end() - begin();
}

template <typename T, size_t N, size_t ALIGN>
inline size_t StaticList<T, N, ALIGN>::capacity() const {
    return N;
}

template <typename T, size_t N, size_t ALIGN>
inline T& StaticList<T, N, ALIGN>::operator[](size_t idx) {
    return m_elems[idx];
}

template <typename T, size_t N, size_t ALIGN>
inline const T& StaticList<T, N, ALIGN>::operator[](size_t idx) const {
    return m_elems[idx];
}

template <typename T, size_t N, size_t ALIGN>
inline T& StaticList<T, N, ALIGN>::at(size_t idx) {
    if (idx >= size()) {
        throw std::out_of_range("Index out of bounds.");
    }
    return (*this)[idx];
}

template <typename T, size_t N, size_t ALIGN>
inline const T& StaticList<T, N, ALIGN>::at(size_t idx) const {
    if (idx >= size()) {
        throw std::out_of_range("Index out of bounds.");
    }
    return (*this)[idx];
}

template <typename T, size_t N, size_t ALIGN>
inline void StaticList<T, N, ALIGN>::push_back(const T& elem) {
    ILLUMINA_ASSERT(!full());
    *m_end++ = elem;
}

template <typename T, size_t N, size_t ALIGN>
inline void StaticList<T, N, ALIGN>::pop_back() {
    ILLUMINA_ASSERT(!empty());
    std::destroy_at(*m_end--);
}

template <typename T, size_t N, size_t ALIGN>
inline StaticList<T, N, ALIGN>::Iterator StaticList<T, N, ALIGN>::begin() {
    return m_elems;
}

template <typename T, size_t N, size_t ALIGN>
inline StaticList<T, N, ALIGN>::Iterator StaticList<T, N, ALIGN>::end() {
    return m_end;
}

template <typename T, size_t N, size_t ALIGN>
inline StaticList<T, N, ALIGN>::ConstIterator StaticList<T, N, ALIGN>::cbegin() const {
    return m_elems;
}

template <typename T, size_t N, size_t ALIGN>
inline StaticList<T, N, ALIGN>::ConstIterator StaticList<T, N, ALIGN>::cend() const {
    return m_end;
}

template <typename T, size_t N, size_t ALIGN>
inline StaticList<T, N, ALIGN>::StaticList()
    : m_end(m_elems) {
}

template <typename T, size_t N, size_t ALIGN>
inline StaticList<T, N, ALIGN>::~StaticList() {
    if constexpr (!std::is_trivially_destructible_v) {
        std::destroy(begin(), end());
    }
}

template <typename T, size_t N, size_t ALIGN>
inline StaticList<T, N, ALIGN>::StaticList(const StaticList& other) {
    m_end = std::copy(other.cbegin(), other.cend(), begin());
}

template <typename T, size_t N, size_t ALIGN>
inline StaticList<T, N, ALIGN>::StaticList(StaticList&& other) {
    m_end = std::move(other.cbegin(), other.cend(), begin());
}

template <typename T, size_t N, size_t ALIGN>
inline StaticList<T, N, ALIGN>& StaticList<T, N, ALIGN>::operator=(const StaticList& other) {
    m_end = std::copy(other.cbegin(), other.cend(), begin());
}

} // illumina

#endif // ILLUMINA_STATICLIST_H