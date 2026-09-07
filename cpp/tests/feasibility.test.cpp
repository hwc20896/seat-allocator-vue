#include <gtest/gtest.h>

#include "feasibility.hpp"

TEST(Feasibility, NoConstraints3x3IsFeasible) {
    const Grid grid = Grid::fromCSVString("A,B,C\nD,E,F\nG,H,I\n");
    const auto [status, layer, reason] = checkFeasibility(grid, ShuffleConfig{});
    EXPECT_EQ(status, FeasibilityStatus::Feasible) << "layer=" << layer << " reason=" << reason;
}

TEST(Feasibility, DuplicateNamesAreFeasible) {
    const Grid grid = Grid::fromCSVString("A,A,B\nC,D,E\nF,G,H\n");
    const auto [status, layer, reason] = checkFeasibility(grid, ShuffleConfig{});
    EXPECT_EQ(status, FeasibilityStatus::Feasible) << "layer=" << layer << " reason=" << reason;
}

TEST(Feasibility, FrozenEmptyCellsAreFeasible) {
    const Grid grid = Grid::fromCSVString("A,B,C\nD,,F\nG,H,I\n");
    const auto [status, layer, reason] = checkFeasibility(grid, ShuffleConfig{});
    EXPECT_EQ(status, FeasibilityStatus::Feasible) << "layer=" << layer << " reason=" << reason;
}

TEST(Feasibility, FourForcedToSameRowExceedsCapacity) {
    const Grid grid = Grid::fromCSVString("A,B,C\nD,E,F\nG,H,I\n");
    const auto currentConfig = ShuffleConfig{}.forceRow("A", 0).forceRow("B", 0).forceRow("C", 0).forceRow("D", 0);
    const auto [status, layer, reason] = checkFeasibility(grid, currentConfig);
    EXPECT_EQ(status, FeasibilityStatus::Unsatisfiable) << "layer=" << layer << " reason=" << reason;
    EXPECT_EQ(layer, "domain");
}

TEST(Feasibility, ForbidAllRowsLeavesEmptyDomain) {
    const Grid grid = Grid::fromCSVString("A,B,C\nD,E,F\nG,H,I\n");
    const auto currentConfig = ShuffleConfig{}.forbidRow("A", 0).forbidRow("A", 1).forbidRow("A", 2);
    const auto [status, layer, reason] = checkFeasibility(grid, currentConfig);
    EXPECT_EQ(status, FeasibilityStatus::Unsatisfiable) << "layer=" << layer << " reason=" << reason;
    EXPECT_EQ(layer, "domain");
}

TEST(Feasibility, ForceAndForbidSameRowConflicts) {
    const Grid grid = Grid::fromCSVString("A,B,C\nD,E,F\nG,H,I\n");
    const auto currentConfig = ShuffleConfig{}.forceRow("A", 1).forbidRow("A", 1);
    const auto [status, layer, reason] = checkFeasibility(grid, currentConfig);
    EXPECT_EQ(status, FeasibilityStatus::Unsatisfiable) << "layer=" << layer << " reason=" << reason;
    EXPECT_EQ(layer, "domain");
}

TEST(Feasibility, TwoElementsLockedToSameCellFailsMatching) {
    const Grid grid = Grid::fromCSVString("A,B,C\nD,E,F\nG,H,I\n");
    const auto currentConfig = ShuffleConfig{}.forceRow("A", 0).forceCol("A", 0).forceRow("B", 0).forceCol("B", 0);
    const auto [status, layer, reason] = checkFeasibility(grid, currentConfig);
    EXPECT_EQ(status, FeasibilityStatus::Unsatisfiable) << "layer=" << layer << " reason=" << reason;
    EXPECT_EQ(layer, "matching");
}

