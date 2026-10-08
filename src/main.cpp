#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <chrono>
#include <filesystem>
#include <thread>
#include <sstream>

#include "Logger.h"
#include "IndexManager.h"
#include "SearchEngine.h"
#include "BenchmarkManager.h"

// ---- Terminal formatting helpers ----
static const std::string kReset  = "";
static const std::string kBold   = "";
static const std::string kCyan   = "";
static const std::string kGreen  = "";
static const std::string kYellow = "";

// ============================================================
//  CLI argument parsing
// ============================================================
struct CliOptions {
    std::vector<std::string> indexDirectories;
    std::string searchQuery;
    std::string saveIndexPath;
    std::string loadIndexPath;
    std::string benchmarkDirectory;
    std::size_t threadCount{0};          // 0 = auto
    std::size_t maxResults{10};
    bool        showStats{false};
    bool        clearIndex{false};
    bool        interactiveMode{false};
    bool        helpRequested{false};
};

static void printHelp(const char* argv0) {
    std::cout <<
        "Usage: " << argv0 << " [options]\n\n"
        "Options:\n"
        "  --index <dir>          Index all supported files in <dir>\n"
        "  --search <query>       Search the current index\n"
        "  --stats                Print index statistics\n"
        "  --benchmark <dir>      Run multi-thread benchmark on <dir>\n"
        "  --save-index <file>    Save the index to <file>\n"
        "  --load-index <file>    Load the index from <file>\n"
        "  --clear                Clear the in-memory index\n"
        "  --threads <n>          Number of indexing threads (default: auto)\n"
        "  --max-results <n>      Max search results to show (default: 10)\n"
        "  --interactive          Launch interactive menu\n"
        "  --help                 Show this help\n\n"
        "Examples:\n"
        "  search_engine --index ./documents --threads 8\n"
        "  search_engine --search \"machine learning\"\n"
        "  search_engine --benchmark ./documents\n"
        "  search_engine --interactive\n";
}

static CliOptions parseArgs(int argc, char* argv[]) {
    CliOptions opts;
    if (argc <= 1) { opts.interactiveMode = true; return opts; }

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        auto nextArg = [&]() -> std::string {
            if (i + 1 < argc) return argv[++i];
            return "";
        };
        if      (arg == "--index")        opts.indexDirectories.push_back(nextArg());
        else if (arg == "--search")       opts.searchQuery       = nextArg();
        else if (arg == "--stats")        opts.showStats         = true;
        else if (arg == "--benchmark")    opts.benchmarkDirectory = nextArg();
        else if (arg == "--save-index")   opts.saveIndexPath     = nextArg();
        else if (arg == "--load-index")   opts.loadIndexPath     = nextArg();
        else if (arg == "--clear")        opts.clearIndex        = true;
        else if (arg == "--threads")      opts.threadCount       = std::stoul(nextArg());
        else if (arg == "--max-results")  opts.maxResults        = std::stoul(nextArg());
        else if (arg == "--interactive")  opts.interactiveMode   = true;
        else if (arg == "--help")         opts.helpRequested     = true;
        else {
            std::cerr << "Unknown option: " << arg << "\n";
            opts.helpRequested = true;
        }
    }
    return opts;
}

// ============================================================
//  Pretty print helpers
// ============================================================
static void printBanner() {
    std::cout << kBold << kCyan
              << "\n==========================================\n"
              << "   MULTITHREADED FILE SEARCH ENGINE\n"
              << "==========================================\n"
              << kReset << "\n";
}

static void printStats(const IndexManager& mgr) {
    const auto& idx = mgr.index();
    std::cout << kBold << "\n---- Index Statistics ----\n" << kReset
              << "  Documents   : " << idx.documentCount()   << "\n"
              << "  Unique tokens: " << idx.tokenCount()      << "\n"
              << "  Total postings: " << idx.totalPostings()  << "\n\n";
}

static void printResults(const std::vector<SearchResult>& results) {
    if (results.empty()) {
        std::cout << kYellow << "  No results found.\n" << kReset;
        return;
    }
    std::cout << kBold << "\nTop " << results.size() << " results:\n" << kReset
              << std::string(70, '-') << "\n";
    for (std::size_t i = 0; i < results.size(); ++i) {
        const auto& r = results[i];
        std::cout << kGreen << "[" << (i + 1) << "] " << kReset
                  << r.filename << "\n"
                  << "    Score  : " << std::fixed << std::setprecision(4) << r.score << "\n"
                  << "    Path   : " << r.path << "\n"
                  << "    Size   : " << r.fileSize << " bytes  ." << r.extension << "\n"
                  << "    Matched: ";
        for (const auto& t : r.matchedTerms) std::cout << t << " ";
        std::cout << "\n" << std::string(70, '-') << "\n";
    }
}

