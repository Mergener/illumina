#ifndef ILLUMINA_RINGBUFFER_H
#define ILLUMINA_RINGBUFFER_H

#include <cstddef>
#include <cstdlib>
#include <iterator>
#include <memory>
#include <new>
#include <stdexcept>
#include <type_traits>

#include "debug.h"

namespace illumina {

template <typename Container, typename ElementType>
class RingBufferIterator;

template <typename Container, typename ElementType>
RingBufferIterator<Container, ElementType> operator+(
    RingBufferIterator<Container, ElementType> it, std::ptrdiff_t n);

template <typename Container, typename ElementType>
RingBufferIterator<Container, ElementType> operator+(
    std::ptrdiff_t n, RingBufferIterator<Container, ElementType> it);

template <typename Container, typename ElementType>
RingBufferIterator<Container, ElementType> operator-(
    RingBufferIterator<Container, ElementType> it, std::ptrdiff_t n);

template <typename Container, typename ElementType>
std::ptrdiff_t operator-(const RingBufferIterator<Container, ElementType>& a,
                         const RingBufferIterator<Container, ElementType>& b);

template <typename Container, typename ElementType>
class RingBufferIterator {
public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type        = std::remove_cv_t<ElementType>;
    using difference_type   = std::ptrdiff_t;
    using pointer           = ElementType*;
    using reference         = ElementType&;

    RingBufferIterator();
    RingBufferIterator(Container* buf, size_t index);

    template <typename OtherC, typename OtherE,
              typename = std::enable_if_t<std::is_convertible_v<OtherC*, Container*>>>
    RingBufferIterator(const RingBufferIterator<OtherC, OtherE>& other);

    reference operator*() const;
    pointer operator->() const;

    RingBufferIterator& operator++();
    RingBufferIterator operator++(int);
    RingBufferIterator& operator--();
    RingBufferIterator operator--(int);

    RingBufferIterator& operator+=(difference_type n);
    RingBufferIterator& operator-=(difference_type n);

    friend RingBufferIterator operator+<>(RingBufferIterator it, difference_type n);
    friend RingBufferIterator operator+<>(difference_type n, RingBufferIterator it);
    friend RingBufferIterator operator-<>(RingBufferIterator it, difference_type n);
    friend difference_type operator-<>(const RingBufferIterator& a, const RingBufferIterator& b);

    reference operator[](difference_type n) const;

    bool operator==(const RingBufferIterator& other) const;
    bool operator!=(const RingBufferIterator& other) const;
    bool operator<(const RingBufferIterator& other) const;
    bool operator>(const RingBufferIterator& other) const;
    bool operator<=(const RingBufferIterator& other) const;
    bool operator>=(const RingBufferIterator& other) const;

private:
    template <typename, typename> friend class RingBufferIterator;

