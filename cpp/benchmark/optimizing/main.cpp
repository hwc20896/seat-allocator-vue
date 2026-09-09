#include <iostream>

#include "shuffler.hpp"
#include <nlohmann/json.hpp>
#include <print>

#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <cmath>
#include <array>

static constexpr int kGridSeed = 42;
static constexpr std::array gridSize = {10, 30, 50};
static constexpr std::array resultMultiplier = {0.2, 0.3, 0.5};

namespace {
struct Options {
    std::string mode;
    nlohmann::json params = nlohmann::json::object();
};

struct Result {
    double average_ms;
    double avg_steps;
    double avg_attempts;
    double error_rate;
};

std::pair<std::string_view, std::string_view> split_key_value(std::string_view arg) {
    const auto eq = arg.find('=');
    if (eq == std::string_view::npos) throw std::invalid_argument("expected --key=value, got: " + std::string(arg));
    return {arg.substr(0, eq), arg.substr(eq + 1)};
}

Options parse_args(const int argc, char** argv) {
    Options opts;
    for (int i = 1; i < argc; ++i) {
        const auto [key, value] = split_key_value(argv[i]);
        if (key == "--mode") {
            opts.mode = std::string(value);
        } else if (key == "--params") {
            opts.params = nlohmann::json::parse(std::string(value));
            if (!opts.params.is_object())
                throw std::invalid_argument("--params must be a JSON object, e.g. {\"T0\": 5.0}");
        } else {
            throw std::invalid_argument("unknown option: " + std::string(key));
        }
    }
    if (opts.mode.empty()) throw std::invalid_argument("missing required option --mode");
    return opts;
}

void to_json(nlohmann::json& j, const Result& r) {
    j = nlohmann::json{
        {"average_ms", r.average_ms},
        {"avg_steps", r.avg_steps},
        {"avg_attempts", r.avg_attempts},
        {"error_rate", r.error_rate},
    };
}

Grid makeGrid(const int rows, const int cols) {
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

    std::vector<int> indices(totalCells);
    std::ranges::iota(indices, 0);

    std::mt19937 rng(kGridSeed);
    std::ranges::shuffle(indices, rng);

    for (size_t i = 0; i < emptyCount; ++i) {
        grid[indices[i]] = "";
    }

    return grid;
}

ShuffleConfig makeBenchmarkConfig(const Grid& grid, const uint32_t seed) {
    ShuffleConfig cfg;
    cfg.allowOriginalNeighbors = false;
    cfg.allowFixedPoints = false;
    cfg.crossAisleAreNeighbors = true;

    const int rows = grid.rowCount();
    const int cols = grid.colCount();

    std::vector<int> cells;
    for (int i = 0; i < grid.size(); ++i) {
        if (!grid.rawData()[i].empty()) cells.push_back(i);
    }
    const int N = static_cast<int>(cells.size());

    std::mt19937 rng(seed);
    std::ranges::shuffle(cells, rng);

    std::vector<int> orderOf(grid.size(), -1);
    for (int i = 0; i < N; ++i) orderOf[cells[i]] = i;

    std::vector<char> used(N, 0);

    const int cliqueSize = std::min(N / 4, N);
    std::vector<std::string> clique;
    clique.reserve(cliqueSize);
    for (int i = 0; i < cliqueSize; ++i) {
        clique.push_back(grid.rawData()[cells[i]]);
        used[i] = 1;
    }
    for (int a = 0; a < cliqueSize; ++a)
        for (int b = a + 1; b < cliqueSize; ++b)
            cfg.addForbiddenPair(clique[a], clique[b]);

    cfg.enableBuddyMatching = true;
    cfg.doBuddyRotate = true;

    const int aCount= std::min(N / 6, N - cliqueSize);
    const int bCount = std::min(N / 18, N - cliqueSize - aCount);

    std::vector<std::string> groupA;
    groupA.reserve(aCount);
    for (int i = cliqueSize; i < cliqueSize + aCount; ++i) {
        groupA.emplace_back(grid.rawData()[cells[i]]);
    }

    std::vector<std::string> groupB;
    groupB.reserve(bCount);
    for (int i = cliqueSize + aCount; i < cliqueSize + aCount + bCount; ++i) {
        groupB.emplace_back(grid.rawData()[cells[i]]);
    }

    cfg.setBuddyGroups(groupA, groupB);
    return cfg;
}
}

