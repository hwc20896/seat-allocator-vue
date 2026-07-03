module;

#include <vector>
#include <string>
#include <ranges>
#include <random>
#include <algorithm>

export module Algorithm.Utils;

export
template <class T>
using ArrayOf = std::vector<T>;

export
template <class T>
using GridOf = std::vector<std::vector<T>>;

export using Position = std::pair<int, int>;

export using NodeID = int;
export using ValueID = int;
export using DataType = std::string;

export using Graph = GridOf<NodeID>;

export using Row = ArrayOf<DataType>;
export using Grid = GridOf<DataType>;