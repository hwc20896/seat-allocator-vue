#pragma once

#include <algorithm>
#include <cstdint>
#include <optional>
#include <ranges>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <functional>

#include "configs.hpp"
#include "dynamic-bitset.hpp"
#include "grid.hpp"
#include "utils.hpp"

// ═══════════════════════════════════════════════════════════════════
// 可行性檢查（Feasibility Check）
// ───────────────────────────────────────────────────────────────────
// 目的：在跑昂貴的模擬退火之前，先判斷「這組 constraints 在目前
//       grid 下是否可能解出來」，避免白跑一輪才回報 Unsatisfiable。
//
// 回傳三態：
//   Feasible       → 已證明存在解
//   Unsatisfiable  → 已證明無解（某一層抓到矛盾）
//   Unknown        → 檢查預算內無法判定（約束太密，交給退火處理）
//
// 檢查由便宜到貴共三層，任一層失敗就代表無解，可以立刻停手：
//
//   L1 可行域（domain）── 每個元素的可放位置集合是否為空
//       例：A 被 forceRow(0) 又 forbidRow(0)，A 就無處可去；
//           4 人都被 forceRow 到同一列，但每列只有 3 格，塞不下。
//
//   L2 完美匹配 ── 元素 ↔ 位置 的二分圖能否全部配對（匈牙利演算法）
//       例：A、B 都只能進同一個格子，位置只有 1 個，必然失敗。
//
//   L3 著色 ── ForbidShare 兩兩互斥，等同「同列/同行的圖著色」
//       例：A、B、C 三人兩兩 forbidShareRow，但只有 2 列，
//           三人注定要有人同列 → 無解。
//
// 已知限制：
//   * L1 + L2 對「沒有 ForbidShare 約束」的情形是精確的（等價於
//     完美匹配存在與否）。
//   * 加入 L3 後仍是「很強的必要過濾 + 充分條件」：著色成功且匹配
//     存在，不代表兩者一定能同時滿足（匹配與著色的耦合未窮舉），
//     但實務上教室規模的約束幾乎都能在此判定。
// ═══════════════════════════════════════════════════════════════════

enum class FeasibilityStatus : int {
    Feasible,
    Unsatisfiable,
    Unknown,
};

struct FeasibilityOptions {
    bool checkForbidShare = true;
    int coloringNodeBudget = 50'000;
};

// 檢查結果
struct FeasibilityReport {
    FeasibilityStatus status = FeasibilityStatus::Unknown;
    std::string layer;   // 判定層："domain" / "matching" / "coloring" / "ok"
    std::string reason;  // 人性化原因說明，例如：元素 'A' 的可放位置不足
};

namespace feasibility_detail {

// 每個（去重後的）元素的可放位置資訊
struct ElementProfile {
    std::string name;
    int count = 0;              // 在 grid 中出現次數（同名格子數）
    int forcedRow = -1;         // 鎖定列（-1 = 未鎖定）
    int forcedCol = -1;         // 鎖定行（-1 = 未鎖定）
    DynamicBitset rows;         // 可放列遮罩（1 = 可放）
    DynamicBitset cols;         // 可放行遮罩（1 = 可放）
    std::vector<int> positions; // 展開後的可放位置（已剔除空格位置）