/*
 *  Examples:
 *    ./algo_optimizing --mode=fixed   --params={"T0": 5.0, "alpha": 0.999, "maxSteps": 350000}
 *    ./algo_optimizing --mode=dynamic --params={"min_step": 50000, "size_mul": 300, "alpha": 0.99}
 */
int main(const int argc, char** argv) {
    GridShuffler shuffler;
    shuffler.setSeed(kGridSeed);
    try {
        const auto [mode, params] = parse_args(argc, argv);

        if (mode == "fixed") {
            const double t0 = params.value("T0", 5.0);
            const double alpha = params.value("alpha", 0.999);
            const int maxSteps = params.value("maxSteps", 350'000);
            shuffler.setAnnealingConfig({
                .initialTemperature = t0,
                .coolingRate = alpha,
                .maxSteps = maxSteps,
                .maxAttempts = 5
            });
        } else if (mode == "dynamic") {
            const int min_step = params.value("min_step", 50'000);
            const int size_mul = params.value("size_mul", 300);
            const double alpha = params.value("alpha", 0.0);
            shuffler.setAnnealingConfig([=](const int nonEmptyCount) -> AnnealingConfig {
                const double sideLength = std::sqrt(nonEmptyCount);
                const int maxSteps = std::max(min_step, static_cast<int>(sideLength * size_mul));
                return {
                    .initialTemperature = std::min(20.0, 5.0 + std::log2(sideLength)),
                    .coolingRate = alpha,
                    .maxSteps = maxSteps,
                    .maxAttempts = 5
                };
            });
        } else {
            throw std::invalid_argument("unknown mode: " + mode);
        }
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what();
        return 1;
    }

    static_assert(gridSize.size() == resultMultiplier.size());

    try {
        Result result{};
        double weightedMs = 0.0;
        double totalSteps = 0.0;
        double totalAttemptIdx = 0.0;
        int successes = 0;
        int attempts = 0;

        for (size_t i = 0; i < gridSize.size(); ++i) {
            const Grid grid = makeGrid(gridSize[i], gridSize[i]+2);
            shuffler.setGrid(grid);
            shuffler.setConfig(makeBenchmarkConfig(grid, kGridSeed + static_cast<uint32_t>(i)));

            double gridSumUs = 0.0;
            int gridSuccesses = 0;

            for (int attempt = 0; attempt < 15; ++attempt) {
                ++attempts;
                if (const auto shuffleResult = shuffler.shuffle()) {
                    gridSumUs += shuffleResult->tookMUS;
                    totalSteps += shuffleResult->doneAtStep;
                    totalAttemptIdx += shuffleResult->doneAtAttempt;
                    ++gridSuccesses;
                }
            }

            const double gridAvgMs = gridSumUs / gridSuccesses / 1000.0;
            if (gridSuccesses > 0) {
                weightedMs += gridAvgMs * resultMultiplier[i];
                //  std::cerr on purpose
            }
            std::println(std::cerr, "grid {}x{}: avg {:.3f} ms, attempts={}, failures={}",
                             gridSize[i], gridSize[i], gridAvgMs, 15, 15 - gridSuccesses);
            successes += gridSuccesses;
        }

        result.average_ms = weightedMs / static_cast<double>(gridSize.size());
        result.avg_steps = totalSteps / successes;
        result.avg_attempts = totalAttemptIdx / successes;
        result.error_rate = static_cast<double>(attempts - successes) / attempts;

        std::cout << nlohmann::json(result).dump();
    }
    catch (const std::exception& e) {
        std::cerr << "error during benchmark: " << e.what();
        return 1;
    }}