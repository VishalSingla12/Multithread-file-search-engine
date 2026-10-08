#pragma once

#include <string>
#include <vector>
#include "SearchResult.h"
#include "InvertedIndex.h"
#include "Tokenizer.h"

/**
 * Provides keyword search over an InvertedIndex using TF-IDF ranking.
 *
 * Scoring formula (per document d, query Q):
 *
 *   score(d, Q) = sum over t in Q of:
 *       tf(t, d) * idf(t)
 *
 *   tf(t, d)  = termFrequency(t, d) / maxTermFrequency(d)   (normalised TF)
 *   idf(t)    = log(1 + N / df(t))                          (smoothed IDF)
 *
 * where N = total number of documents, df(t) = number of docs containing t.
 *
 * The engine is stateless beyond holding references to the index and tokenizer;
 * it is therefore safe to call search() from multiple threads concurrently.
 */
class SearchEngine {
public:
    /**
     * @param index      The populated inverted index.
     * @param tokenizer  The same tokenizer used during indexing.
     */
    SearchEngine(const InvertedIndex& index, const Tokenizer& tokenizer);

    /**
     * Search for documents matching @p query.
     *
     * @param query      Raw query string (may contain multiple keywords).
     * @param maxResults Maximum number of results to return (0 = all).
     * @return Ranked list of SearchResult objects, best first.
     */
    std::vector<SearchResult> search(const std::string& query,
                                     std::size_t maxResults = 10) const;

    /**
     * @return Query tokens that were actually found in the index.
     */
    std::vector<std::string> expandQuery(const std::string& query) const;

private:
    const InvertedIndex& index_;
    const Tokenizer&     tokenizer_;
};
