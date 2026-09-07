#include <gtest/gtest.h>
#include <bitset>

#include "dynamic-bitset.hpp"

TEST(DynamicBitset, SetTestReset) {
    DynamicBitset bs(100);
    EXPECT_FALSE(bs.test(50));
    bs.set(50, true);
    EXPECT_TRUE(bs.test(50));
    bs.reset();
    EXPECT_FALSE(bs.test(50));
    EXPECT_EQ(bs.size(), 100);
}

TEST(DynamicBitset, OutOfRangeThrows) {
    DynamicBitset bs(10);

    EXPECT_THROW(bs.set(10, true), std::out_of_range);
    EXPECT_THROW((void)bs.test(10), std::out_of_range);

    EXPECT_THROW((void)bs.test(63), std::out_of_range);
}

TEST(DynamicBitset, ValidBoundaryWorks) {
    DynamicBitset bs(10);
    bs.set(9, true);
    EXPECT_TRUE(bs.test(9));
    EXPECT_FALSE(bs.test(0));
}

TEST(DynamicBitset, CrossWordOperations) {
    DynamicBitset bs(200);
    bs.set(0, true);
    bs.set(63, true);
    bs.set(64, true);   //  1st bit of 2nd word
    bs.set(127, true);  //  Last bit of 2nd word
    bs.set(199, true);  //  Last bit of last word

    EXPECT_TRUE(bs.test(0));
    EXPECT_TRUE(bs.test(63));
    EXPECT_TRUE(bs.test(64));
    EXPECT_TRUE(bs.test(127));
    EXPECT_TRUE(bs.test(199));
    EXPECT_FALSE(bs.test(65));  //  words do not interfere with one another
    EXPECT_FALSE(bs.test(128));

    bs.reset();
    for (const auto i : {0, 63, 64, 127, 199}) {
        EXPECT_FALSE(bs.test(i));
    }
}

static constexpr DynamicBitset makeBits(
    const DynamicBitset::SizeType size, const std::initializer_list<DynamicBitset::SizeType> indices
) {
    DynamicBitset bs(size);
    for (const auto i : indices) bs.set(i, true);
    return bs;
}

static constexpr bool dynamicBitsetConstexprOps() {
    DynamicBitset a(70);
    a.set(1, true);
    a.set(69, true);

    const auto same = a & a;
    const auto neg = ~a;
    a |= a;     //  自併 → 不變
    a ^= same;  //  自消 → 空

    return same.test(1) && same.test(69) && neg.test(0) && !neg.test(1) && neg.trueCount() == 68 && a.none() &&
           a.size() == 70;
}
static_assert(dynamicBitsetConstexprOps());

TEST(DynamicBitset, BitwiseAndIntersects) {
    const auto a = makeBits(200, {0, 63, 64, 127, 199});
    const auto b = makeBits(200, {0, 64, 128, 199});

    EXPECT_EQ(a & b, makeBits(200, {0, 64, 199}));
    EXPECT_EQ((a & b).trueCount(), 3);
    EXPECT_EQ(a, makeBits(200, {0, 63, 64, 127, 199}));  //  操作數不受影響
    EXPECT_EQ(b, makeBits(200, {0, 64, 128, 199}));
}

TEST(DynamicBitset, BitwiseOrAndXor) {
    const auto a = makeBits(70, {0, 63, 69});
    const auto b = makeBits(70, {63, 64});

    EXPECT_EQ(a | b, makeBits(70, {0, 63, 64, 69}));
    EXPECT_EQ(a ^ b, makeBits(70, {0, 64, 69}));
}

TEST(DynamicBitset, BitwiseNotKeepsPaddingZero) {
    const auto a = makeBits(70, {1, 69});  //  70 = 64 + 6，最後一個字有 58 個 padding 位
    const auto n = ~a;

    EXPECT_TRUE(n.test(0));
    EXPECT_FALSE(n.test(1));
    EXPECT_TRUE(n.test(68));
    EXPECT_FALSE(n.test(69));
    EXPECT_EQ(n.trueCount(), 68);
    EXPECT_EQ(n.falseCount(), 2);         //  padding 洩漏時 falseCount 會變成 60
    EXPECT_EQ(a, makeBits(70, {1, 69}));  //  原對象不受影響

    EXPECT_TRUE((a | n).all());   //  互補 → 全 1
    EXPECT_TRUE((a & n).none());  //  互斥 → 全 0
}

