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

#if true

#define BENCHMARK_ALG(FncName, IterateCount) \
BENCHMARK(FncName)\
    ->DenseRange(5, 35)\
    ->Unit(benchmark::kMillisecond)\
    ->Repetitions(IterateCount)\
    ->ReportAggregatesOnly(true)

static void BM_Shuffle4Neighborhood(benchmark::State& state) {
    const int size = state.range(0);
    const auto grid = makeGrid(size, size);

    GridShuffler shuffler{};
    shuffler.setGrid(grid);

    int64_t error_count = 0;
    int64_t total_steps = 0;
    int64_t successful_runs = 0;

    for (auto _ : state) {
        if (const auto result = shuffler.shuffle()) {
            total_steps += result.value().doneAtStep;
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
    }
}


static void BM_Shuffle8Neighborhood(benchmark::State& state) {
    const int size = state.range(0);
    const auto grid = makeGrid(size, size);

    const ShuffleConfig constrainedConfig = ShuffleConfig{}.setAllowOriginalNeighbors(false)
                                                              .setDiagonalsAreNeighbors(true);

    GridShuffler shuffler{};
    shuffler.setConfig(constrainedConfig);

    constexpr auto annealingConfig = AnnealingConfig{.maxAttempts = 2};
    shuffler.setAnnealingConfig(annealingConfig);

    int64_t error_count = 0;
    int64_t total_steps = 0;
    int64_t successful_runs = 0;

    for (auto _ : state) {
        if (const auto result = shuffler.shuffle()) {
            total_steps += result.value().doneAtStep;
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
    }
}

static void BM_Shuffle8Neighborhood4by4(benchmark::State& state) {
    const int maxAttempts = state.range(0);
    const auto grid = makeGrid(4, 4);

    const ShuffleConfig constrainedConfig = ShuffleConfig{}.setAllowOriginalNeighbors(false)
                                                              .setDiagonalsAreNeighbors(true);

    GridShuffler shuffler{};
    shuffler.setConfig(constrainedConfig);

    const auto annealingConfig = AnnealingConfig{.maxAttempts = maxAttempts};
    shuffler.setAnnealingConfig(annealingConfig);

    int64_t error_count = 0;
    int64_t total_steps = 0;
    int64_t total_attempts = 0;
    int64_t successful_runs = 0;

    for (auto _ : state) {
        shuffler.setGrid(grid);
        if (const auto result = shuffler.shuffle()) {
            const auto [doneAtAttempt, doneAtStep, _] = result.value();
            total_steps += doneAtStep;
            total_attempts += doneAtAttempt + 1;
            successful_runs++;
            continue;
        }
        error_count++;
    }

    const auto total_iterations = static_cast<double>(state.iterations());

    if (successful_runs > 0) {
        state.counters["AvgAttempts"] = static_cast<double>(total_attempts) / successful_runs;
        state.counters["AvgSteps"] = static_cast<double>(total_steps) / successful_runs;
    }

    if (total_iterations > 0) {
        state.counters["ErrorRate"] = static_cast<double>(error_count) / total_iterations;
    }
}

BENCHMARK_ALG(BM_Shuffle4Neighborhood, 5);

//BENCHMARK_ALG(BM_Shuffle8Neighborhood, 5);

/*
BENCHMARK(BM_Shuffle8Neighborhood4by4)
    ->DenseRange(1, 8)
    ->Unit(benchmark::kMillisecond)
    ->Repetitions(10)
    ->ReportAggregatesOnly(true);
    */

BENCHMARK_MAIN();


#else

int main() {
    /*
    const ShuffleConfig constrainedConfig = ShuffleConfig{}.setAllowOriginalNeighbors(false)
                                                              .setDiagonalsAreNeighbors(true);

    const auto grid = makeGrid(4, 4);
    GridShuffler shuffler;

    shuffler.setConfig(constrainedConfig);
    shuffler.setGrid(grid);

    for (int i = 1; i <= 2; i++) {
        std::println("ATTEMPT {}", i);
        const auto start = std::chrono::high_resolution_clock::now();
        if (const auto result = shuffler.shuffle();
            result.has_value()
        ) {
            std::println("Got result grid:\n");
            std::cout << shuffler.getGrid() << std::endl;
        } else {
            if (result.error() == GridShuffler::ShuffleError::MaxAttemptsReached) {
                std::println("No solution found.");
            }
            else {
                std::println("Grid empty.");
            }
        }
        const auto end = std::chrono::high_resolution_clock::now();
        std::println("Time taken: {}ms", std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());
    }*/

    std::cout << makeGrid(3, 4);
}

#endif