#include <print>
#include <iostream>
#include <chrono>
#include <vector>
#include <string>
#include <numeric>
#include <algorithm>
#include <random>
#include <cmath>
#include <unordered_map>
#include <benchmark/benchmark.h>

#include "shuffler.hpp"
#include "grid.hpp"


constexpr uint32_t kGridSeed = 2026;

static Grid makeGrid(const int rows, const int cols) {
    Grid grid(rows, cols);
    const int totalCells = grid.size();

    int idx = 1;
    for (auto& cell : grid) {
        cell = std::to_string(idx++);
    }

    const auto emptyCount = static_cast<size_t>(std::floor(totalCells * 0.2));

    if (emptyCount == 0) {
        return grid;
    }

    // 3. Generate linear indices [0, totalCells - 1]
    std::vector<int> indices(totalCells);
    std::ranges::iota(indices, 0);

    std::mt19937 rng(kGridSeed);
    std::ranges::shuffle(indices, rng);

    // 5. Clear selected slots via 1D index overload
    for (size_t i = 0; i < emptyCount; ++i) {
        grid[indices[i]] = "";
    }

    return grid;
}

namespace {
enum class AnnealingMode : int {
    Automatic,
    Default,
    Fixed,
    Dynamic,
};

//  靜態調參模式
constexpr auto tunedAnnealingConfig = AnnealingConfig{
    .initialTemperature = 5.0,
    .coolingRate = 0.9990,
    .maxSteps = 350'000,
    .maxAttempts = 5,
};

//  動態調參模式
constexpr auto dynamicAnnealingConfig = [](const int nonEmptyGridSize) -> AnnealingConfig {
    const int dynamicMaxSteps = std::max(50'000, nonEmptyGridSize * 300);

    constexpr double T0 = 10.0 / std::numbers::ln2;
    constexpr double Tend = 0.01;
    const double alpha = std::pow(Tend / T0, 1.0 / dynamicMaxSteps);

    return {
        .initialTemperature = T0,
        .coolingRate = alpha,
        .maxSteps = dynamicMaxSteps,
    };
};

std::unordered_map<int, Grid> gridCache;

void buildGridCache() {
    for (int size = 5; size <= 80; ++size) {
        gridCache.emplace(size, makeGrid(size, size));
    }
}

const Grid& cachedGrid(const int size) {
    return gridCache.at(size);
}

template <AnnealingMode Mode, bool Diagonals>
void BM_Shuffle(benchmark::State& state) {
    const int size = state.range(0);
    const auto& grid = cachedGrid(size);

    auto constrainedConfig = ShuffleConfig{};
    constrainedConfig.setAllowOriginalNeighbors(false);
    if constexpr (Diagonals) {
        constrainedConfig.setDiagonalsAreNeighbors(true);
    }

    GridShuffler shuffler{};
    shuffler.setSeed(kGridSeed);
    shuffler.setConfig(constrainedConfig);

    switch (Mode) {
        case AnnealingMode::Fixed:
            shuffler.setAnnealingConfig(tunedAnnealingConfig);
            break;
        case AnnealingMode::Dynamic:
            shuffler.setAnnealingConfig(dynamicAnnealingConfig);
            break;
        case AnnealingMode::Automatic:
        default:
            break;
    }
    shuffler.setGrid(grid);

    int64_t error_count = 0;
    int64_t total_steps = 0;
    double total_algo_us = 0;
    int64_t successful_runs = 0;
    bool validated = false;

    for (auto _ : state) {
        if (const auto result = shuffler.shuffle()) {
            if (!validated) {
                if (!shuffler.validateResult()) {
                    state.SkipWithError("validateResult() = false");
                    break;
                }
                validated = true;
            }
            total_steps += result.value().doneAtStep;
            total_algo_us += result.value().tookMUS;
            successful_runs++;
        } else {
            error_count++;
        }
    }

    const auto total_iterations = static_cast<double>(state.iterations());

    if (total_iterations > 0) {
        state.counters["ErrorRate"] = static_cast<double>(error_count) / total_iterations;
    }

    if (successful_runs > 0) {
        state.counters["AvgSteps"] = static_cast<double>(total_steps) / successful_runs;
        state.counters["AlgoTimeUS"] = total_algo_us / successful_runs;
    }
}
}

inline constexpr auto BM_Shuffle4AutomaticConfig = BM_Shuffle<AnnealingMode::Automatic, false>;
inline constexpr auto BM_Shuffle8AutomaticConfig = BM_Shuffle<AnnealingMode::Automatic, true>;
inline constexpr auto BM_Shuffle4FixedlyTunedConfig = BM_Shuffle<AnnealingMode::Fixed, false>;
inline constexpr auto BM_Shuffle8FixedlyTunedConfig = BM_Shuffle<AnnealingMode::Fixed, true>;
inline constexpr auto BM_Shuffle4DynamicallyTunedConfig = BM_Shuffle<AnnealingMode::Dynamic, false>;
inline constexpr auto BM_Shuffle8DynamicallyTunedConfig = BM_Shuffle<AnnealingMode::Dynamic, true>;

#define BENCHMARK_ALG(FncName)          \
    BENCHMARK(FncName)                  \
        ->DenseRange(5, 80)             \
        ->Unit(benchmark::kMillisecond) \
        ->MinTime(0.2)                  \
        ->Repetitions(5)                \
        ->ReportAggregatesOnly(true)

BENCHMARK_ALG(BM_Shuffle4AutomaticConfig);
BENCHMARK_ALG(BM_Shuffle8AutomaticConfig);
BENCHMARK_ALG(BM_Shuffle4FixedlyTunedConfig);
BENCHMARK_ALG(BM_Shuffle8FixedlyTunedConfig);
BENCHMARK_ALG(BM_Shuffle4DynamicallyTunedConfig);
BENCHMARK_ALG(BM_Shuffle8DynamicallyTunedConfig);

int main(int argc, char** argv) {
    benchmark::MaybeReenterWithoutASLR(argc, argv);
    char arg0_default[] = "benchmark";
    char* args_default = arg0_default;
    if (!argv) {
        argc = 1;
        argv = &args_default;
    }
    buildGridCache();
    benchmark::Initialize(&argc, argv);
    if (benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
    gridCache.clear();
    return 0;
}