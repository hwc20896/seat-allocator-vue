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
    ->ReportAggregatesOnly(true)\
    ->Complexity()

static void BM_Shuffle4Neighborhood(benchmark::State& state) {
    const int size = state.range(0);
    const auto grid = makeGrid(size, size);

    const ShuffleConfig constrainedConfig = ShuffleConfig{}.setAllowOriginalNeighbors(false);

    GridShuffler shuffler{};
    shuffler.setSeed(42);
    shuffler.setConfig(constrainedConfig);
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

static void BM_Shuffle8Neighborhood(benchmark::State& state) {
    const int size = state.range(0);
    const auto grid = makeGrid(size, size);

    const ShuffleConfig constrainedConfig = ShuffleConfig{}.setAllowOriginalNeighbors(false)
                                                              .setDiagonalsAreNeighbors(true);

    GridShuffler shuffler{};
    shuffler.setSeed(42);
    shuffler.setConfig(constrainedConfig);
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
            continue;
        }
        error_count++;
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
BENCHMARK_ALG(BM_Shuffle4Neighborhood, 5);

BENCHMARK_ALG(BM_Shuffle8Neighborhood, 5);

BENCHMARK_MAIN();