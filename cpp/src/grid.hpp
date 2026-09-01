#pragma once

#include <vector>
#include <string>
#include <stdexcept>
#include <ostream>
#include <algorithm>

class Grid final {
    public /* Statics */:
        static Grid fromCSVString(const std::string& csvString);

    public /* Methods */:
        Grid() = default;

        explicit Grid(int row, int col);

        explicit Grid(int row, int col, std::vector<std::string> data);

        ~Grid() = default;

        [[nodiscard]]
        const std::string& operator[](int row, int col) const;

        [[nodiscard]]
        const std::string& operator[](int index) const;

        std::string& operator[](int row, int col);

        std::string& operator[](int index);

        [[nodiscard]]
        const std::string& get(int row, int col) const;

        [[nodiscard]]
        const std::string& get(int index) const;

        void set(int row, int col, std::string value);

        void set(int index, std::string value);

        [[nodiscard]]
        int rowCount() const noexcept;

        [[nodiscard]]
        int colCount() const noexcept;

        [[nodiscard]]
        size_t size() const noexcept;

        [[nodiscard]]
        bool empty() const noexcept;

        [[nodiscard]]
        const std::vector<std::string>& rawData() const noexcept;

        auto begin() noexcept { return data_.begin(); }
        auto end() noexcept { return data_.end(); }

        [[nodiscard]]
        auto begin() const noexcept { return data_.begin(); }

        [[nodiscard]]
        auto end() const noexcept { return data_.end(); }

        [[nodiscard]]
        Grid clone() const noexcept;

        [[nodiscard]]
        std::string toCSVString() const;

        constexpr auto operator<=>(const Grid&) const = default;

        friend std::ostream& operator<<(std::ostream& os, const Grid& grid);
    private:
        int rows_ = 0;
        int cols_ = 0;

        std::vector<std::string> data_;
};

Grid Grid::fromCSVString(const std::string& csvString) {
    if (csvString.empty()) return {};

    std::vector<std::string> data;
    int cols = -1;
    int rows = 0;

    std::vector<std::string> row;
    std::string field;
    bool in_quotes = false;
    size_t i = 0;
    const size_t len = csvString.size();

    while (i < len) {
        const char c = csvString[i];

        if (in_quotes) {
            if (c == '"') {
                if (i + 1 < len && csvString[i + 1] == '"') {
                    field += '"';
                    i += 2;
                } else {
                    in_quotes = false;
                    i++;
                }
            } else {
                field += c;
                i++;
            }
        } else {
            if (c == '"') {
                in_quotes = true;
                i++;
            } else if (c == ',') {
                row.push_back(std::move(field));
                field.clear();
                i++;
            } else if (c == '\r' || c == '\n') {
                if (row.empty() && field.empty()) {
                    i++;
                    continue;
                }

                row.push_back(std::move(field));
                field.clear();

                if (cols == -1) {
                    cols = static_cast<int>(row.size());
                } else if (static_cast<int>(row.size()) != cols) {
                    throw std::invalid_argument(
                        "Grid::fromCSVString: inconsistent column count."
                    );
                }

#ifdef KEEP_DEBUG_NOTE
                std::println("row: {}", row);
#endif

#if __cpp_lib_containers_ranges >= 202202L
                data.append_range(std::move(row));
#else
                data.insert(data.end(), std::make_move_iterator(row.begin()), std::make_move_iterator(row.end()));
#endif
                row.clear();
                rows++;
                i++;
            } else {
                field += c;
                i++;
            }
        }
    }

    bool has_trailing_comma = (len > 0 && csvString[len - 1] == ',');
    if (!row.empty() || !field.empty() || has_trailing_comma) {
        row.push_back(std::move(field));

        if (cols == -1) {
            cols = static_cast<int>(row.size());
        } else if (static_cast<int>(row.size()) != cols) {
            throw std::invalid_argument(
                "Grid::fromCSVString: inconsistent column count."
            );
        }

#if __cpp_lib_containers_ranges >= 202202L
        data.append_range(std::move(row));
#else
        data.insert(data.end(), std::make_move_iterator(row.begin()), std::make_move_iterator(row.end()));
#endif
        rows++;
    }

    if (rows == 0) return {};
    return Grid(rows, cols, std::move(data));
}