    Container* m_buf = nullptr;
    size_t     m_index = 0;
};

template <typename Container, typename ElementType>
inline RingBufferIterator<Container, ElementType>::RingBufferIterator() = default;

template <typename Container, typename ElementType>
inline RingBufferIterator<Container, ElementType>::RingBufferIterator(Container* buf, size_t index)
    : m_buf(buf), m_index(index) {
}

template <typename Container, typename ElementType>
template <typename OtherC, typename OtherE, typename>
inline RingBufferIterator<Container, ElementType>::RingBufferIterator(
    const RingBufferIterator<OtherC, OtherE>& other)
    : m_buf(other.m_buf), m_index(other.m_index) {
}

template <typename Container, typename ElementType>
inline typename RingBufferIterator<Container, ElementType>::reference
RingBufferIterator<Container, ElementType>::operator*() const {
    return (*m_buf)[m_index];
}

template <typename Container, typename ElementType>
inline typename RingBufferIterator<Container, ElementType>::pointer
RingBufferIterator<Container, ElementType>::operator->() const {
    return &((*m_buf)[m_index]);
}

template <typename Container, typename ElementType>
inline RingBufferIterator<Container, ElementType>& RingBufferIterator<Container, ElementType>::operator++() {
    ++m_index;
    return *this;
}

template <typename Container, typename ElementType>
inline RingBufferIterator<Container, ElementType> RingBufferIterator<Container, ElementType>::operator++(int) {
    RingBufferIterator tmp = *this;
    ++(*this);
    return tmp;
}

template <typename Container, typename ElementType>
inline RingBufferIterator<Container, ElementType>& RingBufferIterator<Container, ElementType>::operator--() {
    --m_index;
    return *this;
}

template <typename Container, typename ElementType>
inline RingBufferIterator<Container, ElementType> RingBufferIterator<Container, ElementType>::operator--(int) {
    RingBufferIterator tmp = *this;
    --(*this);
    return tmp;
}

template <typename Container, typename ElementType>
inline RingBufferIterator<Container, ElementType>&
RingBufferIterator<Container, ElementType>::operator+=(difference_type n) {
    m_index += n;
    return *this;
}

template <typename Container, typename ElementType>
inline RingBufferIterator<Container, ElementType>&
RingBufferIterator<Container, ElementType>::operator-=(difference_type n) {
    m_index -= n;
    return *this;
}

template <typename Container, typename ElementType>
inline RingBufferIterator<Container, ElementType> operator+(
    RingBufferIterator<Container, ElementType> it, std::ptrdiff_t n) {
    return it += n;
}

template <typename Container, typename ElementType>
inline RingBufferIterator<Container, ElementType> operator+(
    std::ptrdiff_t n, RingBufferIterator<Container, ElementType> it) {
    return it += n;
}

template <typename Container, typename ElementType>
inline RingBufferIterator<Container, ElementType> operator-(
    RingBufferIterator<Container, ElementType> it, std::ptrdiff_t n) {
    return it -= n;
}

template <typename Container, typename ElementType>
inline std::ptrdiff_t operator-(const RingBufferIterator<Container, ElementType>& a,
                                 const RingBufferIterator<Container, ElementType>& b) {
    return static_cast<std::ptrdiff_t>(a.m_index) - static_cast<std::ptrdiff_t>(b.m_index);
}

template <typename Container, typename ElementType>
inline typename RingBufferIterator<Container, ElementType>::reference
RingBufferIterator<Container, ElementType>::operator[](difference_type n) const {
    return *(*this + n);
}

template <typename Container, typename ElementType>
inline bool RingBufferIterator<Container, ElementType>::operator==(
    const RingBufferIterator& other) const {
    return m_buf == other.m_buf && m_index == other.m_index;
}

template <typename Container, typename ElementType>
inline bool RingBufferIterator<Container, ElementType>::operator!=(
    const RingBufferIterator& other) const {
    return !(*this == other);
}

template <typename Container, typename ElementType>
inline bool RingBufferIterator<Container, ElementType>::operator<(
    const RingBufferIterator& other) const {
    return m_index < other.m_index;
}

template <typename Container, typename ElementType>
inline bool RingBufferIterator<Container, ElementType>::operator>(
    const RingBufferIterator& other) const {
    return other < *this;
}

template <typename Container, typename ElementType>
inline bool RingBufferIterator<Container, ElementType>::operator<=(
    const RingBufferIterator& other) const {
    return !(other < *this);
}

template <typename Container, typename ElementType>
inline bool RingBufferIterator<Container, ElementType>::operator>=(
    const RingBufferIterator& other) const {
    return !(*this < other);
}

template <typename T, size_t N, size_t ALIGN = alignof(T)>
class RingBuffer {
public:
    using Iterator             = RingBufferIterator<RingBuffer, T>;
    using ConstIterator        = RingBufferIterator<const RingBuffer, const T>;
    using ReverseIterator      = std::reverse_iterator<Iterator>;
    using ConstReverseIterator = std::reverse_iterator<ConstIterator>;

    bool   empty() const;
    bool   full() const;
    size_t size() const;
    size_t capacity() const;

    T&       operator[](size_t idx);
    const T& operator[](size_t idx) const;
    T&       at(size_t idx);
    const T& at(size_t idx) const;

    T&       front();
    const T& front() const;
    T&       back();
    const T& back() const;

    void push_back(const T& elem);
    void push_back(T&& elem);
    void pop_front();
    void pop_back();
    void clear();

    Iterator             begin();
    Iterator             end();
    ConstIterator        begin() const;
    ConstIterator        end() const;
    ConstIterator        cbegin() const;
    ConstIterator        cend() const;

    ReverseIterator      rbegin();
    ReverseIterator      rend();
    ConstReverseIterator rbegin() const;
    ConstReverseIterator rend() const;
    ConstReverseIterator crbegin() const;
    ConstReverseIterator crend() const;

    RingBuffer();
    ~RingBuffer();
    RingBuffer(const RingBuffer& other);
    RingBuffer(RingBuffer&& other) noexcept;
    RingBuffer& operator=(const RingBuffer& other);
    RingBuffer& operator=(RingBuffer&& other) noexcept;

protected:
    T*       element_ptr(size_t index);
    const T* element_ptr(size_t index) const;