    explicit ElementProfile(std::string name_, const uint64_t rowCount, const uint64_t colCount)
        : name(std::move(name_)),
          rows{rowCount},
          cols{colCount}
    {
        rows.fill(true);
        cols.fill(true);
    }
};

std::optional<std::string> layer1(
    const Grid& grid,
    const ShuffleConfig& cfg,
    std::vector<ElementProfile>& profiles
) {
    const int rows = grid.rowCount();
    const int cols = grid.colCount();

    std::vector<uint8_t> isEmptyPos(grid.size(), 0);
    for (int i = 0; i < static_cast<int>(grid.size()); ++i) {
        if (grid[i].empty()) isEmptyPos[i] = 1;
    }

    std::unordered_map<std::string, int> nameToIdx;
    for (const auto& cell : grid) {
        if (cell.empty()) continue;
        const auto inserted = nameToIdx.try_emplace(cell, static_cast<int>(profiles.size())).second;
        if (inserted) {
            profiles.emplace_back(cell, rows, cols);
        }
        ++profiles[nameToIdx[cell]].count;
    }

    std::vector<StringPair> shareRows, shareCols;
    for (const auto& c : cfg.constraints) {
        std::visit(overloaded{
            [&](const ForceRow& c2) {
                const auto it = nameToIdx.find(c2.first);
                if (it != nameToIdx.end() && c2.second >= 0 && c2.second < rows) {
                    // force 到列 r：其他列全部關閉；目標列若被 forbid 過則保持關閉
                    auto& p = profiles[it->second];
                    p.forcedRow = c2.second;
                    for (int r = 0; r < rows; ++r) {
                        if (r != c2.second) p.rows.set(r, false);
                    }
                }
            },
            [&](const ForbidRow& c2) {
                const auto it = nameToIdx.find(c2.first);
                if (it != nameToIdx.end() && c2.second >= 0 && c2.second < rows) {
                    profiles[it->second].rows.set(c2.second, false);
                }
            },
            [&](const ForceCol& c2) {
                const auto it = nameToIdx.find(c2.first);
                if (it != nameToIdx.end() && c2.second >= 0 && c2.second < cols) {
                    auto& p = profiles[it->second];
                    p.forcedCol = c2.second;
                    for (int col = 0; col < cols; ++col) {
                        if (col != c2.second) p.cols.set(col, false);
                    }
                }
            },
            [&](const ForbidCol& c2) {
                const auto it = nameToIdx.find(c2.first);
                if (it != nameToIdx.end() && c2.second >= 0 && c2.second < cols) {
                    profiles[it->second].cols.set(c2.second, false);
                }
            },
            [&](const ForbidShareRow& c2) {
                shareRows.emplace_back(c2.first, c2.second);
            },
            [&](const ForbidShareCol& c2) {
                shareCols.emplace_back(c2.first, c2.second);
            },
        }, c);
    }

    // 展開可行域並檢查是否塞得下
    for (auto& p : profiles) {
        p.positions.reserve(static_cast<size_t>(rows) * cols);
        for (int r = 0; r < rows; ++r) {
            if (!p.rows.test(r)) continue;
            for (int c = 0; c < cols; ++c) {
                if (!p.cols.test(c)) continue;
                const int pos = r * cols + c;
                if (isEmptyPos[pos]) continue;
                p.positions.push_back(pos);
            }
        }
        if (p.positions.size() < static_cast<size_t>(p.count)) {
            return "元素 '" + p.name + "' 的可放位置（" +
                   std::to_string(p.positions.size()) + " 格）少於出現次數（" +
                   std::to_string(p.count) + " 次）";
        }
    }

    // 容量檢查：同一列/行被 force 的元素總數（含重複）不得超過容量
    std::vector<int> forcedPerRow(rows, 0), forcedPerCol(cols, 0);
    for (const auto& p : profiles) {
        if (p.forcedRow != -1) forcedPerRow[p.forcedRow] += p.count;
        if (p.forcedCol != -1) forcedPerCol[p.forcedCol] += p.count;
    }
    for (int r = 0; r < rows; ++r) {
        if (forcedPerRow[r] > cols) {
            return "第 " + std::to_string(r) + " 列被鎖定 " +
                   std::to_string(forcedPerRow[r]) + " 個元素，超過容量 " +
                   std::to_string(cols);
        }
    }
    for (int c = 0; c < cols; ++c) {
        if (forcedPerCol[c] > rows) {
            return "第 " + std::to_string(c) + " 行被鎖定 " +
                   std::to_string(forcedPerCol[c]) + " 個元素，超過容量 " +
                   std::to_string(rows);
        }
    }

    const auto checkShare = [&](const std::vector<StringPair>& edges, const auto& domain) -> std::optional<std::string> {
        for (const auto& [a, b] : edges) {
            if (a == b) continue;
            const auto ia = nameToIdx.find(a);
            const auto ib = nameToIdx.find(b);
            if (ia == nameToIdx.end() || ib == nameToIdx.end()) continue;  // 未知名字忽略（與 rebuildConstraints 一致）
            const ElementProfile& pa = profiles[ia->second];
            const ElementProfile& pb = profiles[ib->second];
            // 交集為空 → 永不同列 → 安全；雙方都是單例（且交集非空）→ 必然同列 → 衝突
            if (std::invoke(domain, pa).trueCount() != 1 || std::invoke(domain, pb).trueCount() != 1) continue;
            const auto& da = std::invoke(domain, pa);
            const auto& db = std::invoke(domain, pb);
            bool same = false;
            for (int i = 0; i < static_cast<int>(da.size()); ++i) {
                if (da.test(i) && db.test(i)) { same = true; break; }
            }
            if (same) {
                return "元素 '" + a + "' 與 '" + b +
                       "' 都被鎖定在同一個位置範圍，卻被 forbidShare 禁止共列/共行";
            }
        }
        return std::nullopt;
    };

    if (auto fail = checkShare(shareRows, &ElementProfile::rows)) return fail;
    if (auto fail = checkShare(shareCols, &ElementProfile::cols)) return fail;

    return std::nullopt;
}

bool tryAugment(
    const int e,
    const Graph& adj,
    std::vector<int>& matchPos,
    DynamicBitset& visited
) {
    for (const int pos : adj[e]) {
        if (visited.test(pos)) continue;
        visited.set(pos, true);
        if (matchPos[pos] == -1 || tryAugment(matchPos[pos], adj, matchPos, visited)) {
            matchPos[pos] = e;
            return true;
        }
    }
    return false;
}

std::optional<std::string> layer2(
    const Grid& grid,
    const std::vector<ElementProfile>& profiles
) {
    // 展開節點：同名元素（count 次）共享同一個可行域
    std::vector<std::vector<int>> adj;
    for (const auto& p : profiles) {
        for (int i = 0; i < p.count; ++i) {
            adj.push_back(p.positions);
        }
    }

    std::vector matchPos(grid.size(), -1);
    DynamicBitset visited(grid.size());
    for (int e = 0; e < static_cast<int>(adj.size()); ++e) {
        visited.reset();
        if (!tryAugment(e, adj, matchPos, visited)) {
            return "存在元素無法配到不同的位置（完美匹配失敗）";
        }
    }
    return std::nullopt;
}

struct ColoringOutcome {
    bool feasible = false;
    bool budgetExceeded = false;
};

ColoringOutcome solveColoring(
    const DynamicBitset& adj,               // 衝突邊（n×n 位矩陣，adj[v*n+u] = 有邊）
    const int n,                            // 節點數
    const std::vector<int>& forcedColor,    // 鎖定顏色（-1 = 未鎖定）
    const int colorCount,                   // 顏色數（列數或行數）
    const int colorCapacity,                // 每色容量（每列/行格數）
    const int budget                        // 回溯節點預算
) {
    std::vector color(n, -1);
    std::vector used(colorCount, 0);
    int nodes = 0;

    const auto hasEdge = [&](const int v, const int u) {
        return adj.test(static_cast<uint64_t>(v) * n + u);
    };

    // 預著色：被 force 鎖定的元素直接指定顏色；與鄰居撞色 → 無解
    for (int v = 0; v < n; ++v) {
        if (forcedColor[v] == -1) continue;
        for (int u = 0; u < n; ++u) {
            if (hasEdge(v, u) && color[u] == forcedColor[v]) {
                return {.feasible = false, .budgetExceeded = false};
            }
        }
        color[v] = forcedColor[v];
        ++used[forcedColor[v]];
    }
    const int preColored =
        static_cast<int>(std::ranges::count_if(color, [](const int c) { return c != -1; }));

    // 回傳：1 = 著色成功，0 = 無解，2 = 超預算
    auto dfs = [&](this auto&& self, const int depth) -> int {
        if (++nodes > budget) return 2;
        if (depth == n) return 1;

        int best = -1, bestScore = -1, bestDeg = -1;
        for (int v = 0; v < n; ++v) {
            if (color[v] != -1) continue;
            std::vector<uint8_t> seen(colorCount, 0);
            int score = 0;
            int deg = 0;
            for (int u = 0; u < n; ++u) {
                if (!hasEdge(v, u)) continue;
                ++deg;
                if (color[u] != -1 && !seen[color[u]]) {
                    seen[color[u]] = 1;
                    ++score;
                }
            }
            if (score > bestScore || (score == bestScore && deg > bestDeg)) {
                best = v;
                bestScore = score;
                bestDeg = deg;
            }
        }

        // 依序嘗試可用顏色（避開衝突與超容量）
        for (int c = 0; c < colorCount; ++c) {
            if (used[c] >= colorCapacity) continue;
            if (std::ranges::any_of(std::views::iota(0, n), [&](const int u) {
                return hasEdge(best, u) && color[u] == c;
            })) {
                continue;
            }
            color[best] = c;
            ++used[c];
            const int r = self(depth + 1);
            if (r != 0) return r;  // 成功或超預算都直接回傳
            color[best] = -1;
            --used[c];
        }
        return 0;
    };

    const int r = dfs(preColored);
    return {.feasible = r == 1, .budgetExceeded = r == 2};
}

std::optional<std::string> layer3(
    const Grid& grid,
    const ShuffleConfig& cfg,
    const std::vector<ElementProfile>& profiles,
    const FeasibilityOptions& opts,
    bool& budgetExceeded
) {
    const int rows = grid.rowCount();
    const int cols = grid.colCount();

    std::unordered_map<std::string, int> nameToIdx;
    for (int i = 0; i < static_cast<int>(profiles.size()); ++i) {
        nameToIdx[profiles[i].name] = i;
    }

    std::vector<StringPair> shareRows, shareCols;
    for (const auto& c : cfg.constraints) {
        std::visit(overloaded{
            [&](const ForbidShareRow& c2) {
                shareRows.emplace_back(c2.first, c2.second);
            },
            [&](const ForbidShareCol& c2) {
                shareCols.emplace_back(c2.first, c2.second);
            },
            [](const auto&) {},
        }, c);
    }

    // 對「列方向」與「行方向」各做一次著色
    budgetExceeded = false;
    const auto run = [&](const std::vector<StringPair>& edges, const int colorCount, const int colorCapacity,
                         const auto& forcedColorOf) -> std::optional<std::string> {
        if (edges.empty()) return std::nullopt;

        // 節點 = 有 forbidShare 邊的名字（去重）；未知名字忽略（與 rebuildConstraints 一致）
        std::vector nodeIdx(profiles.size(), -1);
        std::vector<int> forcedColor;
        for (const auto& [a, b] : edges) {
            for (const auto& name : {a, b}) {
                const auto it = nameToIdx.find(name);
                if (it == nameToIdx.end()) continue;
                if (nodeIdx[it->second] == -1) {
                    nodeIdx[it->second] = static_cast<int>(forcedColor.size());
                    forcedColor.push_back(std::invoke(forcedColorOf, profiles[it->second]));
                }
            }
        }
        const auto n = forcedColor.size();
        if (n == 0) return std::nullopt;

        // 2D 位矩陣（與 shuffler 的 originalNeighborsMatrix_ 同款佈局）
        DynamicBitset adj(n * n);
        for (const auto& [a, b] : edges) {
            const auto ia = nameToIdx.find(a);
            const auto ib = nameToIdx.find(b);
            if (ia == nameToIdx.end() || ib == nameToIdx.end()) continue;
            const int va = nodeIdx[ia->second];
            const int vb = nodeIdx[ib->second];
            if (va == -1 || vb == -1 || va == vb) continue;
            adj.set(va * n + vb, true);
            adj.set(vb * n + va, true);
        }

        const auto [outcomeFeasible, outcomeBudgetExceeded] =
            solveColoring(adj, n, forcedColor, colorCount, colorCapacity, opts.coloringNodeBudget);
        if (outcomeBudgetExceeded) {
            budgetExceeded = true;
            return "著色檢查超出預算（此原因不會被回報，主函式會轉成 Unknown）";
        }
        if (!outcomeFeasible) {
            return "存在 forbidShare 互斥群，無法在 " + std::to_string(colorCount) +
                   " 個列/行內錯開";
        }
        return std::nullopt;
    };

    if (auto fail = run(shareRows, rows, cols, &ElementProfile::forcedRow)) return fail;
    if (auto fail = run(shareCols, cols, rows, &ElementProfile::forcedCol)) return fail;
    return std::nullopt;
}

}

