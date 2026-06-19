#include "algorithm.hpp"

#include <utility>
#include <chrono>
#include <format>
#include <ranges>
#include <algorithm>
#include <limits>
#include <array>
#include <spdlog/spdlog.h>
#include <future>

#include "utils/constants.hpp"

GridShuffler::GridShuffler(ShuffleConfig config)
  : rowCount(0), columnCount(0), config(std::move(config)), forbiddenAdjMatrix(0), numItems(0), dim(0), rng(std::random_device{}())
{
    dirs = {{0, 1}, {1, 0}, {0, -1}, {-1, 0}};
    static constexpr auto diagonalDirs = std::array<Position, 4>({{-1, 1}, {1, 1}, {-1, -1}, {1, -1}});
    if (config.diagonals_are_neighbors)
        dirs.append_range(diagonalDirs);
}

size_t GridShuffler::getSize() const noexcept {
    return data.size();
}

bool GridShuffler::setGrid(const Grid& grid) {
    rowCount = grid.size();
    columnCount = rowCount ? grid[0].size() : 0;

    originalGrid = grid;

    data.clear();
    nodeToPos.clear();
    idToString.clear();
    stringToID.clear();
    graph.clear();
    forbiddenAdjMatrix.reset();
    originalValueAtNode.clear();
    nodesByRow.clear();
    nodesByColumn.clear();
    domainMask.clear();
    numItems = 0;
    dim = 0;

    if (rowCount == 0 || columnCount == 0)
        return false;

    initTopology();

    try {
        initConstraints();
    }
    catch (std::exception& e) {
        spdlog::critical("Error: {}", e.what());
        throw;
    }

    initDomains();
    return true;
}

const Grid& GridShuffler::getOriginalGrid() const {
    return originalGrid;
}

const Grid& GridShuffler::getGrid() const {
    return data.empty()? originalGrid : data.back();
}

void GridShuffler::shuffle() {
    if (numItems == 0) return;

    if (dim < numItems) {
        const auto msg = std::format("Unable to shuffle: ({}) is less than quantity of non-null element count ({})", dim, numItems);
        spdlog::critical(msg);
        throw std::invalid_argument(msg);
    }

    using namespace std::chrono;

    const auto startTime = high_resolution_clock::now();

    #if defined(__EMSCRIPTEN_PTHREADS__) && (__EMSCRIPTEN_PTHREADS__ == 1)
    // ==========================================
    // MULTITHREADED EXECUTION (Pthreads enabled)
    // ==========================================
    std::atomic found{false};
    std::mutex resultMutex;
    Grid resultGrid;
    std::exception_ptr exceptionPtr;

    std::vector<std::future<void>> futures;

    for ([[maybe_unused]] const auto t : std::views::iota(0, Constants::NUM_THREADS)) {
        futures.push_back(std::async(std::launch::async, [&] {
            try {
                auto assignment = std::vector<std::optional<int>>(numItems, std::nullopt);
                auto usedValues = std::vector(dim, false);
                auto threadLocalDomainMask = domainMask;

                for ([[maybe_unused]] const auto _ : std::views::iota(0, Constants::ATTEMPTS_PER_THREAD)) {
                    if (found.load(std::memory_order_relaxed)) return;

                    if (solve(assignment, usedValues, threadLocalDomainMask)) {
                        if (!found.exchange(true)) {
                            auto newGrid = std::vector(rowCount, std::vector(columnCount, std::string()));
                            for (int nodeIdx = 0; nodeIdx < numItems; nodeIdx++) {
                                assignment[nodeIdx].and_then([this, &newGrid, nodeIdx](const int valID) -> std::optional<int> {
                                    const auto& [row, column] = nodeToPos.at(nodeIdx);
                                    newGrid[row][column] = idToString.at(valID);
                                    return {};
                                });
                            }

                            std::lock_guard lock(resultMutex);
                            resultGrid = std::move(newGrid);
                        }
                        return;
                    }

                    std::ranges::fill(assignment, std::nullopt);
                    std::ranges::fill(usedValues, false);
                    threadLocalDomainMask = domainMask;
                }
            }
            catch (...) {
                std::lock_guard lock(resultMutex);
                if (!exceptionPtr) {
                    exceptionPtr = std::current_exception();
                }
            }
        }));
    }

    for (auto& f : futures) {
        f.get();
    }

    if (found.load()) {
        const auto endTime = high_resolution_clock::now();
        const auto duration = duration_cast<microseconds>(endTime - startTime);
        spdlog::info("[OK] Shuffle successful (took {:.3f}ms).", duration.count()/1000.0);

        data.push_back(std::move(resultGrid));
        return;
    }

    if (exceptionPtr) {
        std::rethrow_exception(exceptionPtr);
    }

#else
    // ==========================================
    // SINGLE-THREADED FALLBACK (Pthreads disabled)
    // ==========================================
    auto assignment = std::vector<std::optional<int>>(numItems, std::nullopt);
    auto usedValues = std::vector(dim, false);
    auto localDomainMask = domainMask;
    bool found = false;

    // Run total attempts sequentially
    const int totalAttempts = Constants::NUM_THREADS * Constants::ATTEMPTS_PER_THREAD;
    for ([[maybe_unused]] const auto _ : std::views::iota(0, totalAttempts)) {
        if (solve(assignment, usedValues, localDomainMask)) {
            auto newGrid = std::vector(rowCount, std::vector(columnCount, std::string()));
            for (int nodeIdx = 0; nodeIdx < numItems; nodeIdx++) {
                assignment[nodeIdx].and_then([this, &newGrid, nodeIdx](const int valID) -> std::optional<int> {
                    const auto& [row, column] = nodeToPos.at(nodeIdx);
                    newGrid[row][column] = idToString.at(valID);
                    return {};
                });
            }
            data.push_back(std::move(newGrid));
            found = true;
            break;
        }

        std::ranges::fill(assignment, std::nullopt);
        std::ranges::fill(usedValues, false);
        localDomainMask = domainMask;
    }

    if (found) {
        const auto endTime = high_resolution_clock::now();
        const auto duration = duration_cast<microseconds>(endTime - startTime);
        spdlog::info("[OK] Shuffle successful (took {:.3f}ms).", duration.count()/1000.0);
        return;
    }
#endif

    const auto endTime = high_resolution_clock::now();
    const auto duration = duration_cast<milliseconds>(endTime - startTime);

    static const auto msg = std::format(
        "[FAIL] Shuffle failed after {} attempts (took {} ms). Constraints may be unsatisfiable.",
        Constants::MAX_ATTEMPTS,
        duration.count()
    );
    spdlog::critical(msg);
    throw std::runtime_error(msg);
}

