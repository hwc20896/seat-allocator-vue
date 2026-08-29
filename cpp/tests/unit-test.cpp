#include <gtest/gtest.h>

#include "grid.hpp"
#include "dynamic-bitset.hpp"
#include "shuffler.hpp"

#include "configs.hpp"
#include "feasibility.hpp"

//   -------------------------------------------------------
//   Grid CSV
//   -------------------------------------------------------

TEST(GridCSV, BasicRoundTrip) {
    const auto g = Grid::fromCSVString("A,B\nC,D\n");
    EXPECT_EQ(g.rowCount(), 2);
    EXPECT_EQ(g.colCount(), 2);
    EXPECT_EQ(g.toCSVString(), "A,B\nC,D\n");
}

TEST(GridCSV, QuotedFieldWithCommaAndQuote) {
    const auto g = Grid::fromCSVString("A,\"B,C\"\n\"He said \"\"hi\"\"\",D\n");
    EXPECT_EQ((g[0, 1]), "B,C");
    EXPECT_EQ((g[1, 0]), "He said \"hi\"");
    EXPECT_EQ(g.toCSVString(), "A,\"B,C\"\n\"He said \"\"hi\"\"\",D\n");
}

TEST(GridCSV, InconsistentColumnsThrows) {
    EXPECT_THROW(Grid::fromCSVString("A,B\nC\n"), std::invalid_argument);
}

TEST(GridCSV, EmptyStringGivesEmptyGrid) {
    EXPECT_TRUE(Grid::fromCSVString("").empty());
}

TEST(GridCSV, TrailingComma) {
    const auto g = Grid::fromCSVString("A,B,\n");
    EXPECT_EQ(g.rowCount(), 1);
    EXPECT_EQ((g[0, 2]), "");
}

TEST(GridCSV, CRLFHandling) {
    const auto g = Grid::fromCSVString("A,B\r\nC,D\r\n");
    EXPECT_EQ(g.rowCount(), 2);
    EXPECT_EQ((g[1, 1]), "D");
}

//   -------------------------------------------------------
//   Grid Accessibility
//   -------------------------------------------------------

TEST(GridAccess, SetGetAndOperators) {
    Grid g(2, 3);
    g.set(1, 2, "X");
    EXPECT_EQ(g.get(1, 2), "X");
    EXPECT_EQ((g[1, 2]), "X");
    EXPECT_EQ(g[5], "X");            // 1*3+2 = 5
    g[0] = "Y";
    EXPECT_EQ((g[0, 0]), "Y");
}

TEST(GridAccess, OutOfRangeThrows) {
    Grid g(2, 2);
    EXPECT_THROW((g[2, 0]), std::out_of_range);
    EXPECT_THROW((g[0, -1]), std::out_of_range);
    EXPECT_THROW(g[4], std::out_of_range);
    EXPECT_THROW((void) g.get(1, 2), std::out_of_range);
}

TEST(GridAccess, CloneIsIndependent) {
    Grid g = Grid::fromCSVString("A,B\nC,D\n");
    auto c = g.clone();
    c.set(0, 0, "Z");
    EXPECT_EQ((g[0, 0]), "A");
    EXPECT_NE(g, c);
}

TEST(GridAccess, NegativeSingleIndexThrows) {
    Grid g(2, 2);
    EXPECT_THROW((void) g[-1], std::out_of_range);
    const Grid& cg = g;
    EXPECT_THROW((void) cg[-1], std::out_of_range);
}

//   -------------------------------------------------------
//   Dynamic Bitsets
//   -------------------------------------------------------

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
    EXPECT_THROW((void) bs.test(10), std::out_of_range);

    EXPECT_THROW((void) bs.test(63), std::out_of_range);
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
    bs.set(64, true);     //  1st bit of 2nd word
    bs.set(127, true);    //  Last bit of 2nd word
    bs.set(199, true);    //  Last bit of last word

    EXPECT_TRUE(bs.test(0));
    EXPECT_TRUE(bs.test(63));
    EXPECT_TRUE(bs.test(64));
    EXPECT_TRUE(bs.test(127));
    EXPECT_TRUE(bs.test(199));
    EXPECT_FALSE(bs.test(65));    //  words do not interfere with one another
    EXPECT_FALSE(bs.test(128));

    bs.reset();
    for (const auto i : {0, 63, 64, 127, 199}) {
        EXPECT_FALSE(bs.test(i));
    }
}