TEST(DynamicBitset, FillAndNotAreInverses) {
    DynamicBitset bs(70);
    bs.fill(true);
    EXPECT_TRUE(bs.all());
    EXPECT_EQ(bs.trueCount(), 70);
    EXPECT_TRUE((~bs).none());  //  全 1 取反 → 全 0，padding 必須被遮罩

    bs.fill(false);
    EXPECT_TRUE(bs.none());
    EXPECT_TRUE((~bs).all());
}

TEST(DynamicBitset, DisjointAndIsZero) {
    const auto a = makeBits(64, {0, 63});
    const auto b = makeBits(64, {31});

    EXPECT_TRUE((a & b).none());
    EXPECT_EQ(a & b, DynamicBitset(64));
    EXPECT_EQ(a ^ b, makeBits(64, {0, 31, 63}));
}

TEST(DynamicBitset, AnyAllNone) {
    constexpr DynamicBitset empty(0);
    EXPECT_FALSE(empty.any());
    EXPECT_TRUE(empty.none());
    EXPECT_TRUE(empty.all());  //  空位集全真（空全稱量詞）

    DynamicBitset bs(70);
    EXPECT_TRUE(bs.none());
    EXPECT_FALSE(bs.any());

    bs.set(0, true);
    EXPECT_TRUE(bs.any());
    EXPECT_FALSE(bs.none());
    EXPECT_FALSE(bs.all());

    bs.fill(true);
    EXPECT_TRUE(bs.all());
    EXPECT_TRUE(bs.any());
    EXPECT_FALSE(bs.none());
}

TEST(DynamicBitset, EqualityComparesContentAndSize) {
    EXPECT_EQ(makeBits(64, {1, 2}), makeBits(64, {1, 2}));
    EXPECT_NE(makeBits(64, {1, 2}), makeBits(64, {1}));
    EXPECT_NE(makeBits(64, {1}), makeBits(65, {1}));  //  尺寸不同 → 不相等
    EXPECT_EQ(DynamicBitset(0), DynamicBitset(0));
    EXPECT_NE(DynamicBitset(0), DynamicBitset(1));
}

TEST(DynamicBitset, SizeMismatchThrows) {
    auto a = makeBits(64, {0});
    const auto b = makeBits(65, {0});
    EXPECT_THROW((void)(a & b), std::invalid_argument);
    EXPECT_THROW((void)(a | b), std::invalid_argument);
    EXPECT_THROW((void)(a ^ b), std::invalid_argument);
    EXPECT_THROW((void)(a &= b), std::invalid_argument);
    EXPECT_THROW((void)(a |= b), std::invalid_argument);
    EXPECT_THROW((void)(a ^= b), std::invalid_argument);
}

TEST(DynamicBitset, CompoundAssignmentsMutateInPlace) {
    auto a = makeBits(64, {0, 1});
    const auto b = makeBits(64, {1, 2});

    DynamicBitset& ref = a &= b;
    EXPECT_EQ(std::addressof(ref), std::addressof(a));
    EXPECT_EQ(a, makeBits(64, {1}));

    a.set(0, true);
    a |= b;
    EXPECT_EQ(a, makeBits(64, {0, 1, 2}));

    a ^= b;
    EXPECT_EQ(a, makeBits(64, {0}));
}

TEST(DynamicBitset, EmptyBitsetOperations) {
    constexpr DynamicBitset e;
    EXPECT_EQ(e & e, DynamicBitset(0));
    EXPECT_EQ(e | e, DynamicBitset(0));
    EXPECT_EQ(e ^ e, DynamicBitset(0));
    EXPECT_EQ(~e, DynamicBitset(0));
    EXPECT_TRUE((~e).none());
    EXPECT_EQ(e.trueCount(), 0);
    EXPECT_EQ(e.falseCount(), 0);
}

TEST(DynamicBitset, MatchesStdBitsetSemantics) {
    constexpr size_t N = 200;
    const auto a = makeBits(N, {0, 63, 64, 127, 199});
    const auto b = makeBits(N, {0, 64, 128, 199});

    std::bitset<N> sa, sb;
    for (int i = 0; i < static_cast<int>(N); ++i) {
        if (a.test(i)) sa.set(i);
        if (b.test(i)) sb.set(i);
    }
    const auto fromStd = [](const std::bitset<N>& s) {
        DynamicBitset bs(N);
        for (int i = 0; i < static_cast<int>(N); ++i) {
            if (s.test(i)) bs.set(i, true);
        }
        return bs;
    };

    EXPECT_EQ(a & b, fromStd(sa & sb));
    EXPECT_EQ(a | b, fromStd(sa | sb));
    EXPECT_EQ(a ^ b, fromStd(sa ^ sb));
    EXPECT_EQ(~a, fromStd(~sa));
}