// ============================================================
//  Interactive menu
// ============================================================
static void runInteractive(IndexManager& mgr) {
    SearchEngine engine(mgr.index(), mgr.tokenizer());

    std::cout << kBold
              << "\n==========================================\n"
              << "   MULTITHREADED FILE SEARCH ENGINE\n"
              << "==========================================\n" << kReset
              << "  1. Index Directory\n"
              << "  2. Search\n"
              << "  3. Show Index Statistics\n"
              << "  4. Benchmark\n"
              << "  5. Save Index\n"
              << "  6. Load Index\n"
              << "  7. Clear Index\n"
              << "  8. Exit\n\n";

    std::cout << "Enter a choice (1-8) or command (e.g., 'index <dir>', 'search <query>', 'stats', 'help', 'exit'):\n";
    while (true) {
        std::cout << "\n> ";
        std::string line;
        if (!std::getline(std::cin, line)) {
            break;
        }

        // Trim leading and trailing whitespace
        size_t first = line.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) continue;
        size_t last = line.find_last_not_of(" \t\r\n");
        std::string trimmed = line.substr(first, last - first + 1);

        if (trimmed == "8" || trimmed == "exit" || trimmed == "quit") {
            std::cout << "Goodbye.\n";
            return;
        }

        if (trimmed == "help" || trimmed == "?") {
            std::cout << "Available commands:\n"
                      << "  index <directory>  : Index files in directory\n"
                      << "  search <query>      : Search the inverted index\n"
                      << "  stats               : Show index statistics\n"
                      << "  benchmark <dir>     : Run concurrency benchmark on directory\n"
                      << "  save <file>         : Save index to binary file\n"
                      << "  load <file>         : Load index from binary file\n"
                      << "  clear               : Clear current index\n"
                      << "  menu                : Display full interactive menu\n"
                      << "  exit                : Exit application\n";
            continue;
        }

        if (trimmed == "menu") {
            std::cout << kBold
                      << "\n==========================================\n"
                      << "   MULTITHREADED FILE SEARCH ENGINE\n"
                      << "==========================================\n" << kReset
                      << "  1. Index Directory\n"
                      << "  2. Search\n"
                      << "  3. Show Index Statistics\n"
                      << "  4. Benchmark\n"
                      << "  5. Save Index\n"
                      << "  6. Load Index\n"
                      << "  7. Clear Index\n"
                      << "  8. Exit\n";
            continue;
        }

        if (trimmed == "3" || trimmed == "stats") {
            printStats(mgr);
            continue;
        }

        if (trimmed == "7" || trimmed == "clear") {
            mgr.clearIndex();
            std::cout << kGreen << "  Index cleared.\n" << kReset;
            continue;
        }

        if (trimmed == "1") {
            std::cout << "Directory to index: ";
            std::string dir; std::getline(std::cin, dir);
            // Trim leading/trailing whitespace
            size_t dFirst = dir.find_first_not_of(" \t\r\n");
            if (dFirst != std::string::npos) {
                size_t dLast = dir.find_last_not_of(" \t\r\n");
                dir = dir.substr(dFirst, dLast - dFirst + 1);
            }
            // If user accidentally typed "index <path>"
            if (dir.rfind("index ", 0) == 0) {
                dir = dir.substr(6);
            }
            if (dir.size() >= 2 && dir.front() == '"' && dir.back() == '"') {
                dir = dir.substr(1, dir.size() - 2);
            }
            if (dir.empty()) { std::cout << "Empty path.\n"; continue; }
            auto t0 = std::chrono::steady_clock::now();
            std::size_t count = mgr.indexDirectory(dir);
            auto t1 = std::chrono::steady_clock::now();
            double secs = std::chrono::duration<double>(t1 - t0).count();
            std::cout << kGreen << "  Indexed " << count << " files in "
                      << std::fixed << std::setprecision(2) << secs << "s\n" << kReset;
            continue;
        }

        if (trimmed.rfind("index ", 0) == 0) {
            std::string dir = trimmed.substr(6);
            // remove surrounding quotes if present
            if (dir.size() >= 2 && dir.front() == '"' && dir.back() == '"') {
                dir = dir.substr(1, dir.size() - 2);
            }
            auto t0 = std::chrono::steady_clock::now();
            std::size_t count = mgr.indexDirectory(dir);
            auto t1 = std::chrono::steady_clock::now();
            double secs = std::chrono::duration<double>(t1 - t0).count();
            std::cout << kGreen << "  Indexed " << count << " files in "
                      << std::fixed << std::setprecision(2) << secs << "s\n" << kReset;
            continue;
        }

        if (trimmed == "2") {
            std::cout << "Query: ";
            std::string query; std::getline(std::cin, query);
            if (query.empty()) { std::cout << "Empty query.\n"; continue; }
            auto results = engine.search(query, 10);
            printResults(results);
            continue;
        }

        if (trimmed.rfind("search ", 0) == 0) {
            std::string query = trimmed.substr(7);
            if (query.size() >= 2 && query.front() == '"' && query.back() == '"') {
                query = query.substr(1, query.size() - 2);
            }
            auto results = engine.search(query, 10);
            printResults(results);
            continue;
        }

        if (trimmed == "4" || trimmed.rfind("benchmark", 0) == 0) {
            std::string dir;
            if (trimmed == "4" || trimmed == "benchmark") {
                std::cout << "Directory to benchmark: ";
                std::getline(std::cin, dir);
            } else {
                dir = trimmed.substr(10);
            }
            if (dir.empty()) { std::cout << "Empty path.\n"; continue; }

            unsigned hw = std::thread::hardware_concurrency();
            std::vector<std::size_t> threadCounts = {1, 2, 4};
            if (hw >= 8)  threadCounts.push_back(8);
            if (hw >= 16) threadCounts.push_back(16);

            BenchmarkManager bm(dir, threadCounts);
            auto results = bm.run();
            BenchmarkManager::printResults(results);
            continue;
        }

        if (trimmed == "5" || trimmed.rfind("save ", 0) == 0) {
            std::string path;
            if (trimmed == "5") {
                std::cout << "Save path (e.g. index.bin): ";
                std::getline(std::cin, path);
            } else {
                path = trimmed.substr(5);
            }
            if (path.empty()) path = "index.bin";
            if (mgr.saveIndex(path))
                std::cout << kGreen << "  Index saved to " << path << "\n" << kReset;
            else
                std::cout << kYellow << "  Failed to save index.\n" << kReset;
            continue;
        }

        if (trimmed == "6" || trimmed.rfind("load ", 0) == 0) {
            std::string path;
            if (trimmed == "6") {
                std::cout << "Load path: ";
                std::getline(std::cin, path);
            } else {
                path = trimmed.substr(5);
            }
            if (path.empty()) { std::cout << "Empty path.\n"; continue; }
            if (mgr.loadIndex(path))
                std::cout << kGreen << "  Index loaded from " << path << "\n" << kReset;
            else
                std::cout << kYellow << "  Failed to load index.\n" << kReset;
            continue;
        }

        std::cout << "Unknown command. Type 'help' or 'menu'.\n";
    }
}