constexpr DynamicBitset makeBits(
    const DynamicBitset::SizeType size,
    const std::initializer_list<DynamicBitset::SizeType> indices
) {
    DynamicBitset bs(size);
    for (const auto i : indices) bs.set(i, true);
    return bs;
}

constexpr bool dynamicBitsetConstexprOps() {
    DynamicBitset a(70);
    a.set(1, true);
    a.set(69, true);

    const auto same = a & a;
    const auto neg = ~a;
    a |= a;              //  自併 → 不變
    a ^= same;           //  自消 → 空

    return same.test(1) && same.test(69)
        && neg.test(0) && !neg.test(1) && neg.trueCount() == 68
        && a.none() && a.size() == 70;
}
static_assert(dynamicBitsetConstexprOps());

TEST(DynamicBitset, BitwiseAndIntersects) {
    const auto a = makeBits(200, {0, 63, 64, 127, 199});
    const auto b = makeBits(200, {0, 64, 128, 199});

    EXPECT_EQ(a & b, makeBits(200, {0, 64, 199}));
    EXPECT_EQ((a & b).trueCount(), 3);
    EXPECT_EQ(a, makeBits(200, {0, 63, 64, 127, 199}));   //  操作數不受影響
    EXPECT_EQ(b, makeBits(200, {0, 64, 128, 199}));
}

TEST(DynamicBitset, BitwiseOrAndXor) {
    const auto a = makeBits(70, {0, 63, 69});
    const auto b = makeBits(70, {63, 64});

    EXPECT_EQ(a | b, makeBits(70, {0, 63, 64, 69}));
    EXPECT_EQ(a ^ b, makeBits(70, {0, 64, 69}));
}

TEST(DynamicBitset, BitwiseNotKeepsPaddingZero) {
    const auto a = makeBits(70, {1, 69});   //  70 = 64 + 6，最後一個字有 58 個 padding 位
    const auto n = ~a;

    EXPECT_TRUE(n.test(0));
    EXPECT_FALSE(n.test(1));
    EXPECT_TRUE(n.test(68));
    EXPECT_FALSE(n.test(69));
    EXPECT_EQ(n.trueCount(), 68);
    EXPECT_EQ(n.falseCount(), 2);    //  padding 洩漏時 falseCount 會變成 60
    EXPECT_EQ(a, makeBits(70, {1, 69}));     //  原對象不受影響

    EXPECT_TRUE((a | n).all());      //  互補 → 全 1
    EXPECT_TRUE((a & n).none());     //  互斥 → 全 0
}

TEST(DynamicBitset, FillAndNotAreInverses) {
    DynamicBitset bs(70);
    bs.fill(true);
    EXPECT_TRUE(bs.all());
    EXPECT_EQ(bs.trueCount(), 70);
    EXPECT_TRUE((~bs).none());       //  全 1 取反 → 全 0，padding 必須被遮罩

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
    const DynamicBitset empty(0);
    EXPECT_FALSE(empty.any());
    EXPECT_TRUE(empty.none());
    EXPECT_TRUE(empty.all());        //  空位集全真（空全稱量詞）

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
    EXPECT_NE(makeBits(64, {1}), makeBits(65, {1}));   //  尺寸不同 → 不相等
    EXPECT_EQ(DynamicBitset(0), DynamicBitset(0));
    EXPECT_NE(DynamicBitset(0), DynamicBitset(1));
}

TEST(DynamicBitset, SizeMismatchThrows) {
    auto a = makeBits(64, {0});
    const auto b = makeBits(65, {0});
    EXPECT_THROW((void) (a & b), std::invalid_argument);
    EXPECT_THROW((void) (a | b), std::invalid_argument);
    EXPECT_THROW((void) (a ^ b), std::invalid_argument);
    EXPECT_THROW((void) (a &= b), std::invalid_argument);
    EXPECT_THROW((void) (a |= b), std::invalid_argument);
    EXPECT_THROW((void) (a ^= b), std::invalid_argument);
}