bool GridShuffler::validateResult() {
    const auto currentGrid = getGrid();

    for (const auto r : std::views::iota(0ULL, rowCount)) {
        for (const auto c : std::views::iota(0ULL, columnCount)) {
            const auto& currentValue = currentGrid[r][c];
            if (currentValue.empty()) continue;

            if (!config.allow_fixed_points &&
                currentValue == originalGrid[r][c])
                return false;

            const auto u = stringToID.at(currentValue);

            for (const auto& [dr, dc] : dirs) {
                const auto [nr, nc] = std::make_pair(r + dr, c + dc);

                if (nr >= rowCount || nc >= columnCount)
                    continue;

                const auto neighborValue = currentGrid[nr][nc];
                if (neighborValue.empty())
                    continue;

                if (isForbidden(u, stringToID.at(neighborValue)))
                    return false;
            }
        }
    }
    return true;
}

void GridShuffler::clearShuffledGrids() {
    data.clear();
}

const std::vector<Grid>& GridShuffler::getAllGrids() const {
    return data;
}

const Grid& GridShuffler::operator[](const int index) const {
    if (index < 0 || index >= data.size())
        return getGrid();
    return data[index];
}

const Grid& GridShuffler::getGrid(const int index) const {
    if (index < 0 || index >= data.size())
        return getGrid();
    return data[index];
}

//  private methods
void GridShuffler::initTopology() {
    auto gridToNode = std::vector(rowCount, std::vector<std::optional<int>>(columnCount, std::nullopt));

    for (const auto r : std::views::iota(0ULL, rowCount)) {
        for (const auto c : std::views::iota(0ULL, columnCount)) {
            if (!originalGrid[r][c].empty()) {
                gridToNode[r][c] = numItems;
                nodeToPos.emplace_back(r, c);
                numItems++;
            }
        }
    }

    nodesByRow = std::vector<std::vector<NodeID>>(rowCount);
    nodesByColumn = std::vector<std::vector<NodeID>>(columnCount);
    for (const auto u : std::views::iota(0, numItems)) {
        const auto [r, c] = nodeToPos[u];
        nodesByRow[r].push_back(u);
        nodesByColumn[c].push_back(u);
    }

    graph = std::vector<std::vector<NodeID>>(numItems);

    for (const auto i : std::views::iota(0, numItems)) {
        const auto& [cr, cc] = nodeToPos[i];
        for (const auto& [dr, dc] : dirs) {
            const auto [nr, nc] = std::make_pair(cr + dr, cc + dc);
            if (nr < 0 || nr >= rowCount || nc < 0 || nc >= columnCount)
                continue;

            gridToNode[nr][nc].and_then([&](const int val) -> std::optional<int> {
                graph[i].push_back(val);
                return std::nullopt;
            });
        }
    }
}

