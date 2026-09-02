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

    //  ISSUE #6
    bool crossAisleAreNeighbors = true;
    bool enableBuddyMatching = false;
    std::pair<std::vector<std::string>, std::vector<std::string>> buddyGroups;

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

    constexpr ShuffleConfig& forceRow(const std::string& name, const int row_idx) {
        constraints.emplace_back(ForceRow{.name = name, .rowIdx = row_idx});
        return *this;
    }

    constexpr ShuffleConfig& forbidRow(const std::string& val, const int row_idx) {
        constraints.emplace_back(ForbidRow{.name = val, .rowIdx = row_idx});
        return *this;
    }

    constexpr ShuffleConfig& forceCol(const std::string& val, const int col_idx) {
        constraints.emplace_back(ForceCol{.name = val, .colIdx = col_idx});
        return *this;
    }

    constexpr ShuffleConfig& forbidCol(const std::string& val, const int col_idx) {
        constraints.emplace_back(ForbidCol{.name = val, .colIdx = col_idx});
        return *this;
    }

    constexpr ShuffleConfig& forbidShareRow(const std::string& val1, const std::string& val2) {
        constraints.emplace_back(ForbidShareRow{.name1 = val1, .name2 = val2});
        return *this;
    }

    constexpr ShuffleConfig& forbidShareCol(const std::string& val1, const std::string& val2) {
        constraints.emplace_back(ForbidShareCol{.name1 = val1, .name2 = val2});
        return *this;
    }

    //  ISSUE #6
    constexpr ShuffleConfig& setCrossAisleAreNeighbors(const bool _cross_aisle_are_neighbors) {
        this->crossAisleAreNeighbors = _cross_aisle_are_neighbors;
        return *this;
    }

    constexpr ShuffleConfig& setEnableBuddyMatching(const bool _enable_buddy_matching) {
        this->enableBuddyMatching = _enable_buddy_matching;
        return *this;
    }

    constexpr ShuffleConfig& setBuddyGroups(const std::vector<std::string>& group1, const std::vector<std::string>& group2) {
        const auto paired = std::make_pair(group1, group2);
        if (this->buddyGroups == paired) return *this;
        this->buddyGroups = paired;
        return *this;
    }

    constexpr ShuffleConfig& addBuddyPair(const std::string& name1, const std::string& name2) {
        this->buddyGroups.first.emplace_back(name1);
        this->buddyGroups.second.emplace_back(name2);
        return *this;
    }
};