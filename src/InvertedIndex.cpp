#include "InvertedIndex.h"
#include <algorithm>
#include <functional>

InvertedIndex::InvertedIndex()
    : shards_(kShardCount)
{}

std::size_t InvertedIndex::shardIndex(const std::string& token) const {
    return std::hash<std::string>{}(token) & (kShardCount - 1);
}

void InvertedIndex::addDocument(const FileMetadata& metadata,
                                const std::vector<std::string>& tokens) {
    std::unordered_map<std::string, uint32_t> tf;
    for (const auto& t : tokens) {
        ++tf[t];
    }

    {
        std::unique_lock<std::shared_mutex> lock(docMutex_);
        documents_[metadata.id] = metadata;
        documents_[metadata.id].indexed = true;
    }

    for (const auto& [token, freq] : tf) {
        std::size_t si = shardIndex(token);
        std::lock_guard<std::mutex> lock(shards_[si].mutex);
        auto& postings = shards_[si].postings[token];
        auto it = std::find_if(postings.begin(), postings.end(),
            [&](const Posting& p) { return p.documentId == metadata.id; });
        if (it != postings.end()) {
            it->termFrequency = freq;
        } else {
            postings.push_back({metadata.id, freq});
        }
    }
}

bool InvertedIndex::removeDocument(uint64_t documentId) {
    bool found = false;
    {
        std::unique_lock<std::shared_mutex> lock(docMutex_);
        found = (documents_.erase(documentId) > 0);
    }
    if (!found) return false;

    for (auto& shard : shards_) {
        std::lock_guard<std::mutex> lock(shard.mutex);
        for (auto& [token, postings] : shard.postings) {
            postings.erase(
                std::remove_if(postings.begin(), postings.end(),
                    [documentId](const Posting& p) {
                        return p.documentId == documentId;
                    }),
                postings.end());
        }
    }
    return true;
}

std::vector<Posting> InvertedIndex::lookup(const std::string& token) const {
    std::size_t si = shardIndex(token);
    std::lock_guard<std::mutex> lock(shards_[si].mutex);
    const auto& shard = shards_[si].postings;
    auto it = shard.find(token);
    if (it == shard.end()) return {};
    return it->second;
}

std::unordered_map<std::string, std::vector<Posting>> InvertedIndex::lookupPrefix(const std::string& prefix) const {
    std::unordered_map<std::string, std::vector<Posting>> result;
    if (prefix.empty()) return result;

    for (const auto& shard : shards_) {
        std::lock_guard<std::mutex> lock(shard.mutex);
        for (const auto& [token, postings] : shard.postings) {
            if (token.size() >= prefix.size() && token.compare(0, prefix.size(), prefix) == 0) {
                result[token] = postings;
            }
        }
    }
    return result;
}

const FileMetadata* InvertedIndex::getDocument(uint64_t documentId) const {
    std::shared_lock<std::shared_mutex> lock(docMutex_);
    auto it = documents_.find(documentId);
    if (it == documents_.end()) return nullptr;
    return &it->second;
}

std::vector<FileMetadata> InvertedIndex::allDocuments() const {
    std::shared_lock<std::shared_mutex> lock(docMutex_);
    std::vector<FileMetadata> result;
    result.reserve(documents_.size());
    for (const auto& [id, meta] : documents_) {
        result.push_back(meta);
    }
    return result;
}

void InvertedIndex::clear() {
    {
        std::unique_lock<std::shared_mutex> lock(docMutex_);
        documents_.clear();
    }
    for (auto& shard : shards_) {
        std::lock_guard<std::mutex> lock(shard.mutex);
        shard.postings.clear();
    }
}

std::size_t InvertedIndex::documentCount() const {
    std::shared_lock<std::shared_mutex> lock(docMutex_);
    return documents_.size();
}

std::size_t InvertedIndex::tokenCount() const {
    std::size_t count = 0;
    for (const auto& shard : shards_) {
        std::lock_guard<std::mutex> lock(shard.mutex);
        count += shard.postings.size();
    }
    return count;
}

std::size_t InvertedIndex::totalPostings() const {
    std::size_t count = 0;
    for (const auto& shard : shards_) {
        std::lock_guard<std::mutex> lock(shard.mutex);
        for (const auto& [token, postings] : shard.postings) {
            count += postings.size();
        }
    }
    return count;
}