void GridShuffler::initConstraints() {
    idToString.clear();
    stringToID.clear();
    originalValueAtNode = std::vector(numItems, 0);

    int valCounter = 0;
    std::vector<std::string> duplicateElements;

    for (const auto r : std::views::iota(0ULL, rowCount)) {
        for (const auto c : std::views::iota(0ULL, columnCount)) {
            const auto& value = originalGrid[r][c];
            if (value.empty()) continue;

            if (stringToID.contains(value)) {
                if (!std::ranges::contains(duplicateElements, value))
                    duplicateElements.push_back(value);
                continue;
            }

            stringToID.emplace(value, valCounter);
            idToString.push_back(value);
            valCounter++;
        }
    }

    if (!duplicateElements.empty()) {
        const auto msg = std::format("Duplicate elements found: {}.\nPlease ensure all element are unique.", duplicateElements);
        spdlog::critical(msg);
        throw std::invalid_argument(msg);
    }

    dim = idToString.size();

    if (dim < numItems) {
        const auto msg = std::format("Unable to shuffle: ({}) is less than quantity of non-null element count ({})", dim, numItems);
        spdlog::critical(msg);
        throw std::invalid_argument(msg);
    }

    for (const auto i : std::views::iota(0, numItems)) {
        const auto& [r, c] = nodeToPos[i];
        originalValueAtNode[i] = stringToID.at(originalGrid[r][c]);
    }

    const int pow_dim = dim * dim;
    forbiddenAdjMatrix = DynamicBitset(pow_dim);

    if (!config.allow_original_neighbors) {
        for (const auto i : std::views::iota(0, numItems)) {
            const int u = originalValueAtNode[i];
            for (const auto j : graph[i]) {
                const int v = originalValueAtNode[j];
                const auto [idx1, idx2] = std::make_pair(u * dim + v, v * dim + u);
                if (idx1 < pow_dim)
                    forbiddenAdjMatrix.set(idx1, true);
                if (idx2 < pow_dim)
                    forbiddenAdjMatrix.set(idx2, true);
            }
        }
    }

    for (const auto& [s1, s2] : config.custom_forbidden_pairs) {
        const auto [u, v] = std::make_pair(stringToID.at(s1), stringToID.at(s2));
        const auto [idx1, idx2] = std::make_pair(u * dim + v, v * dim + u);
        if (idx1 < pow_dim)
            forbiddenAdjMatrix.set(idx1, true);
        if (idx2 < pow_dim)
            forbiddenAdjMatrix.set(idx2, true);
    }
}

void GridShuffler::initDomains() {
    domainMask = std::vector(numItems, std::vector(dim, true));
    preparedDynamicConstraints.clear();

    for (const auto& constraint : config.constraints) {
        std::visit([&]<typename ConstraintType>(const ConstraintType& c){
            constexpr bool isForce = std::is_same_v<ConstraintType, ForceCol> || std::is_same_v<ConstraintType, ForceRow>;
            constexpr bool isForbid = std::is_same_v<ConstraintType, ForbidCol> || std::is_same_v<ConstraintType, ForbidRow>;
            constexpr bool constraintCol = std::is_same_v<ConstraintType, ForceCol> || std::is_same_v<ConstraintType, ForbidCol>;
            constexpr bool constraintRow = std::is_same_v<ConstraintType, ForceRow> || std::is_same_v<ConstraintType, ForbidRow>;
            constexpr bool isDynamic = std::is_same_v<ConstraintType, ForbidShareCol> || std::is_same_v<ConstraintType, ForbidShareRow>;

            if constexpr (constraintCol || constraintRow) {
                const int limitIdx = c.second;
                const int maxLimit = constraintCol ? columnCount : rowCount;

                if (limitIdx < 0 || limitIdx >= maxLimit) {
                    throw std::invalid_argument(std::format("Constraint Error: Index {} does not exist", limitIdx));
                }

                if (stringToID.contains(c.first)) {
                    const ValueID val_id = stringToID.at(c.first);

                    if constexpr (isForce) {
                        for (const auto u : std::views::iota(0, numItems)) {
                            const auto [nr, nc] = nodeToPos[u];
                            if (const int currentIdx = constraintCol ? nc : nr;
                                currentIdx != limitIdx
                            ) {
                                domainMask[u][val_id] = false;
                            }
                        }
                    }
                    else if constexpr (isForbid) {
                        const auto& nodesToRestrict = constraintCol ? nodesByColumn[limitIdx] : nodesByRow[limitIdx];
                        for (const auto u : nodesToRestrict) {
                            domainMask[u][val_id] = false;
                        }
                    }
                }
            }
            // 2. 處理並預存動態約束 (Share 類型)
            else if constexpr (isDynamic) {
                if (stringToID.contains(c.first) && stringToID.contains(c.second)) {
                    preparedDynamicConstraints.emplace_back(
                        std::is_same_v<ConstraintType, ForbidShareRow> ?
                            DynamicConstraint::Type::ShareRow :
                            DynamicConstraint::Type::ShareCol,
                        stringToID.at(c.first),
                        stringToID.at(c.second)
                    );
                }
            }
        }, constraint);
    }
}

