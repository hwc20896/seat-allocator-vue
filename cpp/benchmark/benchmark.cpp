#include <print>
#include <iostream>
#include <chrono>
#include <benchmark/benchmark.h>

#include "shuffler.hpp"
#include "grid.hpp"

static Grid makeGrid(const int rows, const int cols) {
    Grid grid(rows, cols);
    int idx = 1;
    for (auto& cell : grid) {
        cell = std::to_string(idx++);
    }
    return grid;
}


#define BENCHMARK_ALG(FncName, IterateCount) \
BENCHMARK(FncName)\
    ->DenseRange(4, 40)\
    ->MinTime(0.2)\
    ->Unit(benchmark::kMillisecond)\
    ->Repetitions(IterateCount)\
    ->ReportAggregatesOnly(true)

enum class AnnealingMode : int {
    Automatic,
    DefaultConfig,
    TunedConfig,
};

//  調參模式
constexpr auto tunedAnnealingConfig = AnnealingConfig{
    .initialTemperature = 5.0,
    .coolingRate = 0.9990,
    .maxSteps = 350'000,
    .maxAttempts = 5,
};

template <AnnealingMode Mode, bool Diagonals>
static void BM_Shuffle(benchmark::State& state) {
    const int size = state.range(0);
    const auto grid = makeGrid(size, size);

    auto constrainedConfig = ShuffleConfig{};
    constrainedConfig.setAllowOriginalNeighbors(false);
    if constexpr (Diagonals) {
        constrainedConfig.setDiagonalsAreNeighbors(true);
    }

    GridShuffler shuffler{};
    shuffler.setSeed(42);
    shuffler.setConfig(constrainedConfig);
    if constexpr (Mode == AnnealingMode::DefaultConfig) {
        shuffler.setAnnealingConfig(AnnealingConfig{});
    }
    else if constexpr (Mode == AnnealingMode::TunedConfig) {
        shuffler.setAnnealingConfig(tunedAnnealingConfig);
    }
    shuffler.setGrid(grid);

    int64_t error_count = 0;
    int64_t total_steps = 0;
    int64_t total_algo_us = 0;
    int64_t successful_runs = 0;

    for (auto _ : state) {
        if (const auto result = shuffler.shuffle()) {
            total_steps += result.value().doneAtStep;
            total_algo_us += result.value().tookMUS;
            successful_runs++;
        }
        else {
            error_count++;
        }
    }

    const auto total_iterations = static_cast<double>(state.iterations());

    if (total_iterations > 0) {
        state.counters["ErrorRate"] = static_cast<double>(error_count) / total_iterations;
    }

    if (successful_runs > 0) {
        state.counters["AvgSteps"] = static_cast<double>(total_steps) / successful_runs;
        state.counters["AlgoTimeUS"] = static_cast<double>(total_algo_us) / successful_runs;
    }
}

inline constexpr auto BM_Shuffle4Automatic = BM_Shuffle<AnnealingMode::Automatic, false>;
inline constexpr auto BM_Shuffle8Automatic = BM_Shuffle<AnnealingMode::Automatic, true>;
inline constexpr auto BM_Shuffle4DefaultConfig = BM_Shuffle<AnnealingMode::DefaultConfig, false>;
inline constexpr auto BM_Shuffle8DefaultConfig = BM_Shuffle<AnnealingMode::DefaultConfig, true>;
inline constexpr auto BM_Shuffle4TunedConfig = BM_Shuffle<AnnealingMode::TunedConfig, false>;
inline constexpr auto BM_Shuffle8TunedConfig = BM_Shuffle<AnnealingMode::TunedConfig, true>;

BENCHMARK_ALG(BM_Shuffle4Automatic, 5);
BENCHMARK_ALG(BM_Shuffle4DefaultConfig, 5);
BENCHMARK_ALG(BM_Shuffle4TunedConfig, 5);
BENCHMARK_ALG(BM_Shuffle8Automatic, 5);
BENCHMARK_ALG(BM_Shuffle8DefaultConfig, 5);
BENCHMARK_ALG(BM_Shuffle8TunedConfig, 5);

BENCHMARK_MAIN();