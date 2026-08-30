#pragma once

#include <algorithm>
#include <unordered_map>
#include <random>
#include <utility>
#include <chrono>
#include <ranges>
#include <future>
#include <expected>
#include <numeric>

#ifdef __EMSCRIPTEN__
    #include <emscripten.h>
#endif

#include "configs.hpp"
#include "constraints.hpp"
#include "grid.hpp"
#include "dynamic-bitset.hpp"
#include "utils.hpp"

struct ResultType {
    int doneAtAttempt;
    int doneAtStep;
    int64_t tookMUS;
};

enum class ShuffleError : int {
    EmptyGrid,
    Unsatisfiable,
    MaxAttemptsReached
};

class GridShuffler final {
    public:
        GridShuffler();

        explicit GridShuffler(uint32_t seed);

        ~GridShuffler() = default;

        [[nodiscard]]
        size_t getShuffledGridCount() const noexcept;

        void setSeed(const uint32_t seed) const {rng = std::mt19937{seed};}

        bool setGrid(const Grid& grid);

        void setConfig(const ShuffleConfig& cfg);

        void setAnnealingConfig(const AnnealingConfig& cfg);

        void setPenaltyWeights(const PenaltyWeights& weights);

        [[nodiscard]]
        const Grid& getOriginalGrid() const noexcept;

        [[nodiscard]]
        const Grid& getGrid() const noexcept;

        [[nodiscard]]
        const Grid& getGrid(int index) const;

        std::expected<ResultType, ShuffleError> shuffle();

        [[nodiscard]]
        bool validateResult() const;

        void clearShuffledGrids();

        [[nodiscard]]
        const ArrayOf<Grid>& getAllGrids() const noexcept;

    private /* variables */:
        Grid originalGrid_;
        ArrayOf<Grid> shuffleGrids_;

        ShuffleConfig config_;
        AnnealingConfig annealingConfig_;
        PenaltyWeights penaltyWeights_;

        int gridRow_ = 0;
        int gridCol_ = 0;
        int gridSize_ = 0;

        std::unordered_map<DataType, int> stringToID_;
        ArrayOf<DataType> IDToString_;

        ArrayOf<NodeID> originalPos_;
        Graph neighborsOfPos;

        DynamicBitset originalNeighborsMatrix_;
        DynamicBitset customForbiddenMatrix_;

        ArrayOf<NodeID> forcedRow_;
        DynamicBitset forbiddenRowsMatrix_;
        ArrayOf<NodeID> forcedCol_;
        DynamicBitset forbiddenColsMatrix_;
        ArrayOf<ArrayOf<NodeID>> forbidShareRowAdj_;
        ArrayOf<ArrayOf<NodeID>> forbidShareColAdj_;

        DynamicBitset isFrozen_;
        ArrayOf<NodeID> nonFrozenIndices_;

        ArrayOf<ValueID> allElements_;

        mutable std::mt19937 rng;

        bool automaticAnnealing_ = true;

    private /* methods */:
        ArrayOf<NodeID> getNeighbors(int idx, bool diagonals) const;

        void rebuildConstraints();

        int getLocalEnergy(
            int idx,
            const ArrayOf<ValueID>& state
        ) const;

        int getPairEnergyForElements(
            const ArrayOf<ValueID>& elements,
            const ArrayOf<ValueID>& posMap
        ) const;

        bool validateGridInternal(const Grid& grid) const;
};

GridShuffler::GridShuffler()
    : GridShuffler(std::random_device{}()){}

GridShuffler::GridShuffler(const uint32_t seed)
    : rng(seed){}

size_t GridShuffler::getShuffledGridCount() const noexcept {
    return shuffleGrids_.size();
}

bool GridShuffler::setGrid(const Grid& grid) {
    if (grid.empty()) {
        return false;
    }

    if (grid == originalGrid_) {
        return true;
    }

    originalGrid_ = grid;
    shuffleGrids_.clear();
    this->rebuildConstraints();
    return true;
}

void GridShuffler::setConfig(const ShuffleConfig& cfg) {
    if (cfg == config_) return;

    config_ = cfg;
    this->rebuildConstraints();
}

void GridShuffler::setAnnealingConfig(const AnnealingConfig& cfg) {
    annealingConfig_ = cfg;
    automaticAnnealing_ = false;
}

void GridShuffler::setPenaltyWeights(const PenaltyWeights& weights) {
    penaltyWeights_ = weights;
}

