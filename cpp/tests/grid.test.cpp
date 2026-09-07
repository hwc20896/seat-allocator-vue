#include <gtest/gtest.h>
#include "grid.hpp"

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
    EXPECT_EQ(g[5], "X");  // 1*3+2 = 5
    g[0] = "Y";
    EXPECT_EQ((g[0, 0]), "Y");
}

TEST(GridAccess, OutOfRangeThrows) {
    Grid g(2, 2);
    EXPECT_THROW((g[2, 0]), std::out_of_range);
    EXPECT_THROW((g[0, -1]), std::out_of_range);
    EXPECT_THROW(g[4], std::out_of_range);
    EXPECT_THROW((void)g.get(1, 2), std::out_of_range);
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
    EXPECT_THROW((void)g[-1], std::out_of_range);
    const Grid& cg = g;
    EXPECT_THROW((void)cg[-1], std::out_of_range);
}