[[nodiscard]]
FeasibilityReport checkFeasibility(
    const Grid& grid,
    const ShuffleConfig& cfg,
    const FeasibilityOptions& opts = {}
) {
    FeasibilityReport report;

    if (grid.empty()) {
        report = {.status = FeasibilityStatus::Unsatisfiable, .layer = "domain", .reason = "grid 為空，無從安排"};
        return report;
    }

    // L1：可行域 / 容量 / 必然衝突
    std::vector<feasibility_detail::ElementProfile> profiles;
    if (const auto fail = feasibility_detail::layer1(grid, cfg, profiles)) {
        report = {.status = FeasibilityStatus::Unsatisfiable, .layer = "domain", .reason = *fail};
        return report;
    }

    // L2：完美匹配
    if (const auto fail = feasibility_detail::layer2(grid, profiles)) {
        report = {.status = FeasibilityStatus::Unsatisfiable, .layer = "matching", .reason = *fail};
        return report;
    }

    // L3：ForbidShare 著色
    if (opts.checkForbidShare) {
        bool budgetExceeded = false;
        if (const auto fail = feasibility_detail::layer3(grid, cfg, profiles, opts, budgetExceeded)) {
            if (budgetExceeded) {
                report = {.status = FeasibilityStatus::Unknown, .layer = "coloring", .reason = "著色檢查超出預算，無法判定（可調高 coloringNodeBudget）"};
            } else {
                report = {.status = FeasibilityStatus::Unsatisfiable, .layer = "coloring", .reason = *fail};
            }
            return report;
        }
    }

    report = {.status = FeasibilityStatus::Feasible, .layer = "ok", .reason = "所有分層檢查皆通過"};
    return report;
}