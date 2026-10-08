#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <chrono>
#include <cstddef>

/**
 * Records the outcome of a single benchmark run.
 */
struct BenchmarkResult {
    std::size_t threadCount{0};
    std::size_t filesDiscovered{0};
    std::size_t filesIndexed{0};
    double      elapsedSeconds{0.0};
    double      filesPerSecond{0.0};
    double      speedup{1.0};         ///< Relative to the single-thread baseline.
};

/**
 * Orchestrates multi-threaded indexing benchmarks.
 *
 * For each thread count in the provided list the manager:
 *  1. Clears the index.
 *  2. Indexes the target directory.
 *  3. Records timing and derived metrics.
 *  4. Writes a CSV row to the results file.
 */
class BenchmarkManager {
public:
    static constexpr const char* kDefaultOutputFile = "benchmarks/results.csv";

    /**
     * @param targetDirectory  Directory to index during each benchmark run.
     * @param threadCounts     List of thread counts to benchmark.
     * @param outputFile       Path to the CSV output file.
     */
    explicit BenchmarkManager(
        const std::filesystem::path&    targetDirectory,
        const std::vector<std::size_t>& threadCounts,
        const std::filesystem::path&    outputFile = kDefaultOutputFile);

    /**
     * Run all benchmarks and return the results.
     * Also writes results to the CSV file.
     */
    std::vector<BenchmarkResult> run();

    /** Pretty-print the results table to stdout. */
    static void printResults(const std::vector<BenchmarkResult>& results);

private:
    BenchmarkResult runSingle(std::size_t threadCount);
    bool            writeCsv(const std::vector<BenchmarkResult>& results) const;

    std::filesystem::path    targetDirectory_;
    std::vector<std::size_t> threadCounts_;
    std::filesystem::path    outputFile_;
};
