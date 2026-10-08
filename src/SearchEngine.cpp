#include "SearchEngine.h"
#include <cmath>
#include <unordered_map>
#include <algorithm>

SearchEngine::SearchEngine(const InvertedIndex& index, const Tokenizer& tokenizer)
    : index_(index)
    , tokenizer_(tokenizer)
{}

std::vector<SearchResult> SearchEngine::search(const std::string& query,
                                                std::size_t maxResults) const {
    auto queryTokens = tokenizer_.tokenizeQuery(query);
    if (queryTokens.empty()) {
        queryTokens = tokenizer_.uniqueTokens(query);
    }
    if (queryTokens.empty()) return {};

    const std::size_t N = index_.documentCount();
    if (N == 0) return {};

    std::unordered_map<uint64_t, double> scoreAcc;
    std::unordered_map<uint64_t, std::vector<std::string>> matchedTerms;

    for (const auto& token : queryTokens) {
        auto exactPostings = index_.lookup(token);
        auto prefixMap     = index_.lookupPrefix(token);

        if (exactPostings.empty() && prefixMap.empty()) continue;

        // 1. Exact match scoring
        if (!exactPostings.empty()) {
            const double df  = static_cast<double>(exactPostings.size());
            const double idf = std::log(1.0 + static_cast<double>(N) / df);

            for (const auto& p : exactPostings) {
                const double normTf = static_cast<double>(p.termFrequency) /
                                      (static_cast<double>(p.termFrequency) + 1.0);
                scoreAcc[p.documentId] += normTf * idf;
                matchedTerms[p.documentId].push_back(token);
            }
        }

        // 2. Prefix expansion scoring for terms starting with 'token'
        std::vector<std::string> prefixTerms;
        prefixTerms.reserve(prefixMap.size());
        for (const auto& [term, _] : prefixMap) {
            if (term != token) {
                prefixTerms.push_back(term);
            }
        }
        std::sort(prefixTerms.begin(), prefixTerms.end());

        // Limit prefix expansions to top 100 vocabulary terms to avoid skewing
        constexpr std::size_t kMaxPrefixExpansions = 100;
        if (prefixTerms.size() > kMaxPrefixExpansions) {
            prefixTerms.resize(kMaxPrefixExpansions);
        }

        for (const auto& pTerm : prefixTerms) {
            const auto& postings = prefixMap[pTerm];
            const double df  = static_cast<double>(postings.size());
            const double idf = std::log(1.0 + static_cast<double>(N) / df);

            for (const auto& p : postings) {
                const double normTf = static_cast<double>(p.termFrequency) /
                                      (static_cast<double>(p.termFrequency) + 1.0);
                // Prefix matches weighted at 0.75x
                scoreAcc[p.documentId] += 0.75 * normTf * idf;
                matchedTerms[p.documentId].push_back(pTerm);
            }
        }
    }

    if (scoreAcc.empty()) return {};

    std::vector<SearchResult> results;
    results.reserve(scoreAcc.size());

    for (const auto& [docId, score] : scoreAcc) {
        const FileMetadata* meta = index_.getDocument(docId);
        if (!meta) continue;

        SearchResult r;
        r.documentId   = docId;
        r.filename     = meta->filename;
        r.path         = meta->absolutePath;
        r.extension    = meta->extension;
        r.fileSize     = meta->fileSize;
        r.score        = score;
        r.matchedTerms = matchedTerms[docId];
        std::sort(r.matchedTerms.begin(), r.matchedTerms.end());
        r.matchedTerms.erase(std::unique(r.matchedTerms.begin(), r.matchedTerms.end()),
                             r.matchedTerms.end());
        results.push_back(std::move(r));
    }

    std::sort(results.begin(), results.end(),
              [](const SearchResult& a, const SearchResult& b) {
                  if (std::abs(a.score - b.score) > 1e-6) {
                      return a.score > b.score;
                  }
                  return a.documentId < b.documentId;
              });

    if (maxResults > 0 && results.size() > maxResults) {
        results.resize(maxResults);
    }
    return results;
}

std::vector<std::string> SearchEngine::expandQuery(const std::string& query) const {
    auto tokens = tokenizer_.tokenizeQuery(query);
    if (tokens.empty()) tokens = tokenizer_.uniqueTokens(query);
    std::vector<std::string> found;
    for (const auto& t : tokens) {
        if (!index_.lookup(t).empty() || !index_.lookupPrefix(t).empty()) {
            found.push_back(t);
        }
    }
    return found;
}
