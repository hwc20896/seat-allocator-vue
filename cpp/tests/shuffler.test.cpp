#include "shuffler.hpp"

#include <gtest/gtest.h>

#include "test-helper.hpp"

//   -------------------------------------------------------
//   Algorithm Base
//   -------------------------------------------------------

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
    s.setConfig(strictCfg);
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

TEST(GridShuffler, NoFixedPointsWhenDisallowed) {
    GridShuffler s(42);
    s.setConfig(ShuffleConfig{cfg}.setAllowFixedPoints(false));
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

TEST(GridShuffler, AllEmptyGridReturnsEmptyGridError) {
    //  全空格 grid 沒有可排的內容：視為 EmptyGrid（與無 grid 同級），不產生排位。
    GridShuffler s(42);
    s.setGrid(Grid::fromCSVString(",\n,\n"));  //  2x2 grid with all empty cells
    const auto result = s.shuffle();
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ShuffleError::EmptyGrid);
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
    EXPECT_EQ(s.getGrid().toCSVString(), second);  //  getGrid returns the most recent
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
    EXPECT_THROW((void)s.getGrid(0), std::out_of_range);  //  Out-of-bound when not shuffled
    ASSERT_TRUE(s.shuffle().has_value());
    EXPECT_NO_THROW((void)s.getGrid(0));
    EXPECT_THROW((void)s.getGrid(1), std::out_of_range);
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

TEST(GridShuffler, TinyGridWithStrictConfigIsUnsatisfiable) {
    GridShuffler s(42);
    s.setConfig(strictCfg);
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
    ASSERT_TRUE(s.shuffle().has_value());  //  Self-looping constraints do not affect solvability
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
    s.setConfig(ShuffleConfig{strictCfg}.forceRow("A", 0).forceRow("B", 0));  //  Conflict → No Solution
    s.setGrid(Grid::fromCSVString("A,B\nC,D\n"));
    s.setAnnealingConfig(AnnealingConfig{.maxAttempts = 1});
    EXPECT_FALSE(s.shuffle().has_value());

    s.setConfig(ShuffleConfig{cfg});  //  Old constraints must be cleared after overwriting
    ASSERT_TRUE(s.shuffle().has_value());
    EXPECT_TRUE(s.validateResult());
}

TEST(GridShuffler, SetConfigAfterSetGridApplies) {
    GridShuffler s(42);
    s.setGrid(Grid::fromCSVString("A,B\nC,D\n"));
    s.setConfig(ShuffleConfig{cfg}.forceRow("A", 1));  //  Actual Call Sequence of WASM Bridge
    ASSERT_TRUE(s.shuffle().has_value());
    const auto g = s.getGrid();
    for (int r = 0; r < g.rowCount(); ++r)
        for (int c = 0; c < g.colCount(); ++c)
            if (g[r, c] == "A") EXPECT_EQ(r, 1);
}

TEST(GridShuffler, FailedShuffleKeepsOriginalGrid) {
    GridShuffler s(42);
    s.setConfig(strictCfg);
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

TEST(GridShuffler, DefaultConfigAllowsFixedPointsAndOriginalNeighbors) {
    GridShuffler s(42);
    s.setGrid(Grid::fromCSVString("A,B\nC,D\n"));
    ASSERT_TRUE(s.shuffle().has_value());
    EXPECT_TRUE(s.validateResult());
}

TEST(ShuffleConfig, Issue6DefaultsAndSetters) {
    const ShuffleConfig c;
    EXPECT_TRUE(c.crossAisleAreNeighbors);
    EXPECT_FALSE(c.enableBuddyMatching);
    EXPECT_TRUE(c.doBuddyRotate);  //  換搭檔預設開啟
    EXPECT_TRUE(c.buddyGroups.first.empty());
    EXPECT_TRUE(c.buddyGroups.second.empty());
    EXPECT_EQ(c.prioritizeBuddyPairPosition, PrioritizeBuddyPairPosition::AllAreAcceptable);

    const std::vector<std::string> g1 = {"A1", "A2"};
    const std::vector<std::string> g2 = {"B1", "B2"};
    ShuffleConfig d;
    d.setBuddyGroups(g1, g2);
    EXPECT_EQ(d.buddyGroups.first, g1);
    EXPECT_EQ(d.buddyGroups.second, g2);

    d.addBuddyPair("A3", "B3");  //  逐對追加
    EXPECT_EQ(d.buddyGroups.first, (std::vector<std::string>{"A1", "A2", "A3"}));
    EXPECT_EQ(d.buddyGroups.second, (std::vector<std::string>{"B1", "B2", "B3"}));

    d.setEnableBuddyMatching(true)
        .setCrossAisleAreNeighbors(false)
        .setDoBuddyRotate(false)
        .setPrioritizeBuddyPairPosition(PrioritizeBuddyPairPosition::FrontAndBack);
    EXPECT_TRUE(d.enableBuddyMatching);
    EXPECT_FALSE(d.crossAisleAreNeighbors);
    EXPECT_FALSE(d.doBuddyRotate);
    EXPECT_EQ(d.prioritizeBuddyPairPosition, PrioritizeBuddyPairPosition::FrontAndBack);

    d.setBuddyGroups({"X"}, {"Y"});  //  再次設定應完全替換而非追加
    EXPECT_EQ(d.buddyGroups.first, std::vector<std::string>{"X"});
    EXPECT_EQ(d.buddyGroups.second, std::vector<std::string>{"Y"});
}

TEST(GridShuffler, BuddyMatchingEveryMemberGetsCounterpart) {
    //  2x3 網格：A 組 {A, C} 需各有一個 B 組 ({D, F}) 鄰居，反之亦然；
    //  B、E 為中立元素。隨機排位若忽略 buddy 約束很容易違反。
    const std::vector<std::string> groupA = {"A", "C"};
    const std::vector<std::string> groupB = {"D", "F"};
    GridShuffler s(42);
    s.setConfig(ShuffleConfig{cfg}.setEnableBuddyMatching(true).setBuddyGroups(groupA, groupB));
    s.setGrid(Grid::fromCSVString("A,B,C\nD,E,F\n"));

    const auto result = s.shuffle();
    ASSERT_TRUE(result.has_value()) << "Shuffle ended with result: " << static_cast<int>(result.error());
    EXPECT_TRUE(s.validateResult());

    const auto got = s.getGrid();
    for (const auto& a : groupA) EXPECT_TRUE(hasBuddyNeighbor(got, a, groupB, cfg)) << a << " lacks a B buddy";
    for (const auto& b : groupB) EXPECT_TRUE(hasBuddyNeighbor(got, b, groupA, cfg)) << b << " lacks an A buddy";
}

TEST(GridShuffler, BuddyAcrossAisleDependsOnCrossAisleSetting) {
    //  1x3：A、B 之間隔一個被凍結的空格（走廊）。
    //  視線穿透開啟 → 隔走廊相對也算鄰居，buddy 配對可行；
    //  關閉 → 兩者永遠不相鄰，任何排位都違反 buddy 約束。
    const auto src = Grid::fromCSVString("A,,B\n");

    GridShuffler on(42);
    //  隔走廊的 B 是 A 的舊搭檔：rotate 預設會禁 A-B 重逢 → 此測試聚焦穿透語義，關閉 rotate
    on.setConfig(ShuffleConfig{cfg}.setEnableBuddyMatching(true).setDoBuddyRotate(false).setBuddyGroups({"A"}, {"B"}));
    on.setGrid(src);
    const auto onResult = on.shuffle();
    ASSERT_TRUE(onResult.has_value()) << "Shuffle ended with result: " << static_cast<int>(onResult.error());
    EXPECT_TRUE(on.validateResult());
    EXPECT_TRUE(hasBuddyNeighbor(on.getGrid(), "A", {"B"}, cfg));

    GridShuffler off(42);
    off.setConfig(
        ShuffleConfig{cfg}.setEnableBuddyMatching(true).setBuddyGroups({"A"}, {"B"}).setCrossAisleAreNeighbors(false)
    );
    off.setGrid(src);
    const auto offResult = off.shuffle();
    ASSERT_FALSE(offResult.has_value());
    EXPECT_EQ(offResult.error(), ShuffleError::MaxAttemptsReached);
}

TEST(GridShuffler, CrossAisleOffBreaksForbiddenPairAcrossCorridor) {
    //  3x3 中行全空（走廊）。A 鎖定左上角、X 鎖定左下角（forceRow+forceCol 唯一格）：
    //  視線穿透開啟 → A 與 X 隔走廊相對必為鄰居 → (A, X) 禁配對無解；
    //  關閉 → 走廊隔斷鄰居關係 → 有解。
    const auto src = Grid::fromCSVString("A,B,C\n,,\nX,Y,Z\n");
    const auto corridorCfg = [](const bool crossAisle) {
        return ShuffleConfig{cfg}
            .setCrossAisleAreNeighbors(crossAisle)
            .forceRow("A", 0)
            .forceCol("A", 0)
            .forceRow("X", 2)
            .forceCol("X", 0)
            .addForbiddenPair("A", "X");
    };

    GridShuffler ok(42);
    ok.setConfig(corridorCfg(false));
    ok.setGrid(src);
    const auto okResult = ok.shuffle();
    ASSERT_TRUE(okResult.has_value()) << "Shuffle ended with result: " << static_cast<int>(okResult.error());
    EXPECT_EQ(ok.getGrid()[0], "A");
    EXPECT_EQ(ok.getGrid()[6], "X");
    EXPECT_TRUE(ok.validateResult());

    GridShuffler blocked(42);
    blocked.setConfig(corridorCfg(true));
    blocked.setGrid(src);
    const auto blockedResult = blocked.shuffle();
    ASSERT_FALSE(blockedResult.has_value());
    EXPECT_EQ(blockedResult.error(), ShuffleError::MaxAttemptsReached);
}

TEST(GridShuffler, BuddyGroupsIgnoredWhenMatchingDisabled) {
    //  只設定 buddyGroups 但未開啟 enableBuddyMatching：分組不得造成任何約束。
    //  走廊穿透關閉時 A、B 永不相鄰；若 buddy 約束被誤觸發，此案例必定失敗。
    GridShuffler s(42);
    s.setConfig(ShuffleConfig{cfg}.setCrossAisleAreNeighbors(false).setBuddyGroups({"A"}, {"B"}));
    s.setGrid(Grid::fromCSVString("A,,B\n"));
    const auto result = s.shuffle();
    ASSERT_TRUE(result.has_value()) << "Shuffle ended with result: " << static_cast<int>(result.error());
    EXPECT_TRUE(s.validateResult());
}

TEST(GridShuffler, UnknownBuddyNamesAreIgnored) {
    //  buddy 名單中不存在的名字應靜默忽略（與 constraints 的處理一致）。
    //  佈局中 B 是 A 的舊搭檔：關閉 rotate，讓測試專注於「Ghost 忽略後 buddy 仍生效」。
    const std::vector<std::string> groupA = {"A", "Ghost"};
    const std::vector<std::string> groupB = {"B"};
    GridShuffler s(42);
    s.setConfig(ShuffleConfig{cfg}.setEnableBuddyMatching(true).setDoBuddyRotate(false).setBuddyGroups(groupA, groupB));
    s.setGrid(Grid::fromCSVString("A,B,X\n"));
    const auto result = s.shuffle();
    ASSERT_TRUE(result.has_value()) << "Shuffle ended with result: " << static_cast<int>(result.error());
    EXPECT_TRUE(s.validateResult());
    EXPECT_TRUE(hasBuddyNeighbor(s.getGrid(), "A", {"B"}, cfg));
}

TEST(GridShuffler, BuddyRotationReplacesOldPartners) {
    //  ISSUE #6「換新搭檔」語義：allowOriginalNeighbors=false 禁止所有原相鄰對重逢
    //  （originalNeighborsMatrix_ 為超集，涵蓋舊 A-B 搭檔對）。
    //  原排位（1x6 直線）原相鄰對：A-B、B-X、X-C、C-Y、Y-Z；
    //  A 組 {A, C}、B 組 {B, X}：舊搭檔為 A-B 與 C-X，解須讓 A、C 換搭檔。
    const auto src = Grid::fromCSVString("A,B,X,C,Y,Z\n");
    const std::vector<std::string> groupA = {"A", "C"};
    const std::vector<std::string> groupB = {"B", "X"};
    GridShuffler s(42);
    s.setConfig(
        ShuffleConfig{cfg}.setAllowOriginalNeighbors(false).setEnableBuddyMatching(true).setBuddyGroups(groupA, groupB)
    );
    s.setGrid(src);
    const auto result = s.shuffle();
    ASSERT_TRUE(result.has_value()) << "Shuffle ended with result: " << static_cast<int>(result.error());
    EXPECT_TRUE(s.validateResult());

    const auto got = s.getGrid();
    for (const auto& a : groupA) EXPECT_TRUE(hasBuddyNeighbor(got, a, groupB, cfg)) << a << " lacks a B buddy";
    for (const auto& b : groupB) EXPECT_TRUE(hasBuddyNeighbor(got, b, groupA, cfg)) << b << " lacks an A buddy";
    //  舊搭檔不得重逢
    EXPECT_FALSE(hasBuddyNeighbor(got, "A", {"B"}, cfg));
    EXPECT_FALSE(hasBuddyNeighbor(got, "C", {"X"}, cfg));
}

TEST(GridShuffler, BuddyWithNoOriginalNeighborsFailsWhenOnlyOldPartnerFits) {
    //  A 唯一的 B 組候選就是舊搭檔 B：buddy 要求 A-B 相鄰，
    //  allowOriginalNeighbors=false 卻禁止 A-B 重逢 → 結構性無解。
    //  （若舊搭檔排斥未生效，A、B 相鄰恒可行，此案例必然成功。）
    GridShuffler s(42);
    s.setConfig(
        ShuffleConfig{cfg}.setAllowOriginalNeighbors(false).setEnableBuddyMatching(true).setBuddyGroups({"A"}, {"B"})
    );
    s.setGrid(Grid::fromCSVString("A,B,X\n"));
    const auto result = s.shuffle();
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ShuffleError::MaxAttemptsReached);
}

TEST(GridShuffler, BuddyRotationAloneForcesFreshPartner) {
    //  doBuddyRotate 的獨立鑑別：cfg 允許原鄰居重逢（allowOriginalNeighbors=true）。
    //  A、B 為舊搭檔且 B 是 A 唯一可配的 B 組成員：rotate 生效 → A 永遠無法換搭檔
    //  → 無解；rotate 關閉 → A-B 重逢合法 → 有解。rotate 未實作時第一段必然誤判成功。
    const auto makeCfg = [](const bool rotate) {
        return ShuffleConfig{cfg}.setEnableBuddyMatching(true).setDoBuddyRotate(rotate).setBuddyGroups({"A"}, {"B"});
    };

    GridShuffler rotated(42);
    rotated.setConfig(makeCfg(true));
    rotated.setGrid(Grid::fromCSVString("A,B,X\n"));
    const auto rotatedResult = rotated.shuffle();
    ASSERT_FALSE(rotatedResult.has_value());
    EXPECT_EQ(rotatedResult.error(), ShuffleError::MaxAttemptsReached);

    GridShuffler noRotate(42);
    noRotate.setConfig(makeCfg(false));
    noRotate.setGrid(Grid::fromCSVString("A,B,X\n"));
    const auto noRotateResult = noRotate.shuffle();
    ASSERT_TRUE(noRotateResult.has_value())
        << "Shuffle ended with result: " << static_cast<int>(noRotateResult.error());
    EXPECT_TRUE(noRotate.validateResult());
}

TEST(GridShuffler, BuddyRotationPreciseOnOldPartnersOnly) {
    //  舊測試 BuddyRotationReplacesOldPartners 靠 allowOriginalNeighbors=false 的泛化
    //  禁止達標；此測試在 allowOriginalNeighbors=true（cfg）下驗證 rotate 的精準性：
    //  只拆「A × 舊 B 搭檔」（A-B、C-X），A、C 仍須各配到一個新 B 搭檔。
    //  1x6 原相鄰對：A-B、B-X、X-C、C-Y、Y-Z；A 組 {A, C}、B 組 {B, X}。
    const auto src = Grid::fromCSVString("A,B,X,C,Y,Z\n");
    const std::vector<std::string> groupA = {"A", "C"};
    const std::vector<std::string> groupB = {"B", "X"};
    GridShuffler s(42);
    s.setConfig(ShuffleConfig{cfg}.setEnableBuddyMatching(true).setBuddyGroups(groupA, groupB));
    s.setGrid(src);
    const auto result = s.shuffle();
    ASSERT_TRUE(result.has_value()) << "Shuffle ended with result: " << static_cast<int>(result.error());
    EXPECT_TRUE(s.validateResult());

    const auto got = s.getGrid();
    for (const auto& a : groupA) EXPECT_TRUE(hasBuddyNeighbor(got, a, groupB, cfg)) << a << " lacks a B buddy";
    EXPECT_FALSE(hasBuddyNeighbor(got, "A", {"B"}, cfg));  //  舊搭檔 A-B 已拆
    EXPECT_FALSE(hasBuddyNeighbor(got, "C", {"X"}, cfg));  //  舊搭檔 C-X 已拆
}

TEST(GridShuffler, EmptyBuddyGroupDisablesMatching) {
    //  任一群為空（含名單全不在 grid 中）→ 整組 buddy 約束停用，不得造成無解。
    //  若空群未停用：A 需 B 群鄰居但 B 群空 → 必然 MaxAttemptsReached。
    GridShuffler s(42);
    s.setConfig(ShuffleConfig{cfg}.setEnableBuddyMatching(true).setBuddyGroups({"A"}, {}));  //  B 群空
    s.setGrid(Grid::fromCSVString("A,B,X\n"));
    const auto result = s.shuffle();
    ASSERT_TRUE(result.has_value()) << "Shuffle ended with result: " << static_cast<int>(result.error());
    EXPECT_TRUE(s.validateResult());
}

TEST(GridShuffler, OverlappingBuddyNamesKeepRemainingPairs) {
    //  A 組 {A, C}、B 組 {A, B}：重疊的 A 被雙向剔除後 → A 組 {C}、B 組 {B}，
    //  C 仍需 B 鄰居（剔除不得連帶刪掉 C 的需求）。
    //  佈局 A,C,X,B：C 原只鄰 A（重疊者）——剔除後 C 必須改配 B，能量面必然改變；
    //  若剔除未實作，C 靠 A 即可滿足 buddy，結果不會把 C、B 湊在一起。
    GridShuffler s(42);
    s.setConfig(ShuffleConfig{cfg}.setEnableBuddyMatching(true).setBuddyGroups({"A", "C"}, {"A", "B"}));
    s.setGrid(Grid::fromCSVString("A,C,X,B\n"));
    const auto result = s.shuffle();
    ASSERT_TRUE(result.has_value()) << "Shuffle ended with result: " << static_cast<int>(result.error());
    EXPECT_TRUE(s.validateResult());
    EXPECT_TRUE(hasBuddyNeighbor(s.getGrid(), "C", {"B"}, cfg));  //  C 的新搭檔是 B
}

TEST(PenaltyWeights, BuddyPreferenceDefaultsToSoftWeight) {
    //  buddyPreference 是 soft 項：預設應遠低於硬約束（1000 級）且不得為 0
    //  （0 會使方位偏好完全不引導搜尋）。
    EXPECT_EQ(PenaltyWeights{}.buddyPreference, 100);
}

TEST(GridShuffler, BuddyDirectionPreferenceIsSoftWhenPositionImpossible) {
    const auto src = Grid::fromCSVString("A,X,Y\nB,Z,W\n");
    const auto makeCfg = [](const PrioritizeBuddyPairPosition pref) {
        return ShuffleConfig{cfg}
            .setEnableBuddyMatching(true)
            .setDoBuddyRotate(false)
            .setBuddyGroups({"A"}, {"B"})
            .forceRow("A", 0)
            .forceCol("A", 0)
            .forceRow("B", 1)
            .forceCol("B", 0)
            .setPrioritizeBuddyPairPosition(pref);
    };

    GridShuffler lr(42);
    lr.setConfig(makeCfg(PrioritizeBuddyPairPosition::LeftAndRight));
    lr.setGrid(src);
    const auto lrResult = lr.shuffle();
    ASSERT_TRUE(lrResult.has_value()) << "Shuffle ended with result: " << static_cast<int>(lrResult.error());
    EXPECT_TRUE(lr.validateResult());
    EXPECT_TRUE(hasBuddyNeighbor(lr.getGrid(), "A", {"B"}, cfg));  //  硬規則（有搭檔）仍滿足
    
    GridShuffler fb(42);
    fb.setConfig(makeCfg(PrioritizeBuddyPairPosition::FrontAndBack));
    fb.setGrid(src);
    const auto fbResult = fb.shuffle();
    ASSERT_TRUE(fbResult.has_value()) << "Shuffle ended with result: " << static_cast<int>(fbResult.error());
    EXPECT_TRUE(fb.validateResult());
}

TEST(GridShuffler, BuddyDirectionPreferenceGuidesTowardLeftAndRight) {
    const auto src = Grid::fromCSVString("A,X,Y\nB,Z,W\n");
    const auto prefCfg = ShuffleConfig{cfg}.setPrioritizeBuddyPairPosition(PrioritizeBuddyPairPosition::LeftAndRight);

    GridShuffler s(42);
    s.setConfig(
        ShuffleConfig{prefCfg}.setEnableBuddyMatching(true).setDoBuddyRotate(false).setBuddyGroups({"A"}, {"B"})
    );
    s.setGrid(src);
    const auto result = s.shuffle();
    ASSERT_TRUE(result.has_value()) << "Shuffle ended with result: " << static_cast<int>(result.error());
    EXPECT_TRUE(s.validateResult());
    EXPECT_TRUE(hasBuddyNeighbor(s.getGrid(), "A", {"B"}, cfg));
    EXPECT_TRUE(hasBuddyInPreferredDirection(s.getGrid(), "A", {"B"}, prefCfg));
}