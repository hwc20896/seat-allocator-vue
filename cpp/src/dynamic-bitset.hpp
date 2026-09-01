#pragma once

#include <vector>
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <bit>
#include <numeric>
#include <functional>
#include <ranges>

class DynamicBitset final {
    public:
        using SizeType = uint64_t;

        constexpr DynamicBitset() : bitCount_(0) {}

        explicit constexpr DynamicBitset(SizeType size);

        constexpr ~DynamicBitset() = default;

        constexpr void set(SizeType index, bool value);

        [[nodiscard]]
        constexpr bool test(SizeType index) const;

        constexpr void reset() noexcept;

        [[nodiscard]]
        constexpr SizeType size() const noexcept;

        constexpr void fill(bool value) noexcept;

        [[nodiscard]]
        constexpr SizeType trueCount() const noexcept;
        [[nodiscard]]
        constexpr SizeType falseCount() const noexcept;

        [[nodiscard]]
        constexpr DynamicBitset operator&(const DynamicBitset& other) const;
        [[nodiscard]]
        constexpr DynamicBitset operator|(const DynamicBitset& other) const;
        [[nodiscard]]
        constexpr DynamicBitset operator^(const DynamicBitset& other) const;
        [[nodiscard]]
        constexpr DynamicBitset operator~() const;

        constexpr DynamicBitset& operator&=(const DynamicBitset& other);
        constexpr DynamicBitset& operator|=(const DynamicBitset& other);
        constexpr DynamicBitset& operator^=(const DynamicBitset& other);

        [[nodiscard]]
        constexpr bool any() const noexcept;
        [[nodiscard]]
        constexpr bool all() const noexcept;
        [[nodiscard]]
        constexpr bool none() const noexcept;

        [[nodiscard]]
        constexpr bool operator==(const DynamicBitset&) const noexcept = default;

    private:
        SizeType bitCount_;
        std::vector<SizeType> data_;

        static constexpr SizeType BITS_PER_WORD = std::numeric_limits<SizeType>::digits;
};

constexpr DynamicBitset::DynamicBitset(const SizeType size)
    : bitCount_(size),
      data_((size + BITS_PER_WORD - 1) / BITS_PER_WORD, 0) {}

constexpr void DynamicBitset::set(const SizeType index, const bool value) {
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

constexpr bool DynamicBitset::test(const SizeType index) const {
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

constexpr void DynamicBitset::reset() noexcept {
    std::ranges::fill(data_, 0);
}

constexpr DynamicBitset::SizeType DynamicBitset::size() const noexcept {
    return bitCount_;
}

constexpr void DynamicBitset::fill(const bool value) noexcept {
    std::ranges::fill(data_, value ? ~static_cast<SizeType>(0) : static_cast<SizeType>(0));
    if (const SizeType usedBits = bitCount_ % BITS_PER_WORD) {
        data_.back() &= (static_cast<SizeType>(1) << usedBits) - 1;
    }
}

constexpr DynamicBitset::SizeType DynamicBitset::trueCount() const noexcept {
    return std::transform_reduce(
        data_.begin(), data_.end(), 0ULL, std::plus<SizeType>(), &std::popcount<SizeType>
    );
}

constexpr DynamicBitset::SizeType DynamicBitset::falseCount() const noexcept {
    return bitCount_ - trueCount();
}

constexpr DynamicBitset DynamicBitset::operator&(const DynamicBitset& other) const {
    if (bitCount_ != other.bitCount_) {
        throw std::invalid_argument("DynamicBitset::operator&: sizes must match. bitCount_: " +
                                    std::to_string(bitCount_) + ", other.bitCount_: " +
                                    std::to_string(other.bitCount_));
    }
    DynamicBitset result = *this;
    return result &= other;
}

constexpr DynamicBitset DynamicBitset::operator|(const DynamicBitset& other) const {
    if (bitCount_ != other.bitCount_) {
        throw std::invalid_argument("DynamicBitset::operator|: sizes must match. bitCount_: " +
                                    std::to_string(bitCount_) + ", other.bitCount_: " +
                                    std::to_string(other.bitCount_));
    }
    DynamicBitset result = *this;
    return result |= other;
}

constexpr DynamicBitset DynamicBitset::operator^(const DynamicBitset& other) const {
    if (bitCount_ != other.bitCount_) {
        throw std::invalid_argument("DynamicBitset::operator^: sizes must match. bitCount_: " +
                                    std::to_string(bitCount_) + ", other.bitCount_: " +
                                    std::to_string(other.bitCount_));
    }
    DynamicBitset result = *this;
    return result ^= other;
}

constexpr DynamicBitset DynamicBitset::operator~() const {
    DynamicBitset result = *this;
    for (size_t i = 0; i < result.data_.size(); ++i) {
        result.data_[i] = ~result.data_[i];
    }

    if (const SizeType unusedBits = bitCount_ % BITS_PER_WORD) {
        const SizeType mask = (static_cast<SizeType>(1) << unusedBits) - 1;
        result.data_.back() &= mask;
    }
    return result;
}

constexpr DynamicBitset& DynamicBitset::operator&=(const DynamicBitset& other) {
    if (bitCount_ != other.bitCount_) {
        throw std::invalid_argument("DynamicBitset::operator&=: sizes must match. bitCount_: " +
                                    std::to_string(bitCount_) + ", other.bitCount_: " +
                                    std::to_string(other.bitCount_));
    }

    std::ranges::transform(data_, other.data_, data_.begin(), std::bit_and{});
    return *this;
}

constexpr DynamicBitset& DynamicBitset::operator|=(const DynamicBitset& other) {
    if (bitCount_ != other.bitCount_) {
        throw std::invalid_argument("DynamicBitset::operator|=: sizes must match. bitCount_: " +
                                    std::to_string(bitCount_) + ", other.bitCount_: " +
                                    std::to_string(other.bitCount_));
    }

    std::ranges::transform(data_, other.data_, data_.begin(), std::bit_or{});
    return *this;
}

constexpr DynamicBitset& DynamicBitset::operator^=(const DynamicBitset& other) {
    if (bitCount_ != other.bitCount_) {
        throw std::invalid_argument("DynamicBitset::operator^=: sizes must match. bitCount_: " +
                                    std::to_string(bitCount_) + ", other.bitCount_: " +
                                    std::to_string(other.bitCount_));
    }

    std::ranges::transform(data_, other.data_, data_.begin(), std::bit_xor{});
    return *this;
}

[[nodiscard]]
constexpr bool DynamicBitset::any() const noexcept {
    return std::ranges::any_of(data_, [](const SizeType word) { return word != 0; });
}

[[nodiscard]]
constexpr bool DynamicBitset::none() const noexcept {
    return std::ranges::all_of(data_, [](const SizeType word) { return word == 0; });
}

[[nodiscard]]
constexpr bool DynamicBitset::all() const noexcept {
    if (bitCount_ == 0) {
        return true;
    }

    const auto leadingWords = std::views::all(data_) | std::views::take(data_.size() - 1);
    if (std::ranges::any_of(leadingWords, [](const SizeType word) { return word != ~static_cast<SizeType>(0); })) {
        return false;
    }

    const SizeType remainder = bitCount_ % BITS_PER_WORD;
    const SizeType expectedLastWord = (remainder == 0)
        ? ~static_cast<SizeType>(0)
        : (static_cast<SizeType>(1) << remainder) - 1;

    return data_.back() == expectedLastWord;
}