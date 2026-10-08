#include "FileScanner.h"
#include "Logger.h"
#include <filesystem>
#include <system_error>
#include <algorithm>

const std::unordered_set<std::string> FileScanner::kSupportedExtensions = {
    "txt", "md",  "cpp", "h",  "hpp", "c",
    "py",  "java", "json", "csv", "xml", "yaml",
    "yml", "sh",  "bat", "cmake", "rs", "go",
    "js",  "ts",  "html", "css", "rb",  "swift",
    "kt",  "scala", "pl",  "lua",  "r",  "sql"
};

FileScanner::FileScanner(FileCallback callback, uint64_t startDocId)
    : callback_(std::move(callback))
    , nextDocId_(startDocId)
{}

std::size_t FileScanner::scan(const std::filesystem::path& rootPath) {
    std::error_code ec;
    if (!std::filesystem::exists(rootPath, ec) || ec) {
        LOG_ERROR("FileScanner: path does not exist: " + rootPath.string());
        return 0;
    }
    if (!std::filesystem::is_directory(rootPath, ec) || ec) {
        LOG_ERROR("FileScanner: not a directory: " + rootPath.string());
        return 0;
    }

    std::size_t count = 0;
    using namespace std::filesystem;
    recursive_directory_iterator it(rootPath,
        directory_options::skip_permission_denied, ec);
    if (ec) {
        LOG_ERROR("FileScanner: cannot open directory: " + rootPath.string()
                  + " - " + ec.message());
        return 0;
    }

    for (; it != recursive_directory_iterator(); it.increment(ec)) {
        if (ec) {
            LOG_WARNING("FileScanner: skipping entry: " + ec.message());
            ec.clear();
            continue;
        }
        const auto& entry = *it;

        // Skip massive build/package directories that stall traversal
        std::error_code ecDir;
        if (entry.is_directory(ecDir)) {
            std::string dirName = entry.path().filename().string();
            if (dirName == "node_modules" || dirName == ".git" ||
                dirName == ".svn" || dirName == "venv" ||
                dirName == ".venv" || dirName == "__pycache__" ||
                dirName == "dist" || dirName == "build" ||
                dirName == "target" || dirName == ".next") {
                it.disable_recursion_pending();
                continue;
            }
        }

        std::error_code ecReg;
        if (!entry.is_regular_file(ecReg) || ecReg) continue;

        std::string ext = entry.path().extension().string();
        if (!ext.empty() && ext[0] == '.') ext = ext.substr(1);
        std::transform(ext.begin(), ext.end(), ext.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

        if (!isSupportedExtension(ext)) continue;

        uint64_t docId = nextDocId_.fetch_add(1, std::memory_order_relaxed);
        try {
            FileMetadata meta(entry.path(), docId);
            callback_(std::move(meta));
            ++count;
        } catch (const std::exception& e) {
            LOG_WARNING("FileScanner: skipping " + entry.path().string() + " - " + e.what());
        }
    }
    LOG_INFO("FileScanner: discovered " + std::to_string(count) + " files in " + rootPath.string());
    return count;
}

bool FileScanner::isSupportedExtension(const std::string& extension) {
    return kSupportedExtensions.count(extension) > 0;
}

const std::unordered_set<std::string>& FileScanner::supportedExtensions() {
    return kSupportedExtensions;
}
