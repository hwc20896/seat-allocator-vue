#pragma once

#include <vector>
#include <string>
#include "constraints.hpp"
#include "utils.hpp"

struct AnnealingConfig {
    double initialTemperature = 5.0;
    double coolingRate = 0.99995;
    int maxSteps = 2'000'000;
    int maxAttempts = 5;

    CONSTEXPR_DEFAULT_EQUALITY(AnnealingConfig)
};

struct PenaltyWeights {
    int fixedPoint = 1000;
    int absolutePosition = 2000;
    int originalNeighbor = 10;
    int customForbidden = 1000;
    int forbidShare = 1000;

    CONSTEXPR_DEFAULT_EQUALITY(PenaltyWeights)
};

struct ShuffleConfig {
    bool allowFixedPoints = true;
    bool allowOriginalNeighbors = true;
    bool diagonalsAreNeighbors = false;
    std::vector<std::pair<std::string, std::string>> custom_forbidden_pairs;
    std::vector<Constraint> constraints;

    constexpr auto operator<=>(const ShuffleConfig&) const noexcept = default;

    CONSTEXPR_DEFAULT_EQUALITY(ShuffleConfig)

    constexpr ShuffleConfig() = default;

    CONSTEXPR_DEFAULT_COPY(ShuffleConfig)
    CONSTEXPR_DEFAULT_MOVE(ShuffleConfig)

    constexpr ShuffleConfig& setAllowFixedPoints(const bool _allow_fixed_points) {
        this->allowFixedPoints = _allow_fixed_points;
        return *this;
    }

    constexpr ShuffleConfig& setAllowOriginalNeighbors(const bool _allow_original_neighbors) {
        this->allowOriginalNeighbors = _allow_original_neighbors;
        return *this;
    }

    constexpr ShuffleConfig& setDiagonalsAreNeighbors(const bool _diagonals_are_neighbors) {
        this->diagonalsAreNeighbors = _diagonals_are_neighbors;
        return *this;
    }

    constexpr ShuffleConfig& addForbiddenPair(const std::string& name1, const std::string& name2) {
        custom_forbidden_pairs.emplace_back(name1, name2);
        return *this;
    }

    constexpr ShuffleConfig& forceRow(const std::string& val, const int row_idx) {
        constraints.emplace_back(ForceRow{val, row_idx});
        return *this;
    }

    constexpr ShuffleConfig& forbidRow(const std::string& val, const int row_idx) {
        constraints.emplace_back(ForbidRow{val, row_idx});
        return *this;
    }

    constexpr ShuffleConfig& forceCol(const std::string& val, const int col_idx) {
        constraints.emplace_back(ForceCol{val, col_idx});
        return *this;
    }

    constexpr ShuffleConfig& forbidCol(const std::string& val, const int col_idx) {
        constraints.emplace_back(ForbidCol{val, col_idx});
        return *this;
    }

    constexpr ShuffleConfig& forbidShareRow(const std::string& val1, const std::string& val2) {
        constraints.emplace_back(ForbidShareRow{val1, val2});
        return *this;
    }

    constexpr ShuffleConfig& forbidShareCol(const std::string& val1, const std::string& val2) {
        constraints.emplace_back(ForbidShareCol{val1, val2});
        return *this;
    }
};