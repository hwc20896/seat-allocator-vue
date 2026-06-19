#pragma once

#include "utiltypes.hpp"
#include <unordered_map>
#include <regex>
#include <utility>

/**
 * @brief Extracts the filename with extension from a full file path.
 *
 * @param filePath The full file path.
 * @return std::string The filename with extension (without directory path).
 *
 * @note Uses std::filesystem to parse the path.
 */
std::string getFileBasename(const std::string& filePath);

/**
 * @brief Reads the entire content of a file into a string.
 *
 * @param filePath Path to the file to read.
 * @return std::string The complete file content.
 *
 * @note Uses istreambuf_iterator for efficient reading.
 *       Returns empty string if file cannot be opened.
 */
std::string getFileContent(const std::string& filePath);

/**
 * @brief Reads a CSV file and converts it into a 2D grid structure.
 *
 * @param filePath Path to the CSV file to read.
 * @return Grid A 2D vector of strings representing the CSV content.
 *         Returns an empty grid if the file cannot be opened or is empty.
 *
 * @note Each row in the CSV becomes a Row (vector<string>) in the Grid.
 *       Empty cells are represented as empty strings "".
 *       Whitespace around values is trimmed.
 */
Grid readCSV(const std::string& filePath);

/**
 * @brief Writes a 2D grid structure to a CSV file.
 *
 * @param filePath Path to the CSV file to write.
 * @param grid The 2D vector of strings to write.
 *
 * @note Each Row in the grid becomes a line in the CSV.
 *       Cells are separated by commas. No trimming is performed.
 *       Logs an error if the file cannot be opened.
 */
void writeCSV(const std::string& filePath, const Grid& grid);

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
void shuffleGrid(Grid& grid);