TEST(DynamicBitset, CompoundAssignmentsMutateInPlace) {
    auto a = makeBits(64, {0, 1});
    const auto b = makeBits(64, {1, 2});

    DynamicBitset& ref = a &= b;
    EXPECT_EQ(&ref, &a);
    EXPECT_EQ(a, makeBits(64, {1}));

    a.set(0, true);
    a |= b;
    EXPECT_EQ(a, makeBits(64, {0, 1, 2}));

    a ^= b;
    EXPECT_EQ(a, makeBits(64, {0}));
}

TEST(DynamicBitset, EmptyBitsetOperations) {
    const DynamicBitset e;
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

//   -------------------------------------------------------
//   Algorithm Base
//   -------------------------------------------------------

constexpr auto cfg = ShuffleConfig{}.setAllowOriginalNeighbors(true);

TEST(GridShuffler, SetGridRejectsEmpty) {
    GridShuffler s(42);
    EXPECT_FALSE(s.setGrid(Grid{}));
}

TEST(GridShuffler, EmptyGridReturnsError) {
    GridShuffler s(42);
    EXPECT_FALSE(s.setGrid(Grid{}));
    const auto result = s.shuffle();
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ShuffleError::EmptyGrid);
}

TEST(GridShuffler, SingleMovableCellIsUnsatisfiable) {
    GridShuffler s(42);
    s.setGrid(Grid::fromCSVString("A,\n"));
    const auto result = s.shuffle();
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ShuffleError::Unsatisfiable);
}

TEST(GridShuffler, SingleCellWithFixedPointAllowedSucceeds) {
    GridShuffler s(42);
    s.setConfig(ShuffleConfig{cfg}.setAllowFixedPoints(true));
    s.setGrid(Grid::fromCSVString("A\n"));
    const auto result = s.shuffle();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(s.getGrid().toCSVString(), "A\n");
}

TEST(GridShuffler, DeterministicWithSameSeed) {
    const auto src = Grid::fromCSVString("A,B\nC,D\n");

    GridShuffler s1(42), s2(42);
    s1.setConfig(cfg);
    s2.setConfig(cfg);
    s1.setGrid(src);
    s2.setGrid(src);
    const auto result1 = s1.shuffle();
    const auto result2 = s2.shuffle();
    ASSERT_TRUE(result1.has_value()) << "Shuffle ended with result: " << static_cast<int>(result1.error());
    ASSERT_TRUE(result2.has_value()) << "Shuffle ended with result: " << static_cast<int>(result2.error());
    EXPECT_EQ(s1.getGrid().toCSVString(), s2.getGrid().toCSVString());
}

TEST(GridShuffler, ReseedProducesIdenticalResult) {
    GridShuffler s(42);
    s.setConfig(cfg);
    s.setGrid(Grid::fromCSVString("A,B\nC,D\n"));
    ASSERT_TRUE(s.shuffle().has_value());
    const auto first = s.getGrid().toCSVString();

    s.setSeed(42);
    s.clearShuffledGrids();
    ASSERT_TRUE(s.shuffle().has_value());
    EXPECT_EQ(s.getGrid().toCSVString(), first);
}

TEST(GridShuffler, ResultIsPermutation) {
    GridShuffler s(42);
    s.setConfig(cfg);
    const auto src = Grid::fromCSVString("A,B\nC,D\n");
    s.setGrid(src);
    ASSERT_TRUE(s.shuffle().has_value());

    auto got = s.getGrid().rawData();
    auto expected = src.rawData();
    std::ranges::sort(got);
    std::ranges::sort(expected);
    EXPECT_EQ(got, expected);
    EXPECT_TRUE(s.validateResult());
}

TEST(GridShuffler, NoFixedPointsByDefault) {
    GridShuffler s(42);
    s.setConfig(cfg);
    const auto src = Grid::fromCSVString("A,B\nC,D\n");
    s.setGrid(src);
    ASSERT_TRUE(s.shuffle().has_value());

    const auto got = s.getGrid();
    for (int i = 0; i < src.size(); ++i) {
        EXPECT_NE(got[i], src[i]) << "cell " << i << " should not be in the same position";
    }
}

