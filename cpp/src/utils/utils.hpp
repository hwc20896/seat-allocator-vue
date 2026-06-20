#pragma once

#include "utiltypes.hpp"
#include <regex>

/**
 * @brief Reads an XLSX file and converts it into a 2D grid structure.
 *
 * @param filePath Path to the XLSX file to read.
 * @return Grid A 2D vector of strings representing the XLSX content.
 *
 * @note Uses OpenXLSX library. Reads from the first worksheet.
 *       Empty cells are represented as empty strings "".
 *       Cell values are converted to strings.
 */
Grid readXLSX(const std::string& filePath);

/**
 * @brief Writes a 2D grid structure to an XLSX file.
 *
 * @param filePath Path to the XLSX file to write.
 * @param grid The 2D vector of strings to write.
 *
 * @note Uses OpenXLSX library. Creates/overwrites the file.
 *       Each Row in the grid becomes a row in the worksheet.
 *       Grid indices are offset by 1 (Excel uses 1-based indexing).
 */
void writeXLSX(const std::string& filePath, const Grid& grid);

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
