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
