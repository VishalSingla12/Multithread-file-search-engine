#pragma once

#include <string>
#include <filesystem>
#include <chrono>
#include <cstdint>

/**
 * Holds all metadata about a discovered file.
 * This is a plain data class; no business logic resides here.
 */
struct FileMetadata {
    /// Unique numeric ID assigned at scan time.
    uint64_t id{0};

    /// Original filename (e.g. "notes.txt").
    std::string filename;

    /// Absolute, lexically normalized path.
    std::string absolutePath;

    /// Lower-cased file extension without the dot (e.g. "cpp").
    std::string extension;

    /// File size in bytes.
    uintmax_t fileSize{0};

    /// Last-modified time as a Unix epoch (seconds).
    int64_t lastModified{0};

    /// Whether this file's content was successfully indexed.
    bool indexed{false};

    FileMetadata() = default;

    explicit FileMetadata(const std::filesystem::path& path, uint64_t docId);

    bool operator==(const FileMetadata& other) const {
        return id == other.id;
    }
};
