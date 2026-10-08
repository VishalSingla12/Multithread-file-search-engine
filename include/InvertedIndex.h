#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <shared_mutex>
#include <cstdint>
#include "FileMetadata.h"

/**
 * Per-document posting data stored in the inverted index.
 */
struct Posting {
    uint64_t    documentId{0};
    uint32_t    termFrequency{0};  ///< How many times the term appears in the doc.
};

/**
 * Thread-safe inverted index.
 *
 * Maps token -> list of Posting objects.
 * Also maintains a document metadata table (docId -> FileMetadata).
 *
 * Locking strategy:
 *  - A shared_mutex guards the document table.
 *  - A separate per-shard mutex array guards the posting lists to reduce
 *    contention when many threads index simultaneously.
 */
class InvertedIndex {
public:
    /// Number of shards for the posting-list map (must be a power of two).
    static constexpr std::size_t kShardCount = 64;

    InvertedIndex();
    ~InvertedIndex() = default;

    // Non-copyable.
    InvertedIndex(const InvertedIndex&)            = delete;
    InvertedIndex& operator=(const InvertedIndex&) = delete;

    /**
     * Add a document to the index.
     * @param metadata  Document metadata.
     * @param tokens    Tokenised content of the document (may contain repeats).
     */
    void addDocument(const FileMetadata& metadata,
                     const std::vector<std::string>& tokens);

    /**
     * Remove a document from the index by its ID.
     * @return true if the document was found and removed.
     */
    bool removeDocument(uint64_t documentId);

    /**
     * Look up all postings for a given token.
     * @return Empty vector if the token is not in the index.
     */
    std::vector<Posting> lookup(const std::string& token) const;

    /**
     * Look up postings for all tokens starting with @p prefix.
     * @return Map of token -> postings for each matched token.
     */
    std::unordered_map<std::string, std::vector<Posting>> lookupPrefix(const std::string& prefix) const;

    /**
     * Retrieve document metadata by ID.
     * @return nullptr if not found.
     */
    const FileMetadata* getDocument(uint64_t documentId) const;

    /**
     * Retrieve all document metadata.
     */
    std::vector<FileMetadata> allDocuments() const;

    /** Remove all data from the index. */
    void clear();

    /** @return Number of indexed documents. */
    std::size_t documentCount() const;

    /** @return Number of unique tokens in the index. */
    std::size_t tokenCount() const;

    /** @return Total number of postings across all tokens. */
    std::size_t totalPostings() const;

private:
    /// Choose which shard a token belongs to.
    std::size_t shardIndex(const std::string& token) const;

    // ---- Document table (docId -> metadata) ----
    mutable std::shared_mutex                        docMutex_;
    std::unordered_map<uint64_t, FileMetadata>       documents_;

    // ---- Posting lists (token -> postings), sharded ----
    struct Shard {
        mutable std::mutex                                         mutex;
        std::unordered_map<std::string, std::vector<Posting>>     postings;
    };
    std::vector<Shard> shards_;
};