const Grid& GridShuffler::getOriginalGrid() const noexcept {
    return originalGrid_;
}

const Grid& GridShuffler::getGrid() const noexcept {
    if (shuffleGrids_.empty()) {
        return originalGrid_;
    }
    return shuffleGrids_.back();
}

const Grid& GridShuffler::getGrid(const int index) const {
    if (index < 0 || static_cast<size_t>(index) >= shuffleGrids_.size()) {
        throw std::out_of_range("GridRandomizer: Index out of range.");
    }
    return shuffleGrids_[index];
}

std::expected<ResultType, ShuffleError> GridShuffler::shuffle() {
    if (gridSize_ == 0)
        return std::unexpected(ShuffleError::EmptyGrid);

    namespace chrn = std::chrono;

    const auto algoStart = chrn::high_resolution_clock::now();

    if (automaticAnnealing_) {
        const int dynamicMaxSteps = std::max(50'000, gridSize_ * 300);

        const double T0 = -10.0 / std::log(0.5);
        constexpr double Tend = 0.01;
        const double alpha = std::pow(Tend / T0, 1.0 / dynamicMaxSteps);

        annealingConfig_.maxSteps = dynamicMaxSteps;
        annealingConfig_.initialTemperature = T0;
        annealingConfig_.coolingRate = alpha;
    }

    for (const auto attempt : std::views::iota(0, annealingConfig_.maxAttempts)) {
        auto state = std::views::iota(0, gridSize_) | std::ranges::to<ArrayOf<NodeID>>();

        auto nonFrozenValues = nonFrozenIndices_ | std::views::transform([&state](const int idx) {
            return state[idx];
        }) | std::ranges::to<ArrayOf<NodeID>>();
        std::ranges::shuffle(nonFrozenValues, rng);

        for (const auto [idx, val] : std::views::zip(nonFrozenIndices_, nonFrozenValues)) {
            state[idx] = val;
        }

        ArrayOf<ValueID> posMap(gridSize_);
        for (int i = 0; i < state.size(); i++) {
            posMap[state[i]] = i;
        }

        int totalEnergy = std::ranges::fold_left(
            std::views::iota(0, gridSize_), 0,
            [&](const int acc, const int idx) {
                return acc + getLocalEnergy(idx, state);
            }
        ) + getPairEnergyForElements(allElements_, posMap);

        auto temperature = annealingConfig_.initialTemperature;

        if (nonFrozenIndices_.size() < 2 && totalEnergy > 0) {
            return std::unexpected(ShuffleError::Unsatisfiable);
        }

        std::uniform_int_distribution<uint64_t> dist{0ULL, nonFrozenIndices_.size() - 1};
        std::uniform_real_distribution probDist{0.0, 1.0};

        ArrayOf<NodeID> involvedPairVals;
        involvedPairVals.reserve(32);
        ArrayOf<NodeID> affectedIndices;
        affectedIndices.reserve(18);

        int step = 0;

        for (; totalEnergy > 0 && step < annealingConfig_.maxSteps; step++) {
            const auto r_idx1 = dist(rng);
            auto r_idx2 = dist(rng);
            while (r_idx1 == r_idx2) {
                r_idx2 = dist(rng);
            }

            const auto [idx1, idx2] = std::make_pair(nonFrozenIndices_[r_idx1], nonFrozenIndices_[r_idx2]);

            const auto [val1, val2] = std::make_pair(state[idx1], state[idx2]);

            affectedIndices.clear();
            affectedIndices.push_back(idx1);
            affectedIndices.push_back(idx2);
#if __cpp_lib_containers_ranges >= 202202L
            affectedIndices.append_range(neighborsOfPos[idx1]);
            affectedIndices.append_range(neighborsOfPos[idx2]);
#else
            affectedIndices.insert(affectedIndices.cend(), neighborsOfPos[idx1].begin(), neighborsOfPos[idx1].end());
            affectedIndices.insert(affectedIndices.cend(), neighborsOfPos[idx2].begin(), neighborsOfPos[idx2].end());
#endif

            std::ranges::sort(affectedIndices);
            affectedIndices.erase(
                std::ranges::unique(affectedIndices).begin(),
                affectedIndices.end()
            );

            const int localEnergyBefore = std::ranges::fold_left(
                affectedIndices, 0,
                [&] (const int acc, const int idx) {
                    return acc + getLocalEnergy(idx, state);
                }
            );

            involvedPairVals.clear();
            involvedPairVals.push_back(val1);
            involvedPairVals.push_back(val2);
#if __cpp_lib_containers_ranges >= 202202L
            involvedPairVals.append_range(forbidShareRowAdj_[val1]);
            involvedPairVals.append_range(forbidShareColAdj_[val1]);
            involvedPairVals.append_range(forbidShareRowAdj_[val2]);
            involvedPairVals.append_range(forbidShareColAdj_[val2]);
#else
            involvedPairVals.insert(involvedPairVals.cend(), forbidShareRowAdj_[val1].begin(), forbidShareRowAdj_[val1].end());
            involvedPairVals.insert(involvedPairVals.cend(), forbidShareColAdj_[val1].begin(), forbidShareColAdj_[val1].end());
            involvedPairVals.insert(involvedPairVals.cend(), forbidShareRowAdj_[val2].begin(), forbidShareRowAdj_[val2].end());
            involvedPairVals.insert(involvedPairVals.cend(), forbidShareColAdj_[val2].begin(), forbidShareColAdj_[val2].end());
#endif

            std::ranges::sort(involvedPairVals);
            involvedPairVals.erase(
                std::ranges::unique(involvedPairVals).begin(),
                involvedPairVals.end()
            );

            const int pairEnergyBefore = getPairEnergyForElements(involvedPairVals, posMap);

            state[idx1] = val2;
            state[idx2] = val1;
            posMap[val1] = idx2;
            posMap[val2] = idx1;

            const int localEnergyAfter = std::ranges::fold_left(
                affectedIndices, 0,
                [&] (const int acc, const int idx) {
                    return acc + getLocalEnergy(idx, state);
                }
            );

            const int pairEnergyAfter = getPairEnergyForElements(involvedPairVals, posMap);

            const int delta = (localEnergyAfter + pairEnergyAfter) - (localEnergyBefore + pairEnergyBefore);

            if (delta < 0 || (temperature > 0.0 && probDist(rng) < std::exp(-static_cast<double>(delta) / temperature))) {
                totalEnergy += delta;
            }
            else {
                state[idx1] = val1;
                state[idx2] = val2;
                posMap[val1] = idx1;
                posMap[val2] = idx2;
            }

            temperature *= annealingConfig_.coolingRate;
        }

        if (totalEnergy == 0) {
            const auto end = chrn::high_resolution_clock::now();
            shuffleGrids_.emplace_back(gridRow_, gridCol_,
                state | std::views::transform(
                    [this](const int val){return IDToString_[val];}
                ) | std::ranges::to<ArrayOf<DataType>>()
            );
            return ResultType{.doneAtAttempt=attempt, .doneAtStep=step, .tookMUS=chrn::duration_cast<chrn::microseconds>(end - algoStart).count()};
        }
#ifdef __EMSCRIPTEN__
        if (attempt % 200 == 0) {
            emscripten_sleep(0);
        }
#endif
    }
    return std::unexpected(ShuffleError::MaxAttemptsReached);
}

bool GridShuffler::validateResult() const {
    if (shuffleGrids_.empty())
        return false;
    return this->validateGridInternal(shuffleGrids_.back());
}

void GridShuffler::clearShuffledGrids() {
    shuffleGrids_.clear();
}

const ArrayOf<Grid>& GridShuffler::getAllGrids() const noexcept {
    return shuffleGrids_;
}

ArrayOf<NodeID> GridShuffler::getNeighbors(const int idx, const bool diagonals) const {
    const int row = idx / gridCol_;
    const int col = idx % gridCol_;

    ArrayOf<NodeID> res;
    res.reserve(diagonals? 8: 4);

    //  上北下南左西右東

    //  北
    if (row > 0) res.push_back((row - 1) * gridCol_ + col);
    //  南
    if (row < gridRow_ - 1) res.push_back((row + 1) * gridCol_ + col);

    //  西
    if (col > 0) res.push_back(row * gridCol_ + col - 1);
    //  東
    if (col < gridCol_ - 1) res.push_back(row * gridCol_ + col + 1);

    if (diagonals) {
        //  西北
        if (row > 0 && col > 0) res.push_back((row - 1) * gridCol_ + col - 1);
        //  東北
        if (row > 0 && col < gridCol_ - 1) res.push_back((row - 1) * gridCol_ + col + 1);
        //  西南
        if (row < gridRow_ - 1 && col > 0) res.push_back((row + 1) * gridCol_ + col - 1);
        //  東南
        if (row < gridRow_ - 1 && col < gridCol_ - 1) res.push_back((row + 1) * gridCol_ + col + 1);
    }

    return res;
}

void GridShuffler::rebuildConstraints() {
    gridRow_ = originalGrid_.rowCount();
    gridCol_ = gridRow_ > 0? originalGrid_.colCount(): 0;
    gridSize_ = originalGrid_.size();

    stringToID_.clear();

    IDToString_.clear();
    IDToString_.reserve(gridSize_);

    for (const auto& item : originalGrid_) {
        stringToID_.emplace(item.data(), static_cast<int>(IDToString_.size()));
        IDToString_.emplace_back(item.data());
    }

    originalPos_.resize(gridSize_);
    allElements_.resize(gridSize_);
    std::ranges::iota(originalPos_, 0);
    std::ranges::iota(allElements_, 0);

    neighborsOfPos.resize(gridSize_);
    for (int i = 0; i < gridSize_; ++i) {
        neighborsOfPos[i] = getNeighbors(i, config_.diagonalsAreNeighbors);
    }

    originalNeighborsMatrix_ = DynamicBitset(static_cast<uint64_t>(gridSize_) * gridSize_);
    customForbiddenMatrix_ = DynamicBitset(static_cast<uint64_t>(gridSize_) * gridSize_);
    forbiddenRowsMatrix_ = DynamicBitset(static_cast<uint64_t>(gridSize_) * gridRow_);
    forbiddenColsMatrix_ = DynamicBitset(static_cast<uint64_t>(gridSize_) * gridCol_);

    isFrozen_ = DynamicBitset(gridSize_);
    nonFrozenIndices_.clear();

    for (int i = 0; i < IDToString_.size(); i++) {
        if (IDToString_[i].empty()) {
            isFrozen_.set(i, true);
        }
        else {
            nonFrozenIndices_.push_back(i);
        }
    }

    for (const int i : std::views::iota(0, gridSize_)) {
        const uint64_t base = static_cast<uint64_t>(i) * gridSize_;

        for (const int neighbor : neighborsOfPos[i])
            originalNeighborsMatrix_.set(base + neighbor, true);
    }

    for (const auto& [a, b] : config_.custom_forbidden_pairs) {
        auto it_a = stringToID_.find(a);
        auto it_b = stringToID_.find(b);

        if (it_a == stringToID_.end() || it_b == stringToID_.end())
            continue;

        const int id_a = it_a->second;
        const int id_b = it_b->second;

        const uint64_t base_a = static_cast<uint64_t>(id_a) * gridSize_;
        const uint64_t base_b = static_cast<uint64_t>(id_b) * gridSize_;

        customForbiddenMatrix_.set(base_a + id_b, true);
        customForbiddenMatrix_.set(base_b + id_a, true);
    }

    forcedRow_.assign(gridSize_, -1);
    forcedCol_.assign(gridSize_, -1);
    forbidShareRowAdj_.assign(gridSize_, std::vector<int>{});
    forbidShareColAdj_.assign(gridSize_, std::vector<int>{});

    for (const auto& constraint : config_.constraints) {
        std::visit(overloaded{
            [&](const ForceRow& c) {
                if (const auto it = stringToID_.find(c.first);
                    it != stringToID_.end() && c.second >= 0 && c.second < gridRow_
                ) {
                    forcedRow_[it->second] = c.second;
                }
            },
            [&](const ForbidRow& c) {
                if (const auto it = stringToID_.find(c.first);
                    it != stringToID_.end() && c.second >= 0 && c.second < gridRow_
                ) {
                    forbiddenRowsMatrix_.set(static_cast<uint64_t>(it->second) * gridRow_ + c.second, true);
                }
            },
            [&](const ForceCol& c) {
                if (const auto it = stringToID_.find(c.first);
                    it != stringToID_.end() && c.second >= 0 && c.second < gridCol_
                ) {
                    forcedCol_[it->second] = c.second;
                }
            },
            [&](const ForbidCol& c) {
                if (const auto it = stringToID_.find(c.first);
                    it != stringToID_.end() && c.second >= 0 && c.second < gridCol_) {
                    forbiddenColsMatrix_.set(static_cast<uint64_t>(it->second) * gridCol_ + c.second, true);
                }
            },
            [&](const ForbidShareRow& c) {
                if (stringToID_.contains(c.first) && stringToID_.contains(c.second)) {
                    const int id1 = stringToID_[c.first];
                    const int id2 = stringToID_[c.second];
                    if (id1 == id2) return;
                    forbidShareRowAdj_[id1].push_back(id2);
                    forbidShareRowAdj_[id2].push_back(id1);
                }
            },
            [&](const ForbidShareCol& c) {
                if (stringToID_.contains(c.first) && stringToID_.contains(c.second)) {
                    const int id1 = stringToID_[c.first];
                    const int id2 = stringToID_[c.second];
                    if (id1 == id2) return;
                    forbidShareColAdj_[id1].push_back(id2);
                    forbidShareColAdj_[id2].push_back(id1);
                }
            }
        }, constraint);
    }
}

int GridShuffler::getLocalEnergy(const int idx, const ArrayOf<ValueID>& state) const {
    const int val = state[idx];

    if (IDToString_[val].empty()) {
        return 0;
    }

    const int row = idx / gridCol_;
    const int col = idx % gridCol_;
    int energy = 0;

    if (!config_.allowFixedPoints && originalPos_[val] == idx) {
        energy += penaltyWeights_.fixedPoint;
    }

    if (forcedRow_[val] != -1 && row != forcedRow_[val]) {
        energy += penaltyWeights_.absolutePosition;
    }
    if (forbiddenRowsMatrix_.test(static_cast<uint64_t>(val) * gridRow_ + row)) {
        energy += penaltyWeights_.absolutePosition;
    }

    if (forcedCol_[val] != -1 && col != forcedCol_[val]) {
        energy += penaltyWeights_.absolutePosition;
    }
    if (forbiddenColsMatrix_.test(static_cast<uint64_t>(val) * gridCol_ + col)) {
        energy += penaltyWeights_.absolutePosition;
    }

    for (const int n_idx : neighborsOfPos[idx]) {
        const int neighbor_val = state[n_idx];
        if (IDToString_[neighbor_val].empty())
            continue;
        if (!config_.allowOriginalNeighbors) {
            if (originalNeighborsMatrix_.test(static_cast<uint64_t>(val) * gridSize_ + neighbor_val)) {
                energy += penaltyWeights_.originalNeighbor;
            }
        }
        if (customForbiddenMatrix_.test(static_cast<uint64_t>(val) * gridSize_ + neighbor_val)) {
            energy += penaltyWeights_.customForbidden;
        }
    }

    return energy;
}

int GridShuffler::getPairEnergyForElements(const ArrayOf<ValueID>& elements, const ArrayOf<ValueID>& posMap) const {
    return std::ranges::fold_left(
        elements, 0,
        [&](const int acc, const ValueID val) {
            int localAdd = 0;
            const int posVal = posMap[val];

            const int rVal = posVal / gridCol_;
            const int cVal = posVal % gridCol_;

            for (const int otherVal : forbidShareRowAdj_[val]) {
                if (val < otherVal && rVal == posMap[otherVal] / gridCol_)
                    localAdd += penaltyWeights_.forbidShare;
            }
            for (const int otherVal : forbidShareColAdj_[val]) {
                if (val < otherVal && cVal == posMap[otherVal] % gridCol_)
                    localAdd += penaltyWeights_.forbidShare;
            }
            return acc + localAdd;
        }
    );
}

bool GridShuffler::validateGridInternal(const Grid& grid) const {
    if (grid.size() != gridSize_)
        return false;

    if (grid.colCount() != gridCol_)
        return false;

    std::vector state(gridSize_, -1), posMap(gridSize_, -1);
    DynamicBitset seen(gridSize_);

    for (int idx = 0; idx < gridSize_; ++idx) {
        const auto pos = grid.rawData()[idx];

        if (pos.empty()) {
            if (!IDToString_[idx].empty()) {
                return false;
            }
            state[idx] = idx;
            posMap[idx] = idx;
            continue;
        }

        const auto it = stringToID_.find(pos);
        if (it == stringToID_.end())
            return false;

        const int id = it->second;
        if (seen.test(id)) {
            return false;
        }
        seen.set(id, true);

        state[idx] = id;
        posMap[id] = idx;
    }

    if (!(isFrozen_ | seen).all()) return false;

    const int energy = std::ranges::fold_left(
        std::views::iota(0, gridSize_), 0,
        [&](const int acc, const int val) {
            return acc + getLocalEnergy(val, state);
        }
    ) + getPairEnergyForElements(state, posMap);

    return energy == 0;
}
