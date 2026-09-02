#pragma once

#include <vector>
#include <string>

template <class T>
using ArrayOf = std::vector<T>;

template <class T>
using GridOf = std::vector<std::vector<T>>;

using Position = std::pair<int, int>;
using StringPair = std::pair<std::string, std::string>;

using NodeID = int;
using ValueID = int;
using DataType = std::string;

using Graph = GridOf<NodeID>;

using Row = ArrayOf<DataType>;

template <class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };

#define DEFAULT_COPY(cls)\
    cls(const cls&) = default;\
    cls& operator=(const cls&) = default;

#define CONSTEXPR_DEFAULT_COPY(cls)\
    constexpr cls(const cls&) = default;\
    constexpr cls& operator=(const cls&) = default;

#define DEFAULT_MOVE(cls)\
    cls(cls&&) = default;\
    cls& operator=(cls&&) = default;

#define CONSTEXPR_DEFAULT_MOVE(cls)\
    constexpr cls(cls&&) = default;\
    constexpr cls& operator=(cls&&) = default;

#define CONSTEXPR_DEFAULT_THREE_WAY(cls)\
    constexpr auto operator<=>(const cls&) const noexcept = default;

#define CONSTEXPR_DEFAULT_EQUALITY(cls)\
    constexpr bool operator==(const cls&) const noexcept = default;
