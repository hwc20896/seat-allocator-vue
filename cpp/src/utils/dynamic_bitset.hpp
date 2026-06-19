#pragma once

#include <vector>

class DynamicBitset final {
    public:
        explicit DynamicBitset(uint64_t size);

        void set(uint64_t index, bool value);

        [[nodiscard]]
        bool test(uint64_t index) const;

        void reset();

        [[nodiscard]]
        uint64_t size() const noexcept;

    private:
        uint64_t bitCount_;
        std::vector<uint64_t> data_;

        static constexpr uint64_t BITS_PER_WORD = 64;
};