TEST(GridShuffler, TinyGridHasExactlyOneSolution) {
    GridShuffler s(42);
    s.setConfig(cfg);
    s.setGrid(Grid::fromCSVString("A,B\n"));
    ASSERT_TRUE(s.shuffle().has_value());
    EXPECT_EQ(s.getGrid().toCSVString(), "B,A\n");
}

TEST(GridShuffler, FrozenEmptyCellsStayPut) {
    GridShuffler s(42);
    s.setGrid(Grid::fromCSVString("A,\n,C\n"));
    ASSERT_TRUE(s.shuffle().has_value());

    const auto got = s.getGrid();
    EXPECT_EQ(got[1], "");
    EXPECT_EQ(got[2], "");
    EXPECT_TRUE(s.validateResult());
}

TEST(GridShuffler, LargerGridShufflesSuccessfully) {
    GridShuffler s(7);
    s.setGrid(Grid::fromCSVString("A,B,C\nD,E,F\nG,H,I\n"));
    ASSERT_TRUE(s.shuffle().has_value());
    EXPECT_TRUE(s.validateResult());
}

TEST(GridShuffler, GridCollectionAndClear) {
    GridShuffler s(42);
    s.setConfig(cfg);
    s.setGrid(Grid::fromCSVString("A,B\nC,D\n"));

    EXPECT_EQ(s.getShuffledGridCount(), 0);
    EXPECT_EQ(s.getGrid().toCSVString(), "A,B\nC,D\n");

    ASSERT_TRUE(s.shuffle().has_value());
    EXPECT_EQ(s.getShuffledGridCount(), 1);
    EXPECT_EQ(s.getAllGrids().size(), 1);

    s.clearShuffledGrids();
    EXPECT_EQ(s.getShuffledGridCount(), 0);
}

TEST(GridShuffler, AllEmptyGridShufflesUnchanged) {
    GridShuffler s(42);
    s.setGrid(Grid::fromCSVString(",\n,\n"));   //  2x2 grid with all empty cells
    const auto result = s.shuffle();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(s.getGrid().toCSVString(), ",\n,\n");
}

TEST(GridShuffler, MultipleShufflesAccumulate) {
    GridShuffler s(42);
    s.setConfig(cfg);
    s.setGrid(Grid::fromCSVString("A,B\nC,D\n"));
    ASSERT_TRUE(s.shuffle().has_value());
    const auto first = s.getGrid().toCSVString();
    ASSERT_TRUE(s.shuffle().has_value());
    const auto second = s.getGrid().toCSVString();

    EXPECT_EQ(s.getShuffledGridCount(), 2);
    EXPECT_EQ(s.getAllGrids()[0].toCSVString(), first);
    EXPECT_EQ(s.getAllGrids()[1].toCSVString(), second);
    EXPECT_EQ(s.getGrid().toCSVString(), second);   //  getGrid returns the most recent
}

TEST(GridShuffler, SwitchGridRebuildsConstraints) {
    GridShuffler s(42);
    s.setConfig(cfg);
    s.setGrid(Grid::fromCSVString("A,B\nC,D\n"));
    ASSERT_TRUE(s.shuffle().has_value());

    const auto src2 = Grid::fromCSVString("X,Y,Z\n");
    s.setGrid(src2);
    ASSERT_TRUE(s.shuffle().has_value());
    EXPECT_EQ(s.getShuffledGridCount(), 1);
    EXPECT_EQ(s.getOriginalGrid().toCSVString(), "X,Y,Z\n");

    auto got = s.getGrid().rawData();
    auto expected = src2.rawData();
    std::ranges::sort(got);
    std::ranges::sort(expected);
    EXPECT_EQ(got, expected);
}

TEST(GridShuffler, GetGridAtIndexOutOfRangeThrows) {
    GridShuffler s(42);
    s.setConfig(cfg);
    s.setGrid(Grid::fromCSVString("A,B\nC,D\n"));
    EXPECT_THROW((void) s.getGrid(0), std::out_of_range);   //  Out-of-bound when not shuffled
    ASSERT_TRUE(s.shuffle().has_value());
    EXPECT_NO_THROW((void) s.getGrid(0));
    EXPECT_THROW((void) s.getGrid(1), std::out_of_range);
}