// ============================================================
//  main
// ============================================================
int main(int argc, char* argv[]) {
    // Suppress log noise from stderr by default; only print warnings+
    Logger::instance().setMinLevel(LogLevel::WARNING);
    Logger::instance().enableConsole(true);

    CliOptions opts = parseArgs(argc, argv);

    if (opts.helpRequested) {
        printHelp(argv[0]);
        return 0;
    }

    printBanner();

    IndexManager mgr(opts.threadCount);
    bool anyAction = false;

    // Load index first if requested
    if (!opts.loadIndexPath.empty()) {
        anyAction = true;
        if (mgr.loadIndex(opts.loadIndexPath))
            std::cout << kGreen << "Index loaded from " << opts.loadIndexPath << "\n" << kReset;
        else
            std::cerr << "Failed to load index from " << opts.loadIndexPath << "\n";
    }

    // Index directories
    if (!opts.indexDirectories.empty()) {
        anyAction = true;
        Logger::instance().setMinLevel(LogLevel::INFO);
        std::size_t totalCount = 0;
        auto t0 = std::chrono::steady_clock::now();
        for (const auto& dir : opts.indexDirectories) {
            std::cout << "Indexing directory: " << dir << "\n";
            totalCount += mgr.indexDirectory(dir);
        }
        auto t1 = std::chrono::steady_clock::now();
        double secs = std::chrono::duration<double>(t1 - t0).count();
        Logger::instance().setMinLevel(LogLevel::WARNING);
        std::cout << kGreen
                  << "Total indexed " << totalCount << " files across "
                  << opts.indexDirectories.size() << " directory(ies) in "
                  << std::fixed << std::setprecision(2) << secs << " seconds\n"
                  << kReset;
    }

    // Save index
    if (!opts.saveIndexPath.empty()) {
        anyAction = true;
        if (!mgr.saveIndex(opts.saveIndexPath))
            std::cerr << "Failed to save index.\n";
        else
            std::cout << kGreen << "Index saved to " << opts.saveIndexPath << "\n" << kReset;
    }

    // Search
    if (!opts.searchQuery.empty()) {
        anyAction = true;
        SearchEngine engine(mgr.index(), mgr.tokenizer());
        auto results = engine.search(opts.searchQuery, opts.maxResults);
        printResults(results);
    }

    // Stats
    if (opts.showStats) {
        anyAction = true;
        printStats(mgr);
    }

    // Clear index
    if (opts.clearIndex) {
        anyAction = true;
        mgr.clearIndex();
        std::cout << "Index cleared.\n";
    }

    // Benchmark
    if (!opts.benchmarkDirectory.empty()) {
        anyAction = true;
        unsigned hw = std::thread::hardware_concurrency();
        std::vector<std::size_t> threadCounts = {1, 2, 4};
        if (hw >= 8)  threadCounts.push_back(8);
        if (hw >= 16) threadCounts.push_back(16);

        std::cout << "Running benchmark on: " << opts.benchmarkDirectory << "\n";
        BenchmarkManager bm(opts.benchmarkDirectory, threadCounts);
        auto results = bm.run();
        BenchmarkManager::printResults(results);
    }

    // Interactive mode (default when no arguments given)
    if (opts.interactiveMode || !anyAction) {
        runInteractive(mgr);
    }

    return 0;
}
