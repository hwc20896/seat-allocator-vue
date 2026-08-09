#include <emscripten/emscripten.h>
#include <emscripten/bind.h>
#include <string>
#include <vector>

#include "src/constraints.hpp"
#include "src/shuffler.hpp"
#include "src/utils.hpp"
#include "src/grid.hpp"
#include "src/configs.hpp"

using namespace emscripten;

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
    class_<ShuffleConfig>("ShuffleConfig")
        .constructor<>()
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
    value_object<AnnealingConfig>("AnnealingConfig")
        .field("initialTemperature", &AnnealingConfig::initialTemperature)
        .field("coolingRate", &AnnealingConfig::coolingRate)
        .field("maxSteps", &AnnealingConfig::maxSteps)
        .field("maxAttempts", &AnnealingConfig::maxAttempts);

    value_object<PenaltyWeights>("PenaltyWeights")
        .field("fixedPoint", &PenaltyWeights::fixedPoint)
        .field("absolutePosition", &PenaltyWeights::absolutePosition)
        .field("originalNeighbor", &PenaltyWeights::originalNeighbor)
        .field("customForbidden", &PenaltyWeights::customForbidden)
        .field("forbidShare", &PenaltyWeights::forbidShare);

    enum_<ShuffleError>("ShuffleError", enum_value_type::string)
        .value("EmptyGrid", ShuffleError::EmptyGrid)
        .value("MaxAttemptsReached", ShuffleError::MaxAttemptsReached);

    class_<Grid>("Grid")
        .constructor<>()
        .constructor<int, int>()
        .constructor(+[](const int rows, const int cols, const val& jsArray) {
            std::vector<std::string> data;

            const auto len = jsArray["length"].as<unsigned>();
            data.reserve(len);

            for (unsigned i = 0; i < len; ++i) {
                data.push_back(jsArray[i].as<std::string>());
            }

            return std::make_unique<Grid>(rows, cols, std::move(data));
        })
        .function("getByPos", select_overload<const std::string& (int, int) const>(&Grid::get))
        .function("getByIndex", select_overload<const std::string& (int) const>(&Grid::get))
        .function("setByPos", select_overload<void (int, int, std::string)>(&Grid::set))
        .function("setByIndex", select_overload<void (int, std::string)>(&Grid::set))
        .function("rowCount", &Grid::rowCount)
        .function("colCount", &Grid::colCount)
        .function("size", &Grid::size)
        .function("empty", &Grid::empty)
        .function("rawData", optional_override([](const Grid& self) {
            val js_array = val::array();
            for (const auto& str : self.rawData()) {
                js_array.call<void>("push", str);
            }
            return js_array;
        }))
        .function("clone", &Grid::clone)
        .function("toCSVString", &Grid::toCSVString)
        .class_function("fromCSV", &Grid::fromCSVString);

    class_<GridShuffler>("GridShuffler")
        .constructor<>()
        .function("getShuffledGridCount", &GridShuffler::getShuffledGridCount)
        .function("setGrid", &GridShuffler::setGrid, allow_raw_pointers())
        .function("setConfig", &GridShuffler::setConfig, allow_raw_pointers())
        .function("setAnnealingConfig", &GridShuffler::setAnnealingConfig, allow_raw_pointers())
        .function("setPenaltyWeights", &GridShuffler::setPenaltyWeights)
        .function("getOriginalGrid", &GridShuffler::getOriginalGrid)
        .function("getGrid", select_overload<const Grid& () const noexcept>(&GridShuffler::getGrid))
        .function("getGridAt", select_overload<const Grid& () const>(&GridShuffler::getGrid))
        .function("shuffle", optional_override([](GridShuffler& self) {
            auto res = self.shuffle();

            val js_obj = val::object();
            if (res.has_value()) {
                js_obj.set("success", true);

                val data_obj = val::object();
                data_obj.set("tookMUS", static_cast<double>(res->tookMUS));  // μs
                data_obj.set("doneAtAttempt", res->doneAtAttempt);
                data_obj.set("doneAtStep", res->doneAtStep);

                js_obj.set("data", data_obj);
            } else {
                js_obj.set("success", false);
                js_obj.set("error", res.error());
            }
            return js_obj;
        }))
        .function("validateResult", &GridShuffler::validateResult)
        .function("clearShuffledGrids", &GridShuffler::clearShuffledGrids);
}