module;

#include <variant>
#include <string>
#include <vector>

export module Algorithm.Constraints;

export struct ForceRow {
    std::string first;
    int second;
    constexpr auto operator<=>(const ForceRow&) const noexcept = default;
};

export struct ForbidRow {
    std::string first;
    int second;
    constexpr auto operator<=>(const ForbidRow&) const noexcept = default;
};

export struct ForceCol {
    std::string first;
    int second;
    constexpr auto operator<=>(const ForceCol&) const noexcept = default;
};

export struct ForbidCol {
    std::string first;
    int second;
    constexpr auto operator<=>(const ForbidCol&) const noexcept = default;
};

export struct ForbidShareRow {
    std::string first;
    std::string second;
    constexpr auto operator<=>(const ForbidShareRow&) const noexcept = default;
};

export struct ForbidShareCol {
    std::string first;
    std::string second;
    constexpr auto operator<=>(const ForbidShareCol&) const noexcept = default;
};

export using Constraint = std::variant<ForceRow, ForbidRow, ForceCol, ForbidCol, ForbidShareRow, ForbidShareCol>;