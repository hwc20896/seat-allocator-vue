#pragma once

#include <variant>
#include <string>

struct ForceRow {
    std::string first;
    int second;
    constexpr auto operator<=>(const ForceRow&) const noexcept = default;
};

struct ForbidRow {
    std::string first;
    int second;
    constexpr auto operator<=>(const ForbidRow&) const noexcept = default;
};

struct ForceCol {
    std::string first;
    int second;
    constexpr auto operator<=>(const ForceCol&) const noexcept = default;
};

struct ForbidCol {
    std::string first;
    int second;
    constexpr auto operator<=>(const ForbidCol&) const noexcept = default;
};

struct ForbidShareRow {
    std::string first;
    std::string second;
    constexpr auto operator<=>(const ForbidShareRow&) const noexcept = default;
};

struct ForbidShareCol {
    std::string first;
    std::string second;
    constexpr auto operator<=>(const ForbidShareCol&) const noexcept = default;
};

using Constraint = std::variant<ForceRow, ForbidRow, ForceCol, ForbidCol, ForbidShareRow, ForbidShareCol>;
