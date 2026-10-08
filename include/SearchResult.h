#pragma once

#include <string>
#include <vector>
#include <cstdint>

/**
 * Represents a single result returned by the search engine.
 * Results are ranked by score (higher = more relevant).
 */
struct SearchResult {
    uint64_t    documentId{0};
    std::string filename;
    std::string path;
    std::string extension;
    uintmax_t   fileSize{0};

    /// TF-IDF-based relevance score.
    double score{0.0};

    /// Which query terms were matched in this document.
    std::vector<std::string> matchedTerms;

    bool operator<(const SearchResult& other) const {
        return score < other.score;          // ascending  (use max-heap)
    }
    bool operator>(const SearchResult& other) const {
        return score > other.score;
    }
};