TEST(GridShuffler, OutOfRangeForceIsIgnored) {
    GridShuffler s(42);
    s.setConfig(ShuffleConfig{cfg}.forceRow("A", 99).forceCol("B", -1));
    s.setGrid(Grid::fromCSVString("A,B\nC,D\n"));
    ASSERT_TRUE(s.shuffle().has_value());
    EXPECT_TRUE(s.validateResult());
}

//   -------------------------------------------------------
//   Algorithm with Constraints
//   -------------------------------------------------------

TEST(GridShuffler, ForceRowAndForceCol) {
    GridShuffler s(42);
    s.setConfig(ShuffleConfig{cfg}.forceRow("A", 1).forceCol("B", 1));
    s.setGrid(Grid::fromCSVString("A,B\nC,D\n"));
    ASSERT_TRUE(s.shuffle().has_value());

    const auto g = s.getGrid();
    for (int r = 0; r < g.rowCount(); ++r) {
        for (int c = 0; c < g.colCount(); ++c) {
            if (g[r, c] == "A") EXPECT_EQ(r, 1);
            if (g[r, c] == "B") EXPECT_EQ(c, 1);
        }
    }
}

TEST(GridShuffler, ForbidRowAndCol) {
    GridShuffler s(42);
    s.setConfig(ShuffleConfig{cfg}.forbidRow("A", 0).forbidCol("B", 0));
    s.setGrid(Grid::fromCSVString("A,B\nC,D\n"));
    ASSERT_TRUE(s.shuffle().has_value());

    const auto g = s.getGrid();
    for (int r = 0; r < g.rowCount(); ++r) {
        for (int c = 0; c < g.colCount(); ++c) {
            if (g[r, c] == "A") EXPECT_NE(r, 0);
            if (g[r, c] == "B") EXPECT_NE(c, 0);
        }
    }
}

TEST(GridShuffler, ForbidShareRow) {
    GridShuffler s(42);
    s.setConfig(ShuffleConfig{cfg}.forbidShareRow("A", "B"));
    s.setGrid(Grid::fromCSVString("A,B\nC,D\n"));
    ASSERT_TRUE(s.shuffle().has_value());

    const auto g = s.getGrid();
    for (int r = 0; r < g.rowCount(); ++r) {
        bool hasA = false, hasB = false;
        for (int c = 0; c < g.colCount(); ++c) {
            hasA |= g[r, c] == "A";
            hasB |= g[r, c] == "B";
        }
        EXPECT_FALSE(hasA && hasB) << "row " << r;
    }
}

TEST(GridShuffler, ForbidShareCol) {
    GridShuffler s(42);
    s.setConfig(ShuffleConfig{cfg}.forbidShareCol("A", "B"));
    s.setGrid(Grid::fromCSVString("A,B\nC,D\n"));
    ASSERT_TRUE(s.shuffle().has_value());

    const auto g = s.getGrid();
    for (int c = 0; c < g.colCount(); ++c) {
        bool hasA = false, hasB = false;
        for (int r = 0; r < g.rowCount(); ++r) {
            hasA |= g[r, c] == "A";
            hasB |= g[r, c] == "B";
        }
        EXPECT_FALSE(hasA && hasB) << "col " << c;
    }
}

TEST(GridShuffler, CustomForbiddenPairNotAdjacent) {
    GridShuffler s(42);
    s.setConfig(ShuffleConfig{cfg}.addForbiddenPair("A", "B"));
    s.setGrid(Grid::fromCSVString("A,B\nC,D\n"));
    ASSERT_TRUE(s.shuffle().has_value());

    const auto g = s.getGrid();
    int aPos = -1, bPos = -1;
    for (int i = 0; i < g.size(); ++i) {
        if (g[i] == "A") aPos = i;
        if (g[i] == "B") bPos = i;
    }
    ASSERT_GE(aPos, 0);
    ASSERT_GE(bPos, 0);
    const int dr = std::abs(aPos / g.colCount() - bPos / g.colCount());
    const int dc = std::abs(aPos % g.colCount() - bPos % g.colCount());
    EXPECT_FALSE(dr + dc == 1);
    EXPECT_TRUE(s.validateResult());
}