TEST(Feasibility, ThreeMutuallyExclusiveWithTwoRowsUnsatisfiable) {
    const Grid grid = Grid::fromCSVString("A,B\nC,D\n");
    const auto currentConfig =
        ShuffleConfig{}.forbidShareRow("A", "B").forbidShareRow("B", "C").forbidShareRow("A", "C");
    const auto [status, layer, reason] = checkFeasibility(grid, currentConfig);
    EXPECT_EQ(status, FeasibilityStatus::Unsatisfiable) << "layer=" << layer << " reason=" << reason;
    EXPECT_EQ(layer, "coloring");
}

TEST(Feasibility, ExclusivePairForcedToSameRowConflicts) {
    const Grid grid = Grid::fromCSVString("A,B,C\nD,E,F\nG,H,I\n");
    const auto currentConfig = ShuffleConfig{}.forceRow("A", 1).forceRow("B", 1).forbidShareRow("A", "B");
    const auto [status, layer, reason] = checkFeasibility(grid, currentConfig);
    EXPECT_EQ(status, FeasibilityStatus::Unsatisfiable) << "layer=" << layer << " reason=" << reason;
    EXPECT_EQ(layer, "domain");
}

TEST(Feasibility, ThreeMutuallyExclusiveWithThreeRowsFeasible) {
    const Grid grid = Grid::fromCSVString("A,B,C\nD,E,F\nG,H,I\n");
    const auto currentConfig =
        ShuffleConfig{}.forbidShareRow("A", "B").forbidShareRow("B", "C").forbidShareRow("A", "C");
    const auto [status, layer, reason] = checkFeasibility(grid, currentConfig);
    EXPECT_EQ(status, FeasibilityStatus::Feasible) << "layer=" << layer << " reason=" << reason;
}

TEST(Feasibility, ZeroColoringBudgetReturnsUnknown) {
    const Grid grid = Grid::fromCSVString("A,B,C\nD,E,F\nG,H,I\n");
    const auto currentConfig =
        ShuffleConfig{}.forbidShareRow("A", "B").forbidShareRow("B", "C").forbidShareRow("A", "C");
    constexpr FeasibilityOptions opts{.checkForbidShare = true, .coloringNodeBudget = 0};
    const auto [status, layer, reason] = checkFeasibility(grid, currentConfig, opts);
    EXPECT_EQ(status, FeasibilityStatus::Unknown) << "layer=" << layer << " reason=" << reason;
    EXPECT_EQ(layer, "coloring");
}

TEST(Feasibility, ExclusivePairForcedToSameColConflicts) {
    const Grid grid = Grid::fromCSVString("A,B,C\nD,E,F\nG,H,I\n");
    const auto currentConfig = ShuffleConfig{}.forceCol("A", 1).forceCol("B", 1).forbidShareCol("A", "B");
    const auto [status, layer, reason] = checkFeasibility(grid, currentConfig);
    EXPECT_EQ(status, FeasibilityStatus::Unsatisfiable) << "layer=" << layer << " reason=" << reason;
    EXPECT_EQ(layer, "domain");
}

TEST(Feasibility, EmptyGridIsUnsatisfiable) {
    const Grid grid;
    const auto [status, layer, reason] = checkFeasibility(grid, ShuffleConfig{});
    EXPECT_EQ(status, FeasibilityStatus::Unsatisfiable) << "layer=" << layer << " reason=" << reason;
    EXPECT_EQ(layer, "domain");
}

TEST(Feasibility, ColoringDisabledFallsThroughToFeasible) {
    const Grid grid = Grid::fromCSVString("A,B\nC,D\n");
    const auto currentConfig =
        ShuffleConfig{}.forbidShareRow("A", "B").forbidShareRow("B", "C").forbidShareRow("A", "C");
    constexpr FeasibilityOptions opts{.checkForbidShare = false};
    const auto [status, layer, reason] = checkFeasibility(grid, currentConfig, opts);
    EXPECT_EQ(status, FeasibilityStatus::Feasible) << "layer=" << layer << " reason=" << reason;
}