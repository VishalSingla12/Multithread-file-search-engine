#include "IndexManager.h"
#include "Logger.h"
#include <fstream>
#include <stdexcept>
#include <thread>
#include <algorithm>

static std::size_t resolveThreadCount(std::size_t requested) {
    if (requested > 0) return requested;
    unsigned hw = std::thread::hardware_concurrency();
    return (hw > 0) ? static_cast<std::size_t>(hw) : 4;
}

IndexManager::IndexManager(std::size_t threadCount)
    : tokenizer_(true)
    , threadCount_(resolveThreadCount(threadCount))
{}

std::size_t IndexManager::indexDirectory(const std::filesystem::path& rootPath) {
    uint64_t startId = static_cast<uint64_t>(index_.documentCount()) + 1;

    ThreadPool pool(threadCount_);

    FileScanner scanner([this, &pool](FileMetadata meta) {
        lastDiscoveredCount_.fetch_add(1, std::memory_order_relaxed);
        pool.submit([this, m = std::move(meta)]() mutable {
            indexFile(std::move(m));
        });
    }, startId);

    scanner.scan(rootPath);
    pool.waitForAll();
    pool.stop();

    LOG_INFO("IndexManager: indexed " +
             std::to_string(lastIndexedCount_.load()) + "/" +
             std::to_string(lastDiscoveredCount_.load()) + " files");
    return lastIndexedCount_.load();
}

void IndexManager::indexFile(FileMetadata metadata) {
    std::ifstream file(metadata.absolutePath, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        LOG_WARNING("IndexManager: cannot open " + metadata.absolutePath);
        return;
    }

    constexpr std::size_t kMaxBytes = 10 * 1024 * 1024;
    std::size_t readSize = static_cast<std::size_t>(
        std::min(static_cast<uintmax_t>(kMaxBytes), metadata.fileSize + 1));
    std::string content;
    content.resize(readSize);
    file.read(content.data(), static_cast<std::streamsize>(readSize));
    content.resize(static_cast<std::size_t>(file.gcount()));

    auto tokens = tokenizer_.tokenize(content);
    auto nameTokens = tokenizer_.tokenize(metadata.filename);
    tokens.insert(tokens.end(), nameTokens.begin(), nameTokens.end());

    if (tokens.empty()) {
        LOG_WARNING("IndexManager: no tokens in " + metadata.absolutePath);
        return;
    }

    index_.addDocument(metadata, tokens);
    lastIndexedCount_.fetch_add(1, std::memory_order_relaxed);
}

static void writeString(std::ostream& out, const std::string& s) {
    uint32_t len = static_cast<uint32_t>(s.size());
    out.write(reinterpret_cast<const char*>(&len), sizeof(len));
    out.write(s.data(), len);
}

static std::string readString(std::istream& in) {
    uint32_t len = 0;
    in.read(reinterpret_cast<char*>(&len), sizeof(len));
    std::string s(len, '\0');
    in.read(s.data(), len);
    return s;
}

bool IndexManager::saveIndex(const std::filesystem::path& filepath) const {
    std::error_code ec;
    std::filesystem::create_directories(filepath.parent_path(), ec);

    std::ofstream out(filepath, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        LOG_ERROR("IndexManager: cannot save to " + filepath.string());
        return false;
    }

    out.write(reinterpret_cast<const char*>(&kMagic),   sizeof(kMagic));
    out.write(reinterpret_cast<const char*>(&kVersion), sizeof(kVersion));

    auto docs = index_.allDocuments();
    uint64_t docCount = docs.size();
    out.write(reinterpret_cast<const char*>(&docCount), sizeof(docCount));

    for (const auto& meta : docs) {
        out.write(reinterpret_cast<const char*>(&meta.id),           sizeof(meta.id));
        writeString(out, meta.filename);
        writeString(out, meta.absolutePath);
        writeString(out, meta.extension);
        out.write(reinterpret_cast<const char*>(&meta.fileSize),     sizeof(meta.fileSize));
        out.write(reinterpret_cast<const char*>(&meta.lastModified), sizeof(meta.lastModified));

        std::ifstream f(meta.absolutePath, std::ios::binary);
        std::string content;
        if (f.is_open()) {
            constexpr std::size_t kMaxBytes = 10 * 1024 * 1024;
            std::size_t readSize = static_cast<std::size_t>(
                std::min(static_cast<uintmax_t>(kMaxBytes), meta.fileSize + 1));
            content.resize(readSize);
            f.read(content.data(), static_cast<std::streamsize>(readSize));
            content.resize(static_cast<std::size_t>(f.gcount()));
        }
        auto tokens = tokenizer_.tokenize(content);
        auto nameTokens = tokenizer_.tokenize(meta.filename);
        tokens.insert(tokens.end(), nameTokens.begin(), nameTokens.end());
        std::sort(tokens.begin(), tokens.end());
        tokens.erase(std::unique(tokens.begin(), tokens.end()), tokens.end());

        uint32_t tokenCount = static_cast<uint32_t>(tokens.size());
        out.write(reinterpret_cast<const char*>(&tokenCount), sizeof(tokenCount));
        for (const auto& t : tokens) writeString(out, t);
    }

    LOG_INFO("IndexManager: saved " + std::to_string(docCount) + " docs to " + filepath.string());
    return true;
}

bool IndexManager::loadIndex(const std::filesystem::path& filepath) {
    std::ifstream in(filepath, std::ios::binary);
    if (!in.is_open()) {
        LOG_ERROR("IndexManager: cannot load from " + filepath.string());
        return false;
    }

    uint32_t magic = 0, version = 0;
    in.read(reinterpret_cast<char*>(&magic),   sizeof(magic));
    in.read(reinterpret_cast<char*>(&version), sizeof(version));

    if (magic != kMagic) {
        LOG_ERROR("IndexManager: invalid index file (bad magic)");
        return false;
    }
    if (version != kVersion) {
        LOG_ERROR("IndexManager: unsupported index version " + std::to_string(version));
        return false;
    }

    index_.clear();

    uint64_t docCount = 0;
    in.read(reinterpret_cast<char*>(&docCount), sizeof(docCount));

    for (uint64_t i = 0; i < docCount; ++i) {
        FileMetadata meta;
        in.read(reinterpret_cast<char*>(&meta.id),           sizeof(meta.id));
        meta.filename     = readString(in);
        meta.absolutePath = readString(in);
        meta.extension    = readString(in);
        in.read(reinterpret_cast<char*>(&meta.fileSize),     sizeof(meta.fileSize));
        in.read(reinterpret_cast<char*>(&meta.lastModified), sizeof(meta.lastModified));
        meta.indexed = true;

        uint32_t tokenCount = 0;
        in.read(reinterpret_cast<char*>(&tokenCount), sizeof(tokenCount));
        std::vector<std::string> tokens;
        tokens.reserve(tokenCount);
        for (uint32_t t = 0; t < tokenCount; ++t) {
            tokens.push_back(readString(in));
        }

        if (!in.good()) {
            LOG_ERROR("IndexManager: corrupt index file at doc " + std::to_string(i));
            return false;
        }
        index_.addDocument(meta, tokens);
    }

    LOG_INFO("IndexManager: loaded " + std::to_string(docCount) + " docs from " + filepath.string());
    return true;
}

void IndexManager::clearIndex() {
    index_.clear();
    lastDiscoveredCount_.store(0);
    lastIndexedCount_.store(0);
    LOG_INFO("IndexManager: index cleared");
}
