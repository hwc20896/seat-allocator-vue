#pragma once

#include <vector>
#include <string>
#include <format>

#include <optional>

template <class T>
using ArrayOf = std::vector<T>;

template <class T>
using GridOf = std::vector<std::vector<T>>;

template <class T>
using NullableTypeOf = std::optional<T>;

using Position = std::pair<int, int>;
using NodeID = int;
using ValueID = int;
using DataType = std::string;

using Graph = GridOf<NodeID>;

using Row = ArrayOf<DataType>;
using Grid = GridOf<DataType>;

template <>
struct std::formatter<Grid> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    auto format(const Grid& v, format_context& ctx) const {
        auto out = ctx.out();
        out = std::format_to(ctx.out(), "[");

        for (size_t i = 0; i < v.size(); ++i) {
            if (i > 0) {
                out = std::format_to(out, ",");
            }

            out = std::format_to(out, "\n    [");
            for (size_t j = 0; j < v[i].size(); ++j) {
                out = std::format_to(out, "\"{}\"{}", v[i][j], (j == v[i].size() - 1 ? "" : ", "));
            }
            out = std::format_to(out, "]");
        }

        if (!v.empty()) {
            out = std::format_to(out, "\n");
        }
        return out = std::format_to(out, "]");
    }
};