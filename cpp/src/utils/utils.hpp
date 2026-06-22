#pragma once

#include "utiltypes.hpp"

/**
 * @brief Randomly shuffles all non-empty cells in the grid while preserving structure.
 *
 * @param grid Reference to the 2D vector of strings to shuffle.
 *
 * @note Only shuffles non-empty cells, maintaining the positions of empty cells.
 *       Uses std::mt19937 random number generator with std::random_device seed.
 *       Modifies the grid in-place.
 */
Grid shuffleGrid(Grid& grid);
