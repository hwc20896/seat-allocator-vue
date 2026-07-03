module;

#include <algorithm>
#include <unordered_map>
#include <random>
#include <optional>
#include <utility>
#include <chrono>
#include <format>
#include <ranges>
#include <limits>
#include <array>
#include <future>

#include <spdlog/spdlog.h>
#include <emscripten.h>

export module Algorithm.Shuffler;

import Algorithm.Constraints;
import Algorithm.Utils;
import Algorithm.DynamicBitset;

export class GridShuffler final {
    public:
        /**
         * @brief Constructs a GridShuffler with optional configuration.
         * @param config Configuration options controlling shuffle behavior.
         */
        explicit GridShuffler(ShuffleConfig config = ShuffleConfig());

        /**
         * @brief Returns the number of successfully generated shuffled grids.
         * @return The count of shuffled grids stored internally.
         */
        [[nodiscard]]
        size_t getSize() const noexcept;

        /**
         * @brief Sets the input grid and initializes internal data structures.
         * @param grid The 2D grid containing string elements to shuffle.
         * @return true if grid was set successfully, false if grid is empty or invalid.
         * @throws std::invalid_argument if duplicate elements are found or constraints are unsatisfiable.
         */
        bool setGrid(const Grid& grid);

        [[nodiscard]]
        const Grid& getOriginalGrid() const;

        /**
         * @brief Returns the most recently generated shuffled grid.
         * @return Reference to the last shuffled grid, or the original grid if no shuffles exist.
         */
        [[nodiscard]]
        const Grid& getGrid() const;

        /**
         * @brief Generates a new shuffled grid respecting all constraints and stores it.
         * @throws std::invalid_argument if available values are insufficient.
         * @throws std::runtime_error if no valid shuffle is found within maximum attempts.
         */
        void shuffle();

        /**
         * @brief Validates the current shuffled grid against all constraints.
         * @return true if the current grid satisfies all constraint rules, false otherwise.
         */
        bool validateResult();

        /**
         * @brief Clears all previously generated shuffled grids from memory.
         */
        void clearShuffledGrids();

        /**
         * @brief Returns all generated shuffled grids.
         * @return Constant reference to the vector containing all shuffled grids.
         */
        [[nodiscard]]
        const ArrayOf<Grid>& getAllGrids() const;

        /**
         * @brief Array subscript operator to access a specific shuffled grid by index.
         * @param index The zero-based index of the desired shuffled grid.
         * @return Reference to the requested grid, or the last grid if index is out of range.
         */
        [[nodiscard]]
        const Grid& operator[](int index) const;

        /**
         * @brief Returns a specific shuffled grid by index.
         * @param index The zero-based index of the desired shuffled grid.
         * @return Reference to the requested grid, or the last grid if index is out of range.
         */
        [[nodiscard]]
        const Grid& getGrid(int index) const;

    private /*  variables  */:
        using AssignmentType = ArrayOf<std::optional<ValueID>>;

        size_t rowCount, columnCount;
        Grid originalGrid;
        ArrayOf<Grid> data;

        ShuffleConfig config;

        ArrayOf<Position> nodeToPos;
        ArrayOf<DataType> idToString;
        std::unordered_map<DataType, ValueID> stringToID;
        Graph graph;

        DynamicBitset forbiddenAdjMatrix;
        ArrayOf<ValueID> originalValueAtNode;

        int numItems;
        int dim;
        GridOf<bool> domainMask;

        ArrayOf<Position> dirs;
        Graph nodesByRow, nodesByColumn;

        struct DynamicConstraint {
            enum class Type {ShareCol, ShareRow} type;
            ValueID id1, id2;
        };

        ArrayOf<DynamicConstraint> preparedDynamicConstraints;

        mutable std::mt19937 rng;

        static constexpr auto MAX_ATTEMPTS = 1000;

    private /*  methods  */:
        /**
         * @brief Initializes the graph topology representing grid adjacency relationships.
         * Maps grid positions to node IDs and builds neighbor lists based on configured directions.
         */
        void initTopology();