TEST(GridShuffler, UnknownConstraintNameIsIgnored) {
    GridShuffler s(42);
    s.setConfig(ShuffleConfig{cfg}.forceRow("NotFound", 0));
    s.setGrid(Grid::fromCSVString("A,B\nC,D\n"));
    ASSERT_TRUE(s.shuffle().has_value());
    EXPECT_TRUE(s.validateResult());
}

TEST(GridShuffler, ImpossibleConstraintsReturnMaxAttempts) {
    GridShuffler s(42);

    s.setConfig(ShuffleConfig{}.forceRow("A", 0).forceRow("B", 0).forceRow("C", 0));
    s.setGrid(Grid::fromCSVString("A,B\nC,D\n"));
    s.setAnnealingConfig(AnnealingConfig{.maxAttempts = 1});
    const auto result = s.shuffle();
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ShuffleError::MaxAttemptsReached);
}

TEST(GridShuffler, TinyGridWithDefaultConfigIsUnsatisfiable) {
    GridShuffler s(42);
    s.setGrid(Grid::fromCSVString("A,B\nC,D\n"));
    s.setAnnealingConfig(AnnealingConfig{.maxAttempts = 1});
    const auto result = s.shuffle();
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ShuffleError::MaxAttemptsReached);
}

TEST(GridShuffler, DiagonalsAreNeighborsMode) {
    GridShuffler s(42);
    s.setConfig(ShuffleConfig{cfg}.setDiagonalsAreNeighbors(true));
    s.setGrid(Grid::fromCSVString("A,B\nC,D\n"));
    ASSERT_TRUE(s.shuffle().has_value());
    EXPECT_TRUE(s.validateResult());
}

TEST(GridShuffler, SelfShareConstraintIsIgnored) {
    GridShuffler s(42);
    s.setConfig(ShuffleConfig{cfg}.forbidShareRow("A", "A").forbidShareCol("B", "B"));
    s.setGrid(Grid::fromCSVString("A,B\nC,D\n"));
    ASSERT_TRUE(s.shuffle().has_value());     //  Self-looping constraints do not affect solvability
    EXPECT_TRUE(s.validateResult());
}

//   -------------------------------------------------------
//   Product Test
//   -------------------------------------------------------

TEST(GridShuffler, ProductScaleDefaultConfigSmoke) {
    GridShuffler s(2026);
    Grid src(8, 5);
    for (int i = 0; i < src.size(); ++i) {
        src.set(i, "S" + std::to_string(i));
    }
    s.setGrid(src);
    const auto result = s.shuffle();
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(s.validateResult());
}

TEST(GridShuffler, ConstraintOverwriteClearsOldConstraints) {
    GridShuffler s(42);
    s.setConfig(ShuffleConfig{}.forceRow("A", 0).forceRow("B", 0));   //  Conflict → No Solution
    s.setGrid(Grid::fromCSVString("A,B\nC,D\n"));
    s.setAnnealingConfig(AnnealingConfig{.maxAttempts = 1});
    EXPECT_FALSE(s.shuffle().has_value());

    s.setConfig(ShuffleConfig{cfg});                                   //  Old constraints must be cleared after overwriting
    ASSERT_TRUE(s.shuffle().has_value());
    EXPECT_TRUE(s.validateResult());
}

TEST(GridShuffler, SetConfigAfterSetGridApplies) {
    GridShuffler s(42);
    s.setGrid(Grid::fromCSVString("A,B\nC,D\n"));
    s.setConfig(ShuffleConfig{cfg}.forceRow("A", 1));   //  Actual Call Sequence of WASM Bridge
    ASSERT_TRUE(s.shuffle().has_value());
    const auto g = s.getGrid();
    for (int r = 0; r < g.rowCount(); ++r)
        for (int c = 0; c < g.colCount(); ++c)
            if (g[r, c] == "A") EXPECT_EQ(r, 1);
}

TEST(GridShuffler, FailedShuffleKeepsOriginalGrid) {
    GridShuffler s(42);
    s.setGrid(Grid::fromCSVString("A,B\nC,D\n"));
    s.setAnnealingConfig(AnnealingConfig{.maxAttempts = 1});
    ASSERT_FALSE(s.shuffle().has_value());
    EXPECT_EQ(s.getGrid().toCSVString(), "A,B\nC,D\n");
    EXPECT_EQ(s.getShuffledGridCount(), 0);
}

