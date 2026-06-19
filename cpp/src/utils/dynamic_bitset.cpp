#include "dynamic_bitset.hpp"

#include <algorithm>

DynamicBitset::DynamicBitset(const uint64_t size)
    : bitCount_(size),
      data_((size + BITS_PER_WORD - 1) / BITS_PER_WORD, 0) {}

void DynamicBitset::set(const uint64_t index, const bool value) {
    const uint64_t wordIndex = index / BITS_PER_WORD;
    const uint64_t bitIndex = index % BITS_PER_WORD;

    if (value) {
        data_[wordIndex] |= (1ULL << bitIndex);
    } else {
        data_[wordIndex] &= ~(1ULL << bitIndex);
    }
}

bool DynamicBitset::test(const uint64_t index) const {
    const uint64_t wordIndex = index / BITS_PER_WORD;
    const uint64_t bitIndex = index % BITS_PER_WORD;
    return (data_[wordIndex] & (1ULL << bitIndex)) != 0;
}

void DynamicBitset::reset() {
    std::ranges::fill(data_, 0);
}

uint64_t DynamicBitset::size() const noexcept {
    return bitCount_;
}