#include "utils.hpp"

#include <sstream>
#include <ranges>
#include <random>
#include <algorithm>
#include <spdlog/spdlog.h>
#include <OpenXLSX.hpp>

Grid shuffleGrid(Grid& grid) {
    auto nonEmptyCells = grid | std::views::join | std::views::filter([](const std::string& cell) {return !cell.empty();}) | std::ranges::to<Row>();

    thread_local std::mt19937 mt{std::random_device{}()};

    std::ranges::shuffle(nonEmptyCells, mt);

    std::ranges::copy(nonEmptyCells, (
        grid | std::views::join | std::views::filter([](const std::string& cell) {return !cell.empty();})
    ).begin());

    return grid;
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
