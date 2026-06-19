#include "utils.hpp"

#include <sstream>
#include <fstream>
#include <ranges>
#include <random>
#include <algorithm>
#include <spdlog/spdlog.h>
#include <filesystem>
#include <OpenXLSX.hpp>

Grid readCSV(const std::string& filePath) {
    Grid grid;
    std::ifstream file(filePath);

    if (!file.is_open()) {
        spdlog::error("Cannot open file {}", filePath);
        return grid;
    }

    std::string line;
    while (std::getline(file, line)) {
        Row row;
        std::stringstream ss(line);
        std::string cell;

        while (std::getline(ss, cell, ',')) {
            // Trim whitespace if needed
            size_t start = cell.find_first_not_of(" \t\r\n");

            if (size_t end = cell.find_last_not_of(" \t\r\n");
                start != std::string::npos && end != std::string::npos
            )
                cell = cell.substr(start, end - start + 1);
            else if (start == std::string::npos)
                cell = "";

            row.push_back(cell);
        }

        // Handle trailing comma (empty last cell)
        if (!line.empty() && line.back() == ',')
            row.emplace_back("");

        grid.push_back(row);
    }

    return grid;
}

void shuffleGrid(Grid& grid) {
    auto nonEmptyCells = grid | std::views::join | std::views::filter([](const std::string& cell) {return !cell.empty();}) | std::ranges::to<Row>();

    thread_local std::mt19937 mt{std::random_device{}()};

    std::ranges::shuffle(nonEmptyCells, mt);

    std::ranges::copy(nonEmptyCells, (
        grid | std::views::join | std::views::filter([](const std::string& cell) {return !cell.empty();})
    ).begin());
}

void writeCSV(const std::string& filePath, const Grid& grid) {
    std::ofstream file(filePath);

    if (!file.is_open()) {
        spdlog::error("Cannot open file {}", filePath);
        return;
    }

    for (const auto& row : grid) {
        for (size_t i = 0; i < row.size(); ++i) {
            file << row[i];
            if (i != row.size() - 1)
                file << ',';
        }
        file << '\n';
    }
}

Grid readXLSX(const std::string& filePath) {
    Grid result;

    OpenXLSX::XLDocument doc;
    doc.open(filePath);
    const auto wks = doc.workbook().worksheet(1);

    const auto [rows, cols] = std::make_pair(wks.rowCount(), wks.columnCount());

    for (auto r = 1; r <= rows; ++r) {
        Row rowVec;
        for (auto c = 1; c <= cols; ++c) {
            if (auto cell = wks.cell(r, c);
                cell.value().type() == OpenXLSX::XLValueType::Empty
            ) {
                rowVec.emplace_back("");
            } else {
                rowVec.push_back(cell.value().get<std::string>());
            }
        }
        result.push_back(rowVec);
    }

    doc.close();
    return result;
}

void writeXLSX(const std::string& filePath, const Grid& grid) {
    OpenXLSX::XLDocument doc;
    doc.create(filePath, true);
    const auto wks = doc.workbook().worksheet("Sheet1");

    for (size_t i = 0; i < grid.size(); ++i) {
        for (size_t j = 0; j < grid[i].size(); ++j) {
            wks.cell(i + 1, j + 1).value() = grid[i][j];
        }
    }

    doc.save();
    doc.close();
}

std::string getFileBasename(const std::string& filePath) {
    return std::filesystem::path(filePath).filename().string();
}

std::string getFileContent(const std::string& filePath) {
    std::ifstream file(filePath);
    return {std::istreambuf_iterator(file), std::istreambuf_iterator<char>()};
}
