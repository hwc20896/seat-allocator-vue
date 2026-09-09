#pragma once

#include <variant>
#include <string>
#include "utils.hpp"

struct ForceRow {
    std::string name;
    int rowIdx;
    CONSTEXPR_DEFAULT_EQUALITY(ForceRow)
};

struct ForbidRow {
    std::string name;
    int rowIdx;
    CONSTEXPR_DEFAULT_EQUALITY(ForbidRow)
};

struct ForceCol {
    std::string name;
    int colIdx;
    CONSTEXPR_DEFAULT_EQUALITY(ForceCol)
};

struct ForbidCol {
    std::string name;
    int colIdx;
    CONSTEXPR_DEFAULT_EQUALITY(ForbidCol)
};

struct ForbidShareRow {
    std::string name1;
    std::string name2;
    CONSTEXPR_DEFAULT_EQUALITY(ForbidShareRow)
};

struct ForbidShareCol {
    std::string name1;
    std::string name2;
    CONSTEXPR_DEFAULT_EQUALITY(ForbidShareCol)
};

using Constraint = std::variant<ForceRow, ForbidRow, ForceCol, ForbidCol, ForbidShareRow, ForbidShareCol>;