TEST(GridShuffler, ResultMetaFieldsAreSane) {
    GridShuffler s(42);
    s.setConfig(cfg);
    s.setGrid(Grid::fromCSVString("A,B\nC,D\n"));
    const auto result = s.shuffle();
    ASSERT_TRUE(result.has_value());
    EXPECT_GE(result->doneAtAttempt, 0);
    EXPECT_GE(result->doneAtStep, 0);
    EXPECT_GE(result->tookMUS, 0);
}

//   -------------------------------------------------------
//   Feasibility Test
//   -------------------------------------------------------

TEST(Feasibility, NoConstraints3x3IsFeasible) {
    const Grid grid = Grid::fromCSVString("A,B,C\nD,E,F\nG,H,I\n");
    const auto [status, layer, reason] = checkFeasibility(grid, ShuffleConfig{});
    EXPECT_EQ(status, FeasibilityStatus::Feasible)
        << "layer=" << layer << " reason=" << reason;
}

TEST(Feasibility, DuplicateNamesAreFeasible) {
    const Grid grid = Grid::fromCSVString("A,A,B\nC,D,E\nF,G,H\n");
    const auto [status, layer, reason] = checkFeasibility(grid, ShuffleConfig{});
    EXPECT_EQ(status, FeasibilityStatus::Feasible)
        << "layer=" << layer << " reason=" << reason;
}

TEST(Feasibility, FrozenEmptyCellsAreFeasible) {
    const Grid grid = Grid::fromCSVString("A,B,C\nD,,F\nG,H,I\n");
    const auto [status, layer, reason] = checkFeasibility(grid, ShuffleConfig{});
    EXPECT_EQ(status, FeasibilityStatus::Feasible)
        << "layer=" << layer << " reason=" << reason;
}

TEST(Feasibility, FourForcedToSameRowExceedsCapacity) {
    const Grid grid = Grid::fromCSVString("A,B,C\nD,E,F\nG,H,I\n");
    const auto currentConfig =
        ShuffleConfig{}.forceRow("A", 0).forceRow("B", 0).forceRow("C", 0).forceRow("D", 0);
    const auto [status, layer, reason] = checkFeasibility(grid, currentConfig);
    EXPECT_EQ(status, FeasibilityStatus::Unsatisfiable)
        << "layer=" << layer << " reason=" << reason;
    EXPECT_EQ(layer, "domain");
}

TEST(Feasibility, ForbidAllRowsLeavesEmptyDomain) {
    const Grid grid = Grid::fromCSVString("A,B,C\nD,E,F\nG,H,I\n");
    const auto currentConfig = ShuffleConfig{}.forbidRow("A", 0).forbidRow("A", 1).forbidRow("A", 2);
    const auto [status, layer, reason] = checkFeasibility(grid, currentConfig);
    EXPECT_EQ(status, FeasibilityStatus::Unsatisfiable)
        << "layer=" << layer << " reason=" << reason;
    EXPECT_EQ(layer, "domain");
}

TEST(Feasibility, ForceAndForbidSameRowConflicts) {
    const Grid grid = Grid::fromCSVString("A,B,C\nD,E,F\nG,H,I\n");
    const auto currentConfig = ShuffleConfig{}.forceRow("A", 1).forbidRow("A", 1);
    const auto [status, layer, reason] = checkFeasibility(grid, currentConfig);
    EXPECT_EQ(status, FeasibilityStatus::Unsatisfiable)
        << "layer=" << layer << " reason=" << reason;
    EXPECT_EQ(layer, "domain");
}

TEST(Feasibility, TwoElementsLockedToSameCellFailsMatching) {
    const Grid grid = Grid::fromCSVString("A,B,C\nD,E,F\nG,H,I\n");
    const auto currentConfig = ShuffleConfig{}.forceRow("A", 0).forceCol("A", 0)
                                                 .forceRow("B", 0).forceCol("B", 0);
    const auto [status, layer, reason] = checkFeasibility(grid, currentConfig);
    EXPECT_EQ(status, FeasibilityStatus::Unsatisfiable)
        << "layer=" << layer << " reason=" << reason;
    EXPECT_EQ(layer, "matching");
}

