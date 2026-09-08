#pragma once

#include "configs.hpp"
#include "grid.hpp"

inline constexpr auto cfg = ShuffleConfig{}.setAllowOriginalNeighbors(true);
inline constexpr auto strictCfg = ShuffleConfig{}.setAllowFixedPoints(false).setAllowOriginalNeighbors(false);

//  復刻 shuffler.hpp 的 ray-cast 鄰居語義：沿方向掃描直到第一個非空格，
//  空格是否穿透（走廊視線）由 crossAisleAreNeighbors 決定。
constexpr std::vector<int> rayNeighbors(
    const Grid& grid, const int pos, const bool diagonals, const bool crossAisle
) {
    const int rows = grid.rowCount();
    const int cols = grid.colCount();
    const int sr = pos / cols;
    const int sc = pos % cols;

    static constexpr std::pair<int, int> cardinalDirs[] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    static constexpr std::pair<int, int> diagonalDirs[] = {{-1, -1}, {-1, 1}, {1, -1}, {1, 1}};

    std::vector<int> res;
    const auto castRay = [&](const int dr, const int dc) {
        int r = sr + dr;
        int c = sc + dc;
        while (r >= 0 && r < rows && c >= 0 && c < cols) {
            const int idx = r * cols + c;
            if (!grid[idx].empty()) {
                res.push_back(idx);
                return;
            }
            if (!crossAisle) return;
            r += dr;
            c += dc;
        }
    };
    for (const auto& [dr, dc] : cardinalDirs) castRay(dr, dc);
    if (diagonals) {
        for (const auto& [dr, dc] : diagonalDirs) castRay(dr, dc);
    }
    return res;
}

//  name 在 grid 中是否至少有一個屬於 buddies 名單的鄰居（與演算法同語義）。
inline bool hasBuddyNeighbor(
    const Grid& grid, const std::string& name, const std::vector<std::string>& buddies,
    const ShuffleConfig& config
) {
    const auto it = std::ranges::find(grid, name);
    if (it == grid.end()) return false;
    const int pos = static_cast<int>(std::ranges::distance(grid.begin(), it));

inline bool hasBuddyInPreferredDirection(
    const Grid& grid, const std::string& name, const std::vector<std::string>& buddies, const ShuffleConfig& config
) {
    const auto it = std::ranges::find(grid, name);
    if (it == grid.end()) return false;
    const int pos = static_cast<int>(std::ranges::distance(grid.begin(), it));
    const int row = pos / grid.colCount();
    const int col = pos % grid.colCount();
    const auto pref = config.prioritizeBuddyPairPosition;
    return std::ranges::any_of(
        rayNeighbors(grid, pos, config.diagonalsAreNeighbors, config.crossAisleAreNeighbors), [&](const int n) {
            if (!std::ranges::contains(buddies, grid[n])) return false;
            const int nRow = n / grid.colCount();
            const int nCol = n % grid.colCount();
            return (pref == PrioritizeBuddyPairPosition::LeftAndRight && nRow == row) ||
                   (pref == PrioritizeBuddyPairPosition::FrontAndBack && nCol == col);
        }
    );
}