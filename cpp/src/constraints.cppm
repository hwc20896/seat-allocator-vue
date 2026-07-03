module;

#include <variant>
#include <string>
#include <vector>

export module Algorithm.Constraints;

export struct ForceRow { std::string first; int second; };

export struct ForbidRow { std::string first; int second; };

export struct ForceCol { std::string first; int second; };

export struct ForbidCol { std::string first; int second; };

export struct ForbidShareRow { std::string first; std::string second; };

export struct ForbidShareCol { std::string first; std::string second; };

export using Constraint = std::variant<ForceRow, ForbidRow, ForceCol, ForbidCol, ForbidShareRow, ForbidShareCol>;

export struct ShuffleConfig {
    bool allow_fixed_points = false;
    bool allow_original_neighbors = false;
    bool diagonals_are_neighbors = false;
    std::vector<std::pair<std::string, std::string>> custom_forbidden_pairs;
    std::vector<Constraint> constraints;

    constexpr
    explicit ShuffleConfig() = default;

    constexpr
    void setAllowFixedPoints(const bool allow_fixed_points) {
        this->allow_fixed_points = allow_fixed_points;
    }

    constexpr
    void setAllowOriginalNeighbors(const bool allow_original_neighbors) {
        this->allow_original_neighbors = allow_original_neighbors;
    }

    constexpr
    void setDiagonalsAreNeighbors(const bool diagonals_are_neighbors) {
        this->diagonals_are_neighbors = diagonals_are_neighbors;
    }

    constexpr
    void addForbiddenPair(const std::string& a, const std::string& b) {
        custom_forbidden_pairs.emplace_back(a, b);
    }

    constexpr
    void forceRow(const std::string& val, const int row_idx) {
        constraints.emplace_back(ForceRow{val, row_idx});
    }

    constexpr
    void forbidRow(const std::string& val, const int row_idx) {
        constraints.emplace_back(ForbidRow{val, row_idx});
    }

    constexpr
    void forceCol(const std::string& val, const int col_idx) {
        constraints.emplace_back(ForceCol{val, col_idx});
    }

    constexpr
    void forbidCol(const std::string& val, const int col_idx) {
        constraints.emplace_back(ForbidCol{val, col_idx});
    }

    constexpr
    void forbidShareRow(const std::string& val1, const std::string& val2) {
        constraints.emplace_back(ForbidShareRow{val1, val2});
    }

    constexpr
    void forbidShareCol(const std::string& val1, const std::string& val2) {
        constraints.emplace_back(ForbidShareCol{val1, val2});
    }
};