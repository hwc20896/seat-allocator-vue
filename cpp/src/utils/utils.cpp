#include "utils.hpp"

#include <ranges>
#include <random>
#include <algorithm>

Grid shuffleGrid(Grid& grid) {
    auto nonEmptyCells = grid | std::views::join | std::views::filter([](const std::string& cell) {return !cell.empty();}) | std::ranges::to<Row>();

    thread_local std::mt19937 mt{std::random_device{}()};

    std::ranges::shuffle(nonEmptyCells, mt);

    std::ranges::copy(nonEmptyCells, (
        grid | std::views::join | std::views::filter([](const std::string& cell) {return !cell.empty();})
    ).begin());

    return grid;
}
