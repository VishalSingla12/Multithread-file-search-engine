#pragma once

#include <string>
#include <filesystem>
#include <memory>
#include <atomic>
#include "InvertedIndex.h"
#include "Tokenizer.h"
#include "ThreadPool.h"
#include "FileScanner.h"

/**
 * High-level coordinator that ties together scanning, tokenising, and indexing.
 *
 * IndexManager owns the InvertedIndex and Tokenizer.
 * Callers interact with it through a simple public API.
 *
 * Persistence format (simple binary):
 *   Header: magic "MFSE" + version (uint32) + docCount (uint64)
 *   Per-document record:
 *     id (uint64), filenameLen (uint32), filename (bytes),
 *     pathLen (uint32), path (bytes), extLen (uint32), ext (bytes),
 *     fileSize (uint64), lastModified (int64), tokenCount (uint32),
 *     tokens (tokenLen uint32 + token bytes) * tokenCount
 */
class IndexManager {
public:
    static constexpr uint32_t kMagic   = 0x4D465345u; // 'MFSE'
    static constexpr uint32_t kVersion = 1u;

    explicit IndexManager(std::size_t threadCount = 0);
    ~IndexManager() = default;

    // Non-copyable.
    IndexManager(const IndexManager&)            = delete;
    IndexManager& operator=(const IndexManager&) = delete;

    /**
     * Index all supported files under @p rootPath using the thread pool.
     * Blocks until indexing is complete.
     *
     * @return Number of files successfully indexed.
     */
    std::size_t indexDirectory(const std::filesystem::path& rootPath);

    /** Save the current index to @p filepath. */
    bool saveIndex(const std::filesystem::path& filepath) const;

    /** Load an index from @p filepath, replacing any existing data. */
    bool loadIndex(const std::filesystem::path& filepath);

    /** Remove all data from the index. */
    void clearIndex();

    /** @return Read-only access to the underlying index. */
    const InvertedIndex& index() const { return index_; }

    /** @return Read-only access to the tokenizer. */
    const Tokenizer& tokenizer() const { return tokenizer_; }

    /** @return Number of files successfully indexed in the last run. */
    std::size_t lastIndexedCount() const { return lastIndexedCount_.load(); }

    /** @return Number of files discovered in the last scan. */
    std::size_t lastDiscoveredCount() const { return lastDiscoveredCount_.load(); }

private:
    void indexFile(FileMetadata metadata);

    InvertedIndex          index_;
    Tokenizer              tokenizer_;
    std::size_t            threadCount_;
    std::atomic<std::size_t> lastDiscoveredCount_{0};
    std::atomic<std::size_t> lastIndexedCount_{0};
};
