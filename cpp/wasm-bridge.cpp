#include <emscripten/emscripten.h>
#include <emscripten/bind.h>
#include <string>
#include <vector>

#include "src/constraints.hpp"
#include "src/shuffler.hpp"
#include "src/feasibility.hpp"
#include "src/grid.hpp"
#include "src/configs.hpp"

using namespace emscripten;

namespace {
struct ShuffleReport {
    bool success = false;
    int doneAtAttempt = 0;
    int doneAtStep = 0;
    double tookMUS = 0;
    ShuffleError error = ShuffleError::Unknown;
};
}

EMSCRIPTEN_BINDINGS(GridShufflerModule) {
    register_vector<std::string>("StringVector");
    register_vector<Grid>("GridVector");

    //  struct Constraints
    (void)
    value_object<ForceRow>("ForceRow")
        .field("first", &ForceRow::name)
        .field("second", &ForceRow::rowIdx);

    (void)
    value_object<ForbidRow>("ForbidRow")
        .field("first", &ForbidRow::name)
        .field("second", &ForbidRow::rowIdx);

    (void)
    value_object<ForceCol>("ForceCol")
        .field("first", &ForceCol::name)
        .field("second", &ForceCol::colIdx);

    (void)
    value_object<ForbidCol>("ForbidCol")
        .field("first", &ForbidCol::name)
        .field("second", &ForbidCol::colIdx);

    (void)
    value_object<ForbidShareRow>("ForbidShareRow")
        .field("first", &ForbidShareRow::name1)
        .field("second", &ForbidShareRow::name2);

    (void)
    value_object<ForbidShareCol>("ForbidShareCol")
        .field("first", &ForbidShareCol::name1)
        .field("second", &ForbidShareCol::name2);

    //  struct ShuffleConfig
    (void)
    class_<ShuffleConfig>("ShuffleConfig")
        .constructor<>()
        .property("allowFixedPoints", &ShuffleConfig::allowFixedPoints)
        .property("allowOriginalNeighbors", &ShuffleConfig::allowOriginalNeighbors)
        .property("diagonalsAreNeighbors", &ShuffleConfig::diagonalsAreNeighbors)
        .property("crossAisleAreNeighbors", &ShuffleConfig::crossAisleAreNeighbors)
        .property("enableBuddyMatching", &ShuffleConfig::enableBuddyMatching)
        .function("setAllowFixedPoints", &ShuffleConfig::setAllowFixedPoints)
        .function("setAllowOriginalNeighbors", &ShuffleConfig::setAllowOriginalNeighbors)
        .function("setDiagonalsAreNeighbors", &ShuffleConfig::setDiagonalsAreNeighbors)
        .function("addForbiddenPair", &ShuffleConfig::addForbiddenPair)
        .function("forceRow", &ShuffleConfig::forceRow)
        .function("forbidRow", &ShuffleConfig::forbidRow)
        .function("forceCol", &ShuffleConfig::forceCol)
        .function("forbidCol", &ShuffleConfig::forbidCol)
        .function("forbidShareRow", &ShuffleConfig::forbidShareRow)
        .function("forbidShareCol", &ShuffleConfig::forbidShareCol)
        .function("setCrossAisleAreNeighbors", &ShuffleConfig::setCrossAisleAreNeighbors)
        .function("setEnableBuddyMatching", &ShuffleConfig::setEnableBuddyMatching)
        .function("setDoBuddyRotate", &ShuffleConfig::setDoBuddyRotate)
        .function("addBuddyPair", &ShuffleConfig::addBuddyPair)
        .function("setBuddyGroups", &ShuffleConfig::setBuddyGroups);

    //  class GridShuffler
    (void)
    value_object<AnnealingConfig>("AnnealingConfig")
        .field("initialTemperature", &AnnealingConfig::initialTemperature)
        .field("coolingRate", &AnnealingConfig::coolingRate)
        .field("maxSteps", &AnnealingConfig::maxSteps)
        .field("maxAttempts", &AnnealingConfig::maxAttempts);

    (void)
    value_object<PenaltyWeights>("PenaltyWeights")
        .field("fixedPoint", &PenaltyWeights::fixedPoint)
        .field("absolutePosition", &PenaltyWeights::absolutePosition)
        .field("originalNeighbor", &PenaltyWeights::originalNeighbor)
        .field("customForbidden", &PenaltyWeights::customForbidden)
        .field("forbidShare", &PenaltyWeights::forbidShare);

    (void)
    enum_<FeasibilityStatus>("FeasibilityStatus", enum_value_type::number)
        .value("Feasible", FeasibilityStatus::Feasible)
        .value("Unsatisfiable", FeasibilityStatus::Unsatisfiable)
        .value("Unknown", FeasibilityStatus::Unknown);

    (void)
    value_object<FeasibilityReport>("FeasibilityReport")
        .field("status", &FeasibilityReport::status)
        .field("layer", &FeasibilityReport::layer)
        .field("reason", &FeasibilityReport::reason);

    (void)
    enum_<ShuffleError>("ShuffleError", enum_value_type::string)
        .value("EmptyGrid", ShuffleError::EmptyGrid)
        .value("Unsatisfiable", ShuffleError::Unsatisfiable)
        .value("MaxAttemptsReached", ShuffleError::MaxAttemptsReached)
        .value("Unknown", ShuffleError::Unknown);

    (void)
    value_object<ShuffleReport>("ShuffleReport")
        .field("success", &ShuffleReport::success)
        .field("doneAtAttempt", &ShuffleReport::doneAtAttempt)
        .field("doneAtStep", &ShuffleReport::doneAtStep)
        .field("tookMUS", &ShuffleReport::tookMUS)
        .field("error", &ShuffleReport::error);

    (void)
    class_<Grid>("Grid")
        .constructor<>()
        .constructor<int, int>()
        .constructor<int, int, std::vector<std::string>>()
        .function("getByPos", select_overload<const std::string& (int, int) const>(&Grid::get))
        .function("getByIndex", select_overload<const std::string& (int) const>(&Grid::get))
        .function("setByPos", select_overload<void (int, int, std::string)>(&Grid::set))
        .function("setByIndex", select_overload<void (int, std::string)>(&Grid::set))
        .function("rowCount", &Grid::rowCount)
        .function("colCount", &Grid::colCount)
        .function("size", &Grid::size)
        .function("empty", &Grid::empty)
        .function("rawData", &Grid::rawData)
        .function("clone", &Grid::clone)
        .function("toCSVString", &Grid::toCSVString)
        .class_function("fromCSV", &Grid::fromCSVString);

    (void)
    class_<GridShuffler>("GridShuffler")
        .constructor<>()
        .function("getShuffledGridCount", &GridShuffler::getShuffledGridCount)
        .function("setGrid", &GridShuffler::setGrid)
        .function("setConfig", &GridShuffler::setConfig)
        .function("setAnnealingConfigFixed", select_overload<void (const AnnealingConfig&)>(&GridShuffler::setAnnealingConfig))
        .function("setAnnealingConfigDynamic", optional_override([](GridShuffler& self, const val& func) {
            self.setAnnealingConfig([func](const int gridSize) -> AnnealingConfig {
                return func(gridSize).as<AnnealingConfig>();
            });
        }))
        .function("setPenaltyWeights", &GridShuffler::setPenaltyWeights)
        .function("getOriginalGrid", &GridShuffler::getOriginalGrid)
        .function("getGrid", select_overload<const Grid& () const noexcept>(&GridShuffler::getGrid))
        .function("getGridAt", select_overload<const Grid& (int) const>(&GridShuffler::getGrid))
        .function("shuffle", optional_override([](GridShuffler& self) {
            const auto res = self.shuffle();
            ShuffleReport report;
            if (res.has_value()) {
                report.success = true;
                report.tookMUS = res->tookMUS;
                report.doneAtAttempt = res->doneAtAttempt;
                report.doneAtStep = res->doneAtStep;
            } else {
                report.error = res.error();
            }
            return report;
        }))
        .function("validateResult", &GridShuffler::validateResult)
        .function("clearShuffledGrids", &GridShuffler::clearShuffledGrids);

    function("checkFeasibility", optional_override([](
        const Grid& grid, const ShuffleConfig& cfg,
        const bool checkForbidShare, const int coloringNodeBudget
    ) {
        return checkFeasibility(grid, cfg, {.checkForbidShare = checkForbidShare, .coloringNodeBudget = coloringNodeBudget});
    }));
}