    alignas(ALIGN) std::byte m_elems[N * sizeof(T)];
    size_t m_head{0};
    size_t m_size{0};
};

template <typename T, size_t N, size_t ALIGN>
inline T* RingBuffer<T, N, ALIGN>::element_ptr(size_t index) {
    return reinterpret_cast<T*>(&m_elems[index * sizeof(T)]);
}

template <typename T, size_t N, size_t ALIGN>
inline const T* RingBuffer<T, N, ALIGN>::element_ptr(size_t index) const {
    return reinterpret_cast<const T*>(&m_elems[index * sizeof(T)]);
}

template <typename T, size_t N, size_t ALIGN>
inline bool RingBuffer<T, N, ALIGN>::empty() const {
    return m_size == 0;
}

template <typename T, size_t N, size_t ALIGN>
inline bool RingBuffer<T, N, ALIGN>::full() const {
    return m_size >= N;
}

template <typename T, size_t N, size_t ALIGN>
inline size_t RingBuffer<T, N, ALIGN>::size() const {
    return m_size;
}

template <typename T, size_t N, size_t ALIGN>
inline size_t RingBuffer<T, N, ALIGN>::capacity() const {
    return N;
}

template <typename T, size_t N, size_t ALIGN>
inline T& RingBuffer<T, N, ALIGN>::operator[](size_t idx) {
    size_t physical_idx = (m_head + idx) % N;
    return *std::launder(element_ptr(physical_idx));
}

template <typename T, size_t N, size_t ALIGN>
inline const T& RingBuffer<T, N, ALIGN>::operator[](size_t idx) const {
    size_t physical_idx = (m_head + idx) % N;
    return *std::launder(element_ptr(physical_idx));
}

template <typename T, size_t N, size_t ALIGN>
inline T& RingBuffer<T, N, ALIGN>::at(size_t idx) {
    if (idx >= m_size) {
        throw std::out_of_range("Index out of bounds.");
    }
    return (*this)[idx];
}

template <typename T, size_t N, size_t ALIGN>
inline const T& RingBuffer<T, N, ALIGN>::at(size_t idx) const {
    if (idx >= m_size) {
        throw std::out_of_range("Index out of bounds.");
    }
    return (*this)[idx];
}

template <typename T, size_t N, size_t ALIGN>
inline T& RingBuffer<T, N, ALIGN>::front() {
    ILLUMINA_ASSERT(!empty());
    return (*this)[0];
}

template <typename T, size_t N, size_t ALIGN>
inline const T& RingBuffer<T, N, ALIGN>::front() const {
    ILLUMINA_ASSERT(!empty());
    return (*this)[0];
}

template <typename T, size_t N, size_t ALIGN>
inline T& RingBuffer<T, N, ALIGN>::back() {
    ILLUMINA_ASSERT(!empty());
    return (*this)[m_size - 1];
}

template <typename T, size_t N, size_t ALIGN>
inline const T& RingBuffer<T, N, ALIGN>::back() const {
    ILLUMINA_ASSERT(!empty());
    return (*this)[m_size - 1];
}

template <typename T, size_t N, size_t ALIGN>
inline void RingBuffer<T, N, ALIGN>::push_back(const T& elem) {
    ILLUMINA_ASSERT(!full());
    size_t tail = (m_head + m_size) % N;
    ::new (static_cast<void*>(element_ptr(tail))) T(elem);
    ++m_size;
}

template <typename T, size_t N, size_t ALIGN>
inline void RingBuffer<T, N, ALIGN>::push_back(T&& elem) {
    ILLUMINA_ASSERT(!full());
    size_t tail = (m_head + m_size) % N;
    ::new (static_cast<void*>(element_ptr(tail))) T(std::move(elem));
    ++m_size;
}

template <typename T, size_t N, size_t ALIGN>
inline void RingBuffer<T, N, ALIGN>::pop_front() {
    ILLUMINA_ASSERT(!empty());
    std::destroy_at(std::launder(element_ptr(m_head)));
    m_head = (m_head + 1) % N;
    --m_size;
}

template <typename T, size_t N, size_t ALIGN>
inline void RingBuffer<T, N, ALIGN>::pop_back() {
    ILLUMINA_ASSERT(!empty());
    size_t tail = (m_head + m_size - 1) % N;
    std::destroy_at(std::launder(element_ptr(tail)));
    --m_size;
}

template <typename T, size_t N, size_t ALIGN>
inline void RingBuffer<T, N, ALIGN>::clear() {
    while (!empty()) {
        pop_front();
    }
}

template <typename T, size_t N, size_t ALIGN>
inline typename RingBuffer<T, N, ALIGN>::Iterator RingBuffer<T, N, ALIGN>::begin() {
    return Iterator(this, 0);
}

template <typename T, size_t N, size_t ALIGN>
inline typename RingBuffer<T, N, ALIGN>::Iterator RingBuffer<T, N, ALIGN>::end() {
    return Iterator(this, m_size);
}

template <typename T, size_t N, size_t ALIGN>
inline typename RingBuffer<T, N, ALIGN>::ConstIterator RingBuffer<T, N, ALIGN>::begin() const {
    return ConstIterator(this, 0);
}

template <typename T, size_t N, size_t ALIGN>
inline typename RingBuffer<T, N, ALIGN>::ConstIterator RingBuffer<T, N, ALIGN>::end() const {
    return ConstIterator(this, m_size);
}

template <typename T, size_t N, size_t ALIGN>
inline typename RingBuffer<T, N, ALIGN>::ConstIterator RingBuffer<T, N, ALIGN>::cbegin() const {
    return ConstIterator(this, 0);
}

template <typename T, size_t N, size_t ALIGN>
inline typename RingBuffer<T, N, ALIGN>::ConstIterator RingBuffer<T, N, ALIGN>::cend() const {
    return ConstIterator(this, m_size);
}

template <typename T, size_t N, size_t ALIGN>
inline typename RingBuffer<T, N, ALIGN>::ReverseIterator RingBuffer<T, N, ALIGN>::rbegin() {
    return ReverseIterator(end());
}

template <typename T, size_t N, size_t ALIGN>
inline typename RingBuffer<T, N, ALIGN>::ReverseIterator RingBuffer<T, N, ALIGN>::rend() {
    return ReverseIterator(begin());
}

template <typename T, size_t N, size_t ALIGN>
inline typename RingBuffer<T, N, ALIGN>::ConstReverseIterator RingBuffer<T, N, ALIGN>::rbegin() const {
    return ConstReverseIterator(end());
}

template <typename T, size_t N, size_t ALIGN>
inline typename RingBuffer<T, N, ALIGN>::ConstReverseIterator RingBuffer<T, N, ALIGN>::rend() const {
    return ConstReverseIterator(begin());
}

template <typename T, size_t N, size_t ALIGN>
inline typename RingBuffer<T, N, ALIGN>::ConstReverseIterator RingBuffer<T, N, ALIGN>::crbegin() const {
    return ConstReverseIterator(cend());
}

template <typename T, size_t N, size_t ALIGN>
inline typename RingBuffer<T, N, ALIGN>::ConstReverseIterator RingBuffer<T, N, ALIGN>::crend() const {
    return ConstReverseIterator(cbegin());
}

template <typename T, size_t N, size_t ALIGN>
inline RingBuffer<T, N, ALIGN>::RingBuffer() = default;

template <typename T, size_t N, size_t ALIGN>
inline RingBuffer<T, N, ALIGN>::~RingBuffer() {
    clear();
}

template <typename T, size_t N, size_t ALIGN>
inline RingBuffer<T, N, ALIGN>::RingBuffer(const RingBuffer& other) {
    for (size_t i = 0; i < other.m_size; ++i) {
        push_back(other[i]);
    }
}

template <typename T, size_t N, size_t ALIGN>
inline RingBuffer<T, N, ALIGN>::RingBuffer(RingBuffer&& other) noexcept {
    for (size_t i = 0; i < other.m_size; ++i) {
        size_t tail = (m_head + m_size) % N;
        size_t other_phys_idx = (other.m_head + i) % N;
        T* src = std::launder(other.element_ptr(other_phys_idx));

        ::new (static_cast<void*>(element_ptr(tail))) T(std::move(*src));
        std::destroy_at(src);
        ++m_size;
    }
    other.m_head = 0;
    other.m_size = 0;
}

template <typename T, size_t N, size_t ALIGN>
inline RingBuffer<T, N, ALIGN>& RingBuffer<T, N, ALIGN>::operator=(const RingBuffer& other) {
    if (this != &other) {
        clear();
        for (size_t i = 0; i < other.m_size; ++i) {
            push_back(other[i]);
        }
    }
    return *this;
}

template <typename T, size_t N, size_t ALIGN>
inline RingBuffer<T, N, ALIGN>& RingBuffer<T, N, ALIGN>::operator=(RingBuffer&& other) noexcept {
    if (this != &other) {
        clear();
        for (size_t i = 0; i < other.m_size; ++i) {
            size_t tail = (m_head + m_size) % N;
            size_t other_phys_idx = (other.m_head + i) % N;
            T* src = std::launder(other.element_ptr(other_phys_idx));

            ::new (static_cast<void*>(element_ptr(tail))) T(std::move(*src));
            std::destroy_at(src);
            ++m_size;
        }
        other.m_head = 0;
        other.m_size = 0;
    }
    return *this;
}

} // illumina

#endif // ILLUMINA_RINGBUFFER_H
