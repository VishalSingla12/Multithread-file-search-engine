#include "Tokenizer.h"
#include <algorithm>
#include <cctype>
#include <sstream>

Tokenizer::Tokenizer(bool removeStopWords, std::size_t minTokenLength)
    : removeStopWords_(removeStopWords)
    , minTokenLength_(minTokenLength)
{
    if (!removeStopWords_) return;
    static const std::vector<std::string> kStopWords = {
        "a", "an", "the", "and", "or", "but", "in", "on", "at", "to",
        "for", "of", "with", "by", "from", "is", "are", "was", "were",
        "be", "been", "being", "have", "has", "had", "do", "does", "did",
        "will", "would", "could", "should", "may", "might", "must", "can",
        "this", "that", "these", "those", "it", "its", "not", "no", "as",
        "if", "then", "than", "so", "up", "out", "about", "into", "also",
        "all", "any", "both", "each", "few", "more", "most", "other",
        "some", "such", "own", "same", "too", "very", "just", "only"
    };
    stopWords_.insert(kStopWords.begin(), kStopWords.end());
}

std::vector<std::string> Tokenizer::tokenize(const std::string& text) const {
    std::vector<std::string> tokens;
    std::string token;
    token.reserve(32);

    auto flushToken = [&]() {
        if (token.empty()) return;
        std::size_t start = 0;
        while (start < token.size() && !std::isalnum(static_cast<unsigned char>(token[start])))
            ++start;
        std::size_t end = token.size();
        while (end > start) {
            char back = token[end - 1];
            if (std::isalnum(static_cast<unsigned char>(back)) || back == '+') {
                break;
            }
            --end;
        }
        if (start < end) {
            std::string trimmed = token.substr(start, end - start);
            if (shouldKeepToken(trimmed)) {
                tokens.push_back(std::move(trimmed));
            }
        }
        token.clear();
    };

    for (char ch : text) {
        char norm = normalizeChar(ch);
        if (norm != '\0') {
            token += norm;
        } else {
            flushToken();
        }
    }
    flushToken();
    return tokens;
}

std::vector<std::string> Tokenizer::uniqueTokens(const std::string& text) const {
    auto tokens = tokenize(text);
    std::sort(tokens.begin(), tokens.end());
    tokens.erase(std::unique(tokens.begin(), tokens.end()), tokens.end());
    return tokens;
}

std::vector<std::string> Tokenizer::tokenizeQuery(const std::string& text) const {
    std::vector<std::string> tokens;
    std::string token;
    token.reserve(32);

    auto flushToken = [&]() {
        if (token.empty()) return;
        std::size_t start = 0;
        while (start < token.size() && !std::isalnum(static_cast<unsigned char>(token[start])))
            ++start;
        std::size_t end = token.size();
        while (end > start) {
            char back = token[end - 1];
            if (std::isalnum(static_cast<unsigned char>(back)) || back == '+') {
                break;
            }
            --end;
        }
        if (start < end) {
            std::string trimmed = token.substr(start, end - start);
            if (!trimmed.empty()) {
                tokens.push_back(std::move(trimmed));
            }
        }
        token.clear();
    };

    for (char ch : text) {
        char norm = normalizeChar(ch);
        if (norm != '\0') {
            token += norm;
        } else {
            flushToken();
        }
    }
    flushToken();

    std::vector<std::string> unique;
    std::unordered_set<std::string> seen;
    for (auto& t : tokens) {
        if (seen.insert(t).second) {
            unique.push_back(std::move(t));
        }
    }
    return unique;
}

void Tokenizer::addStopWord(const std::string& word) {
    std::string lower = word;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    stopWords_.insert(lower);
}

bool Tokenizer::isStopWord(const std::string& token) const {
    return stopWords_.count(token) > 0;
}

char Tokenizer::normalizeChar(char c) {
    if (std::isspace(static_cast<unsigned char>(c))) return '\0';
    if (std::isalnum(static_cast<unsigned char>(c)) ||
        c == '-' || c == '_' || c == '+' || c == '.') {
        return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return '\0';
}

bool Tokenizer::shouldKeepToken(const std::string& token) const {
    if (token.size() < minTokenLength_) return false;
    if (removeStopWords_ && isStopWord(token)) return false;
    for (char c : token) {
        if (std::isalnum(static_cast<unsigned char>(c))) return true;
    }
    return false;
}
