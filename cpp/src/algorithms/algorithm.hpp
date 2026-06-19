#pragma once

#include <vector>
#include <algorithm>
#include <unordered_map>
#include <string>
#include <optional>
#include <random>
#include "constraints.hpp"
#include "utils/utiltypes.hpp"
#include "utils/dynamic_bitset.hpp"

/**
 * @brief A class that shuffles elements in a 2D grid while respecting adjacency constraints.
 *
 * GridShuffler rearranges non-empty elements in a grid such that:
 * - Original neighboring pairs can be optionally forbidden
 * - Custom forbidden pairs can be specified
 * - Fixed points (elements staying in original positions) can be allowed or forbidden
 * - Various row/column constraints can be applied
 */
class GridShuffler final {
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
        using AssignmentType = ArrayOf<NullableTypeOf<ValueID>>;

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
