#pragma once

#include <vector>
#include <algorithm>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <format>

class DynamicBitset final {
    public:
        using SizeType = uint64_t;

        DynamicBitset() : bitCount_(0) {}

        explicit DynamicBitset(SizeType size);

        ~DynamicBitset() = default;

        void set(SizeType index, bool value);

        [[nodiscard]]
        bool test(SizeType index) const;

        void reset();

        [[nodiscard]]
        SizeType size() const noexcept;

    private:
        SizeType bitCount_;
        std::vector<SizeType> data_;

        static constexpr SizeType BITS_PER_WORD = std::numeric_limits<SizeType>::digits;
};

DynamicBitset::DynamicBitset(const SizeType size)
    : bitCount_(size),
      data_((size + BITS_PER_WORD - 1) / BITS_PER_WORD, 0) {}

void DynamicBitset::set(const SizeType index, const bool value) {
    if (index >= bitCount_) {
        throw std::out_of_range(
            "DynamicBitset::set: Index out of range. Index: " +
            std::to_string(index) +
            ", bitCount_: " +
            std::to_string(bitCount_)
        );
    }

    const SizeType wordIndex = index / BITS_PER_WORD;
    const SizeType bitIndex = index % BITS_PER_WORD;

    if (value) {
        data_[wordIndex] |= (1ULL << bitIndex);
    } else {
        data_[wordIndex] &= ~(1ULL << bitIndex);
    }
}

bool DynamicBitset::test(const SizeType index) const {
    if (index >= bitCount_) {
        throw std::out_of_range(
            "DynamicBitset::test: Index out of range. Index: " +
            std::to_string(index) +
            ", bitCount_: " +
            std::to_string(bitCount_)
        );
    }

    const SizeType wordIndex = index / BITS_PER_WORD;
    const SizeType bitIndex = index % BITS_PER_WORD;

    return (data_[wordIndex] & (1ULL << bitIndex)) != 0;
}

void DynamicBitset::reset() {
    std::ranges::fill(data_, 0);
}

DynamicBitset::SizeType DynamicBitset::size() const noexcept {
    return bitCount_;
}