TEST(Feasibility, ThreeMutuallyExclusiveWithTwoRowsUnsatisfiable) {
    const Grid grid = Grid::fromCSVString("A,B\nC,D\n");
    const auto currentConfig = ShuffleConfig{}
        .forbidShareRow("A", "B").forbidShareRow("B", "C").forbidShareRow("A", "C");
    const auto [status, layer, reason] = checkFeasibility(grid, currentConfig);
    EXPECT_EQ(status, FeasibilityStatus::Unsatisfiable)
        << "layer=" << layer << " reason=" << reason;
    EXPECT_EQ(layer, "coloring");
}

TEST(Feasibility, ExclusivePairForcedToSameRowConflicts) {
    const Grid grid = Grid::fromCSVString("A,B,C\nD,E,F\nG,H,I\n");
    const auto currentConfig = ShuffleConfig{}
        .forceRow("A", 1).forceRow("B", 1).forbidShareRow("A", "B");
    const auto [status, layer, reason] = checkFeasibility(grid, currentConfig);
    EXPECT_EQ(status, FeasibilityStatus::Unsatisfiable)
        << "layer=" << layer << " reason=" << reason;
    EXPECT_EQ(layer, "domain");
}

TEST(Feasibility, ThreeMutuallyExclusiveWithThreeRowsFeasible) {
    const Grid grid = Grid::fromCSVString("A,B,C\nD,E,F\nG,H,I\n");
    const auto currentConfig = ShuffleConfig{}
        .forbidShareRow("A", "B").forbidShareRow("B", "C").forbidShareRow("A", "C");
    const auto [status, layer, reason] = checkFeasibility(grid, currentConfig);
    EXPECT_EQ(status, FeasibilityStatus::Feasible)
        << "layer=" << layer << " reason=" << reason;
}

TEST(Feasibility, ZeroColoringBudgetReturnsUnknown) {
    const Grid grid = Grid::fromCSVString("A,B,C\nD,E,F\nG,H,I\n");
    const auto currentConfig = ShuffleConfig{}.forbidShareRow("A", "B").forbidShareRow("B", "C").forbidShareRow("A", "C");
    constexpr FeasibilityOptions opts{.checkForbidShare = true, .coloringNodeBudget = 0};
    const auto [status, layer, reason] = checkFeasibility(grid, currentConfig, opts);
    EXPECT_EQ(status, FeasibilityStatus::Unknown)
        << "layer=" << layer << " reason=" << reason;
    EXPECT_EQ(layer, "coloring");
}

TEST(Feasibility, ExclusivePairForcedToSameColConflicts) {
    const Grid grid = Grid::fromCSVString("A,B,C\nD,E,F\nG,H,I\n");
    const auto currentConfig = ShuffleConfig{}
        .forceCol("A", 1).forceCol("B", 1).forbidShareCol("A", "B");
    const auto [status, layer, reason] = checkFeasibility(grid, currentConfig);
    EXPECT_EQ(status, FeasibilityStatus::Unsatisfiable)
        << "layer=" << layer << " reason=" << reason;
    EXPECT_EQ(layer, "domain");
}

TEST(Feasibility, EmptyGridIsUnsatisfiable) {
    const Grid grid;
    const auto [status, layer, reason] = checkFeasibility(grid, ShuffleConfig{});
    EXPECT_EQ(status, FeasibilityStatus::Unsatisfiable)
        << "layer=" << layer << " reason=" << reason;
    EXPECT_EQ(layer, "domain");
}

TEST(Feasibility, ColoringDisabledFallsThroughToFeasible) {
    const Grid grid = Grid::fromCSVString("A,B\nC,D\n");
    const auto currentConfig = ShuffleConfig{}
        .forbidShareRow("A", "B").forbidShareRow("B", "C").forbidShareRow("A", "C");
    constexpr FeasibilityOptions opts{.checkForbidShare = false};
    const auto [status, layer, reason] = checkFeasibility(grid, currentConfig, opts);
    EXPECT_EQ(status, FeasibilityStatus::Feasible)
        << "layer=" << layer << " reason=" << reason;
}