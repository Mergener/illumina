#ifndef ILLUMINA_WEIGHTED_VECTOR_H
#define ILLUMINA_WEIGHTED_VECTOR_H

#include <cstdlib>
#include <vector>
#include <random>
#include <memory>

namespace illumina {

template <typename T>
class WeightedVector : public std::vector<std::pair<T, ssize_t>> {
public:
    T&       pick_weighted_random();
    const T& pick_weighted_random() const;

private:
    size_t weighted_random_index() const;
    std::unique_ptr<std::mt19937> m_rng = std::make_unique<std::mt19937>();
};

template <typename T>
T& WeightedVector<T>::pick_weighted_random() {
    return ((*this)[weighted_random_index()]).first;
}

template <typename T>
const T& WeightedVector<T>::pick_weighted_random() const {
    return ((*this)[weighted_random_index()]).first;
}

template <typename T>
size_t WeightedVector<T>::weighted_random_index() const {
    ssize_t sum = 0;
    for (const auto& pair: *this) {
        sum += pair.second;
    }

    std::uniform_int_distribution<ssize_t> dist(1, sum);
    ssize_t random = dist(*m_rng);

    auto it = this->begin();
    while (random > 0) {
        random -= it->second;
        it++;
    }
    it--;

    return it - this->begin();
}

} // illumina

#endif // ILLUMINA_WEIGHTED_VECTOR_H
