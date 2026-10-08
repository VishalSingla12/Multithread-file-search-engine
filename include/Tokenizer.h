#pragma once

#include <string>
#include <vector>
#include <unordered_set>

/**
 * Converts raw text into a normalised list of tokens suitable for indexing.
 *
 * Rules applied in order:
 *  1. Lower-case the entire input.
 *  2. Split on whitespace and common punctuation (keeping intra-word
 *     hyphens and underscores so that identifiers stay intact).
 *  3. Strip leading/trailing non-alphanumeric characters from each token.
 *  4. Discard tokens shorter than minTokenLength.
 *  5. Optionally remove stop words.
 */
class Tokenizer {
public:
    static constexpr std::size_t kDefaultMinLength = 2;

    /**
     * @param removeStopWords  If true, common English stop words are dropped.
     * @param minTokenLength   Tokens shorter than this are ignored.
     */
    explicit Tokenizer(bool removeStopWords  = true,
                       std::size_t minTokenLength = kDefaultMinLength);

    /**
     * Tokenise @p text and return the resulting token list.
     * The same token may appear multiple times (useful for TF counting).
     */
    std::vector<std::string> tokenize(const std::string& text) const;

    /**
     * Convenience: tokenise and de-duplicate.
     */
    std::vector<std::string> uniqueTokens(const std::string& text) const;

    /**
     * Tokenise a search query string.
     * Unlike tokenize(), this does not discard short tokens (such as 1-character
     * queries like "a" or "c") and retains words so prefix search works accurately.
     */
    std::vector<std::string> tokenizeQuery(const std::string& query) const;

    /** Add a custom word to the stop-word list. */
    void addStopWord(const std::string& word);

    bool isStopWord(const std::string& token) const;

private:
    static char normalizeChar(char c);
    bool        shouldKeepToken(const std::string& token) const;

    bool                            removeStopWords_;
    std::size_t                     minTokenLength_;
    std::unordered_set<std::string> stopWords_;
};
