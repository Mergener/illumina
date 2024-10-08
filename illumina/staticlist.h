#ifndef ILLUMINA_STATICLIST_H
#define ILLUMINA_STATICLIST_H
#include <iostream>

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
    StaticList(StaticList&& other) noexcept;
    StaticList& operator=(const StaticList& other);

protected:
    typename std::aligned_storage<sizeof(T), ALIGN>::type m_elems[N];
    T* m_end;
};

template <typename T, size_t N, size_t ALIGN>
inline bool StaticList<T, N, ALIGN>::empty() const {
    return cbegin() == cend();
}

template <typename T, size_t N, size_t ALIGN>
inline bool StaticList<T, N, ALIGN>::full() const {
    return size() >= N;
}

template <typename T, size_t N, size_t ALIGN>
inline size_t StaticList<T, N, ALIGN>::size() const {
    return cend() - cbegin();
}

template <typename T, size_t N, size_t ALIGN>
inline size_t StaticList<T, N, ALIGN>::capacity() const {
    return N;
}

template <typename T, size_t N, size_t ALIGN>
inline T& StaticList<T, N, ALIGN>::operator[](size_t idx) {
    return *(begin() + idx);
}

template <typename T, size_t N, size_t ALIGN>
inline const T& StaticList<T, N, ALIGN>::operator[](size_t idx) const {
    return *(cbegin() + idx);
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
    std::destroy_at(m_end--);
}

template <typename T, size_t N, size_t ALIGN>
inline typename StaticList<T, N, ALIGN>::Iterator StaticList<T, N, ALIGN>::begin() {
    return reinterpret_cast<T*>(std::launder(&m_elems));
}

template <typename T, size_t N, size_t ALIGN>
inline typename StaticList<T, N, ALIGN>::Iterator StaticList<T, N, ALIGN>::end() {
    return reinterpret_cast<T*>(std::launder(m_end));
}

template <typename T, size_t N, size_t ALIGN>
inline typename StaticList<T, N, ALIGN>::ConstIterator StaticList<T, N, ALIGN>::cbegin() const {
    return reinterpret_cast<const T*>(std::launder(&m_elems));
}

template <typename T, size_t N, size_t ALIGN>
inline typename StaticList<T, N, ALIGN>::ConstIterator StaticList<T, N, ALIGN>::cend() const {
    return reinterpret_cast<const T*>(std::launder(m_end));
}

template <typename T, size_t N, size_t ALIGN>
inline StaticList<T, N, ALIGN>::StaticList()
    : m_end(std::launder(reinterpret_cast<T*>(m_elems))) {
}

template <typename T, size_t N, size_t ALIGN>
inline StaticList<T, N, ALIGN>::~StaticList() {
    if constexpr (!std::is_trivially_destructible_v<T>) {
        std::destroy(begin(), end());
    }
}

template <typename T, size_t N, size_t ALIGN>
inline StaticList<T, N, ALIGN>::StaticList(const StaticList& other) {
    if (&other == this) {
        return;
    }

    m_end = std::copy(other.cbegin(), other.cend(), begin());
}

template <typename T, size_t N, size_t ALIGN>
inline StaticList<T, N, ALIGN>::StaticList(StaticList&& other) noexcept {
    m_end = std::move(other.cbegin(), other.cend(), begin());
}

template <typename T, size_t N, size_t ALIGN>
inline StaticList<T, N, ALIGN>& StaticList<T, N, ALIGN>::operator=(const StaticList& other) {
    if (&other == this) {
        return *this;
    }

    m_end = std::copy(other.cbegin(), other.cend(), begin());
    return *this;
}

} // illumina

#endif // ILLUMINA_STATICLIST_H