        /**
         * @brief Initializes constraint data including value mappings and forbidden adjacency matrix.
         * Processes original neighbors and custom forbidden pairs from configuration.
         * @throws std::invalid_argument if duplicate elements exist or constraints are invalid.
         */
        void initConstraints();

        /**
         * @brief Initializes domain masks for each node based on static constraints.
         * Applies ForceRow/ForceCol and ForbidRow/ForbidCol constraints to restrict valid assignments.
         * @throws std::invalid_argument if constraint references invalid rows or columns.
         */
        void initDomains();

        /**
         * @brief Checks dynamic constraints for a candidate assignment during solving.
         * @param u The node ID being assigned.
         * @param v The value ID being considered.
         * @param assignment Current partial assignment state.
         * @return true if the assignment satisfies all dynamic constraints, false otherwise.
         */
        bool checkDynamicConstraints(NodeID u, ValueID v, const AssignmentType& assignment) const;

        /**
         * @brief Checks if two values are forbidden to be adjacent.
         * @param u First value ID.
         * @param v Second value ID.
         * @return true if the pair (u, v) is forbidden, false otherwise.
         */
        [[nodiscard]]
        bool isForbidden(int u, int v) const;

        /**
         * @brief Recursive backtracking solver to find a valid assignment.
         * Uses MRV heuristic (Most Constrained Variable) to select next node.
         * @param assignment Current assignment state to complete.
         * @param visited Tracks which values have been used.
         * @param localDomainMask IDK bro. I also want to know what this does.
         * @return true if a complete valid assignment was found, false otherwise.
         */
        bool solve(AssignmentType& assignment, ArrayOf<bool>& visited, GridOf<bool>& localDomainMask);

        bool forwardCheck(
            NodeID assignedNode,
            ValueID assignedValue,
            const AssignmentType& assignment,
            const ArrayOf<bool>& visited,
            GridOf<bool>& localDomainMask
        );
};

module :private;

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

    auto assignment = std::vector<std::optional<int>>(numItems, std::nullopt);
    auto usedValues = std::vector(dim, false);
    auto localDomainMask = domainMask;
    bool found = false;

    // Run total attempts sequentially
    for (const auto attemptCount : std::views::iota(0, MAX_ATTEMPTS)) {
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

#if defined(__EMSCRIPTEN__) && !defined(__EMSCRIPTEN_PTHREADS__)
        // Yield to the event loop in single-threaded Emscripten to avoid blocking
        static_assert(MAX_ATTEMPTS > 0);
        if ((attemptCount+1) % 5 == 0) {
            emscripten_sleep(0);
        }
#endif
    }

    if (found) {
        
        const auto endTime = high_resolution_clock::now();
        const auto duration = duration_cast<microseconds>(endTime - startTime);
        spdlog::info("[OK] Shuffle successful (took {:.3f}ms).", duration.count()/1000.0);
        return;
    }

    const auto endTime = high_resolution_clock::now();
    const auto duration = duration_cast<milliseconds>(endTime - startTime);

    static const auto msg = std::format(
        "[FAIL] Shuffle failed after {} attempts (took {} ms). Constraints may be unsatisfiable.",
        MAX_ATTEMPTS,
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

    nodesByRow = GridOf<NodeID>(rowCount);
    nodesByColumn = GridOf<NodeID>(columnCount);
    for (const auto u : std::views::iota(0, numItems)) {
        const auto [r, c] = nodeToPos[u];
        nodesByRow[r].push_back(u);
        nodesByColumn[c].push_back(u);
    }

    graph = GridOf<NodeID>(numItems);

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
            constexpr bool constraintRow = std::is_same_v<ConstraintType, ForceRow> || std::is_same_v<ConstraintType,  ForbidRow>;
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

        const auto& nodesToCheck = type == DynamicConstraint::Type::ShareRow ? nodesByRow[r] : nodesByColumn[c];

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