bool GridShuffler::checkDynamicConstraints(const NodeID u, const ValueID v, const AssignmentType& assignment) const {
    const auto [r, c] = nodeToPos[u];

    for (const auto& [type, id1, id2] : preparedDynamicConstraints) {
        ValueID targetPartner;
        if (v == id1) targetPartner = id2;
        else if (v == id2) targetPartner = id1;
        else continue;

        const auto& nodesToCheck = (type == DynamicConstraint::Type::ShareRow) ?
                                    nodesByRow[r] : nodesByColumn[c];

        for (const NodeID neighbor_node : nodesToCheck) {
            if (neighbor_node != u && assignment[neighbor_node] == targetPartner) {
                return false;
            }
        }
    }

    return true;
}

bool GridShuffler::isForbidden(const int u, const int v) const {
    const int idx = u * dim + v;
    return idx < forbiddenAdjMatrix.size()? forbiddenAdjMatrix.test(idx) : false;
}

bool GridShuffler::forwardCheck(const NodeID assignedNode, ValueID assignedValue, const AssignmentType& assignment, const std::vector<bool>& visited, GridOf<bool>& localDomainMask) {
    std::vector<std::pair<NodeID, ValueID>> removedValues;

    for (const auto neighbor : graph[assignedNode]) {
        if (assignment[neighbor].has_value()) continue;

        if (localDomainMask[neighbor][assignedValue]) {
            localDomainMask[neighbor][assignedValue] = false;
            removedValues.emplace_back(neighbor, assignedValue);

            bool hasAnyCandidate = false;
            for (const auto v : std::views::iota(0, dim)) {
                if (!visited[v] && localDomainMask[neighbor][v]) {
                    hasAnyCandidate = true;
                    break;
                }
            }
            if (!hasAnyCandidate) {
                for (const auto& [node, val] : removedValues) {
                    localDomainMask[node][val] = true;
                }
                return false;
            }
        }
    }
    return true;
}

bool GridShuffler::solve(std::vector<std::optional<ValueID>>& assignment, std::vector<bool>& visited, GridOf<bool>& localDomainMask) {
    int u = -1;
    int minDomainSize = std::numeric_limits<int>::max();
    int maxDegree = -1;

    for (const auto i : std::views::iota(0, numItems)) {
        if (assignment[i].has_value()) continue;

        int domainCount = 0;
        for (const auto v : std::views::iota(0, dim)) {
            if (!visited[v] && localDomainMask[i][v]) {
                domainCount++;
            }
        }

        if (domainCount < minDomainSize ||
            (domainCount == minDomainSize && graph[i].size() > maxDegree)) {
            minDomainSize = domainCount;
            maxDegree = graph[i].size();
            u = i;
        }
    }

    if (u == -1) return true;

    thread_local std::vector<int> candidates;
    candidates.clear();

    for (const auto v : std::views::iota(0, dim)) {
        if (visited[v] || !localDomainMask[u][v]) continue;

        if (!config.allow_fixed_points && originalValueAtNode[u] == v) continue;

        if (!checkDynamicConstraints(u, v, assignment)) continue;

        if (std::ranges::none_of(graph[u], [&](const auto neighbor) {
            const auto neighbor_assignment = assignment[neighbor];
            return neighbor_assignment.has_value() && isForbidden(v, *neighbor_assignment);
        })) {
            candidates.push_back(v);
        }
    }

    if (candidates.empty()) return false;

    std::ranges::shuffle(candidates, rng);

    for (const auto val : candidates) {
        assignment[u] = val;
        visited[val] = true;

        if (!forwardCheck(u, val, assignment, visited, localDomainMask)) {
            assignment[u] = std::nullopt;
            visited[val] = false;
            continue;
        }

        if (solve(assignment, visited, localDomainMask)) return true;

        assignment[u] = std::nullopt;
        visited[val] = false;

        for (const auto neighbor : graph[u]) {
            if (!assignment[neighbor].has_value()) {
                localDomainMask[neighbor][val] = true;
            }
        }
    }

    return false;
}