Grid::Grid(const int row, const int col)
    : rows_(row), cols_(col), data_(static_cast<size_t>(row) * col) {}

Grid::Grid(const int row, const int col, std::vector<std::string> data)
    : rows_(row), cols_(col), data_(std::move(data))
{
    if (data_.size() != static_cast<size_t>(rows_) * cols_) {
        throw std::invalid_argument(
            "Grid size mismatch"
        );
    }
}

const std::string& Grid::operator[](const int row, const int col) const {
    if (row < 0 || row >= rows_ || col < 0 || col >= cols_) {
        throw std::out_of_range("Grid: Index Out of range");
    }
    return data_[row * cols_ + col];
}

const std::string& Grid::operator[](const int index) const {
    if (index < 0 || index >= this->size())
        throw std::out_of_range("Grid: Index Out of range");
    return data_[index];
}

std::string& Grid::operator[](const int row, const int col) {
    if (row < 0 || row >= rows_ || col < 0 || col >= cols_) {
        throw std::out_of_range("Grid: Index Out of range");
    }
    return data_[row * cols_ + col];
}

std::string& Grid::operator[](const int index) {
    if (index < 0 || index >= this->size())
        throw std::out_of_range("Grid: Index Out of range");
    return data_[index];
}

const std::string& Grid::get(const int row, const int col) const {
    return (*this)[row, col];
}

const std::string& Grid::get(const int index) const {
    return (*this)[index];
}

void Grid::set(const int row, const int col, std::string value) {
    (*this)[row, col] = std::move(value);
}

void Grid::set(const int index, std::string value) {
    (*this)[index] = std::move(value);
}

int Grid::rowCount() const noexcept {
    return rows_;
}

int Grid::colCount() const noexcept {
    return cols_;
}

size_t Grid::size() const noexcept {
    return data_.size();
}

bool Grid::empty() const noexcept {
    return data_.empty();
}

const std::vector<std::string>& Grid::rawData() const noexcept {
    return data_;
}

Grid Grid::clone() const noexcept {
    return *this;
}

std::string Grid::toCSVString() const {
    if (data_.empty())
        return "";

    std::string result;
    for (int r = 0; r < rows_; ++r) {
        for (int c = 0; c < cols_; ++c) {
            const std::string& cell = (*this)[r, c];

            if (std::ranges::any_of(
                std::array{',', '"', '\n'},
                [&cell](const char ch){return cell.contains(ch);})  //  needs quoting
            ) {
                result += '"';
                for (const char ch : cell) {
                    if (ch == '"') result += "\"\"";
                    else result += ch;
                }
                result += '"';
            } else {
                result += cell;
            }

            if (c + 1 < cols_) result += ',';
        }
        result += '\n';
    }
    return result;
}

std::ostream& operator<<(std::ostream& os, const Grid& grid) {
    if (grid.empty()) {
        return os << "[Empty Grid]\n";
    }

    std::vector<size_t> colWidths(grid.cols_, 0);
    for (int r = 0; r < grid.rows_; ++r) {
        for (int c = 0; c < grid.cols_; ++c) {
            colWidths[c] = std::max(colWidths[c], grid[r, c].size());
        }
    }

    for (int r = 0; r < grid.rows_; ++r) {
        for (int c = 0; c < grid.cols_; ++c) {
            os << grid[r, c];
            if (c + 1 < grid.cols_) {
                const size_t padding = colWidths[c] - grid[r, c].size() + 2;
                os << std::string(padding, ' ');
            }
        }
        os << "\n";
    }
    return os;
}
