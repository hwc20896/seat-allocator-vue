#include <emscripten/emscripten.h>
#include <emscripten/bind.h>
#include <string>
#include <vector>

#include "src/algorithms/algorithm.hpp"
#include "src/algorithms/constraints.hpp"
#include "src/utils/utils.hpp"
#include "src/utils/utiltypes.hpp"

using namespace emscripten;

Grid js_array_to_cpp_grid(const val& js_grid) {
    Grid cpp_grid;
    const auto row_count = js_grid["length"].as<unsigned>();

    for (unsigned r = 0; r < row_count; ++r) {
        std::vector<std::string> cpp_row;
        val js_row = js_grid[r];
        const auto col_count = js_row["length"].as<unsigned>();

        cpp_row.reserve(col_count);
        for (unsigned c = 0; c < col_count; ++c) {
            cpp_row.push_back(js_row[c].as<std::string>());
        }
        cpp_grid.push_back(cpp_row);
    }
    return cpp_grid;
}

val cpp_grid_to_js_array(const Grid& cpp_grid) {
    val js_grid = val::array();
    for (const auto& row : cpp_grid) {
        val js_row = val::array();
        for (const auto& cell : row) {
            js_row.call<void>("push", cell);
        }
        js_grid.call<void>("push", js_row);
    }
    return js_grid;
}

EMSCRIPTEN_BINDINGS(GridShufflerModule) {
    register_vector<std::string>("StringVector");
    register_vector<Grid>("GridVector");
    register_vector<Constraint>("ConstraintVector");
    register_vector<std::pair<std::string, std::string>>("StringPairVector");

    //  struct Constraints
    value_object<ForceRow>("ForceRow")
        .field("first", &ForceRow::first)
        .field("second", &ForceRow::second);

    value_object<ForbidRow>("ForbidRow")
        .field("first", &ForbidRow::first)
        .field("second", &ForbidRow::second);

    value_object<ForceCol>("ForceCol")
        .field("first", &ForceCol::first)
        .field("second", &ForceCol::second);

    value_object<ForbidCol>("ForbidCol")
        .field("first", &ForbidCol::first)
        .field("second", &ForbidCol::second);

    value_object<ForbidShareRow>("ForbidShareRow")
        .field("first", &ForbidShareRow::first)
        .field("second", &ForbidShareRow::second);

    value_object<ForbidShareCol>("ForbidShareCol")
        .field("first", &ForbidShareCol::first)
        .field("second", &ForbidShareCol::second);

    //  struct ShuffleConfig
    class_<ShuffleConfig>("ShuffleConfig").constructor<>()
                                                .property("allow_fixed_points", &ShuffleConfig::allow_fixed_points)
                                                .property("allow_original_neighbors", &ShuffleConfig::allow_original_neighbors)
                                                .property("diagonals_are_neighbors", &ShuffleConfig::diagonals_are_neighbors)
                                                .property("custom_forbidden_pairs", &ShuffleConfig::custom_forbidden_pairs)
                                                .property("constraints", &ShuffleConfig::constraints)
                                                .function("setAllowFixedPoints", &ShuffleConfig::setAllowFixedPoints, allow_raw_pointers())
                                                .function("setAllowOriginalNeighbors", &ShuffleConfig::setAllowOriginalNeighbors, allow_raw_pointers())
                                                .function("setDiagonalsAreNeighbors", &ShuffleConfig::setDiagonalsAreNeighbors, allow_raw_pointers())
                                                .function("addForbiddenPair", &ShuffleConfig::addForbiddenPair, allow_raw_pointers())
                                                .function("forceRow", &ShuffleConfig::forceRow, allow_raw_pointers())
                                                .function("forbidRow", &ShuffleConfig::forbidRow, allow_raw_pointers())
                                                .function("forceCol", &ShuffleConfig::forceCol, allow_raw_pointers())
                                                .function("forbidCol", &ShuffleConfig::forbidCol, allow_raw_pointers())
                                                .function("forbidShareRow", &ShuffleConfig::forbidShareRow, allow_raw_pointers())
                                                .function("forbidShareCol", &ShuffleConfig::forbidShareCol, allow_raw_pointers());

    //  class GridShuffler
    class_<GridShuffler>("GridShuffler").constructor<>()
                                              .constructor<const ShuffleConfig&>()
                                              .function("getSize", &GridShuffler::getSize)
                                              .function("setGrid", optional_override([](GridShuffler& self, const val& js_grid) {
                                                  const auto cpp_grid = js_array_to_cpp_grid(js_grid);
                                                  return self.setGrid(cpp_grid);
                                              }))
                                              .function("getOriginalGrid", &GridShuffler::getOriginalGrid)
                                              .function("getGrid", optional_override([](const GridShuffler& self) {
                                                  return cpp_grid_to_js_array(self.getGrid());
                                              }))
                                              .function("getGridAt", optional_override([](const GridShuffler& self, int index) {
                                                  return cpp_grid_to_js_array(self.getGrid(index));
                                              }))
                                              .function("getGridByIndex", select_overload<const Grid&(int) const>(&GridShuffler::getGrid))
                                              .function("shuffle", &GridShuffler::shuffle)
                                              .function("validateResult", &GridShuffler::validateResult)
                                              .function("clearShuffledGrids", &GridShuffler::clearShuffledGrids)
                                              .function("getAllGrids", &GridShuffler::getAllGrids);

    function("shuffleGrid", optional_override([](const val& js_grid) {  //  from editing in place to returning the shuffled one
        auto cpp_grid = js_array_to_cpp_grid(js_grid);
        return cpp_grid_to_js_array(shuffleGrid(cpp_grid));
    }));
}