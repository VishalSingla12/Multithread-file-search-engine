#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <functional>
#include <unordered_set>
#include <atomic>
#include "FileMetadata.h"

/**
 * Recursively traverses a directory tree and produces FileMetadata objects.
 *
 * The scanner itself does NO heavy I/O other than directory enumeration
 * and reading small file metadata.  Actual content reading is delegated
 * to the callback (typically a ThreadPool task).
 *
 * Supported extensions are listed in kSupportedExtensions.
 */
class FileScanner {
public:
    /// File extensions the engine can index (without the leading dot).
    static const std::unordered_set<std::string> kSupportedExtensions;

    using FileCallback = std::function<void(FileMetadata)>;

    /**
     * @param callback  Called for every discovered, supported file.
     *                  May be called from multiple threads if the
     *                  scanner is parallelised in the future; it must
     *                  be thread-safe.
     */
    explicit FileScanner(FileCallback callback, uint64_t startDocId = 1);

    /**
     * Scan @p rootPath recursively.
     * Skips inaccessible sub-directories and unsupported file types silently
     * (a warning is logged).
     *
     * @return Number of files submitted to the callback.
     */
    std::size_t scan(const std::filesystem::path& rootPath);

    /** @return true if @p extension (without dot) is indexable. */
    static bool isSupportedExtension(const std::string& extension);

    /** @return The set of supported extensions (read-only). */
    static const std::unordered_set<std::string>& supportedExtensions();

private:
    FileCallback           callback_;
    std::atomic<uint64_t>  nextDocId_{1};
};
