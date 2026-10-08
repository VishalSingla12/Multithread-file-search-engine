#include "BenchmarkManager.h"
#include "IndexManager.h"
#include "Logger.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <filesystem>
#include <sstream>

BenchmarkManager::BenchmarkManager(
    const std::filesystem::path&    targetDirectory,
    const std::vector<std::size_t>& threadCounts,
    const std::filesystem::path&    outputFile)
    : targetDirectory_(targetDirectory)
    , threadCounts_(threadCounts)
    , outputFile_(outputFile)
{}

std::vector<BenchmarkResult> BenchmarkManager::run() {
    std::vector<BenchmarkResult> results;
    results.reserve(threadCounts_.size());

    double baselineSeconds = 0.0;

    for (std::size_t tc : threadCounts_) {
        auto r = runSingle(tc);
        if (results.empty() && r.elapsedSeconds > 0.0) {
            baselineSeconds = r.elapsedSeconds;
        }
        if (baselineSeconds > 0.0) {
            r.speedup = baselineSeconds / r.elapsedSeconds;
        }
        results.push_back(r);

        std::cout << "  Threads: " << tc
                  << "  Files: "   << r.filesIndexed
                  << "  Time: "    << std::fixed << std::setprecision(2) << r.elapsedSeconds << "s"
                  << "  Files/s: " << static_cast<std::size_t>(r.filesPerSecond)
                  << "  Speedup: " << std::setprecision(2) << r.speedup << "x\n";
    }

    writeCsv(results);
    return results;
}

BenchmarkResult BenchmarkManager::runSingle(std::size_t threadCount) {
    BenchmarkResult result;
    result.threadCount = threadCount;

    IndexManager mgr(threadCount);

    auto start = std::chrono::steady_clock::now();
    mgr.indexDirectory(targetDirectory_);
    auto end   = std::chrono::steady_clock::now();

    result.filesDiscovered = mgr.lastDiscoveredCount();
    result.filesIndexed    = mgr.lastIndexedCount();
    result.elapsedSeconds  = std::chrono::duration<double>(end - start).count();
    result.filesPerSecond  = (result.elapsedSeconds > 0)
        ? result.filesIndexed / result.elapsedSeconds : 0.0;
    return result;
}

bool BenchmarkManager::writeCsv(const std::vector<BenchmarkResult>& results) const {
    std::error_code ec;
    std::filesystem::create_directories(outputFile_.parent_path(), ec);

    std::ofstream csv(outputFile_, std::ios::trunc);
    if (!csv.is_open()) {
        LOG_ERROR("BenchmarkManager: cannot write CSV to " + outputFile_.string());
        return false;
    }

    csv << "threads,files_discovered,files_indexed,elapsed_seconds,files_per_second,speedup\n";
    for (const auto& r : results) {
        csv << r.threadCount << ','
            << r.filesDiscovered << ','
            << r.filesIndexed << ','
            << std::fixed << std::setprecision(4) << r.elapsedSeconds << ','
            << std::setprecision(2) << r.filesPerSecond << ','
            << std::setprecision(3) << r.speedup << '\n';
    }
    LOG_INFO("BenchmarkManager: results written to " + outputFile_.string());
    return true;
}

void BenchmarkManager::printResults(const std::vector<BenchmarkResult>& results) {
    std::cout << '\n'
              << std::string(70, '=')
              << "\nBENCHMARK RESULTS\n"
              << std::string(70, '=')
              << '\n'
              << std::left
              << std::setw(10) << "Threads"
              << std::setw(12) << "Discovered"
              << std::setw(12) << "Indexed"
              << std::setw(12) << "Time(s)"
              << std::setw(14) << "Files/sec"
              << std::setw(10) << "Speedup"
              << '\n'
              << std::string(70, '-') << '\n';

    for (const auto& r : results) {
        std::cout << std::left
                  << std::setw(10) << r.threadCount
                  << std::setw(12) << r.filesDiscovered
                  << std::setw(12) << r.filesIndexed
                  << std::setw(12) << std::fixed << std::setprecision(2) << r.elapsedSeconds
                  << std::setw(14) << static_cast<std::size_t>(r.filesPerSecond)
                  << std::setw(10) << std::setprecision(2) << r.speedup << "x\n";
    }
    std::cout << std::string(70, '=') << '\n';
}
