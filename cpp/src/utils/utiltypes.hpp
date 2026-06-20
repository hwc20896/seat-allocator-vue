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