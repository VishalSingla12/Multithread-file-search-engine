/**
 * example.cpp - Example C++ inverted index snippet for sample data.
 *
 * This file demonstrates the basic structure of an inverted index
 * and is included as sample searchable content.
 */

#include <iostream>
#include <unordered_map>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <cctype>

// Simple in-memory inverted index demonstration
class SimpleIndex {
public:
    /// Add a document with the given content
    void addDocument(int docId, const std::string& content) {
        std::istringstream iss(content);
        std::string word;
        while (iss >> word) {
            // Normalize: lowercase
            std::transform(word.begin(), word.end(), word.begin(), ::tolower);
            // Remove trailing punctuation
            while (!word.empty() && !std::isalnum(word.back())) {
                word.pop_back();
            }
            if (!word.empty()) {
                index_[word].push_back(docId);
            }
        }
        documents_[docId] = content;
    }

    /// Search for documents containing the query word
    std::vector<int> search(const std::string& query) const {
        std::string lower = query;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        auto it = index_.find(lower);
        if (it == index_.end()) return {};
        return it->second;
    }

    void printStats() const {
        std::cout << "Documents: " << documents_.size() << "\n"
                  << "Unique tokens: " << index_.size() << "\n";
    }

private:
    std::unordered_map<std::string, std::vector<int>> index_;
    std::unordered_map<int, std::string>               documents_;
};

int main() {
    SimpleIndex idx;

    idx.addDocument(1, "Machine learning is a subset of artificial intelligence");
    idx.addDocument(2, "Deep learning uses neural networks with many layers");
    idx.addDocument(3, "Natural language processing enables computers to understand text");
    idx.addDocument(4, "Computer vision allows machines to interpret images");

    idx.printStats();

    auto results = idx.search("learning");
    std::cout << "Search 'learning': ";
    for (int id : results) std::cout << id << " ";
    std::cout << "\n";

    return 0;
}
