#include <gtest/gtest.h>
#include "InvertedIndex.h"
#include "FileMetadata.h"
#include <thread>
#include <vector>

static FileMetadata makeMeta(uint64_t id, const std::string& name) {
    FileMetadata m;
    m.id           = id;
    m.filename     = name;
    m.absolutePath = "/tmp/" + name;
    m.extension    = "txt";
    m.fileSize     = 100;
    m.indexed      = true;
    return m;
}

TEST(InvertedIndexTest, EmptyIndex) {
    InvertedIndex idx;
    EXPECT_EQ(idx.documentCount(), 0u);
    EXPECT_EQ(idx.tokenCount(), 0u);
    EXPECT_TRUE(idx.lookup("hello").empty());
}

TEST(InvertedIndexTest, AddAndLookup) {
    InvertedIndex idx;
    auto meta = makeMeta(1, "doc1.txt");
    idx.addDocument(meta, {"machine", "learning", "machine"});

    auto postings = idx.lookup("machine");
    ASSERT_EQ(postings.size(), 1u);
    EXPECT_EQ(postings[0].documentId, 1u);
    EXPECT_EQ(postings[0].termFrequency, 2u); // appeared twice
    EXPECT_EQ(idx.documentCount(), 1u);
}

TEST(InvertedIndexTest, MultipleDocuments) {
    InvertedIndex idx;
    idx.addDocument(makeMeta(1, "a.txt"), {"machine", "learning"});
    idx.addDocument(makeMeta(2, "b.txt"), {"machine", "vision"});
    idx.addDocument(makeMeta(3, "c.txt"), {"deep", "learning"});

    auto mPostings = idx.lookup("machine");
    EXPECT_EQ(mPostings.size(), 2u);

    auto lPostings = idx.lookup("learning");
    EXPECT_EQ(lPostings.size(), 2u);

    auto vPostings = idx.lookup("vision");
    EXPECT_EQ(vPostings.size(), 1u);
}

TEST(InvertedIndexTest, RemoveDocument) {
    InvertedIndex idx;
    idx.addDocument(makeMeta(1, "a.txt"), {"machine", "learning"});
    idx.addDocument(makeMeta(2, "b.txt"), {"machine", "vision"});

    EXPECT_TRUE(idx.removeDocument(1));
    EXPECT_EQ(idx.documentCount(), 1u);

    auto postings = idx.lookup("machine");
    // Only doc 2 should remain
    ASSERT_EQ(postings.size(), 1u);
    EXPECT_EQ(postings[0].documentId, 2u);
}

TEST(InvertedIndexTest, RemoveNonExistentDocument) {
    InvertedIndex idx;
    EXPECT_FALSE(idx.removeDocument(9999));
}

TEST(InvertedIndexTest, Clear) {
    InvertedIndex idx;
    idx.addDocument(makeMeta(1, "a.txt"), {"hello", "world"});
    idx.clear();
    EXPECT_EQ(idx.documentCount(), 0u);
    EXPECT_EQ(idx.tokenCount(), 0u);
    EXPECT_TRUE(idx.lookup("hello").empty());
}

TEST(InvertedIndexTest, GetDocument) {
    InvertedIndex idx;
    auto meta = makeMeta(42, "myfile.txt");
    idx.addDocument(meta, {"test"});

    const auto* found = idx.getDocument(42);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->filename, "myfile.txt");

    const auto* notFound = idx.getDocument(0);
    EXPECT_EQ(notFound, nullptr);
}

TEST(InvertedIndexTest, ConcurrentAddDocuments) {
    InvertedIndex idx;
    constexpr int kDocs = 100;
    std::vector<std::thread> threads;

    for (int i = 0; i < kDocs; ++i) {
        threads.emplace_back([&idx, i]() {
            auto meta = makeMeta(static_cast<uint64_t>(i + 1), "doc" + std::to_string(i) + ".txt");
            idx.addDocument(meta, {"word" + std::to_string(i % 10), "common"});
        });
    }
    for (auto& t : threads) t.join();

    EXPECT_EQ(idx.documentCount(), static_cast<std::size_t>(kDocs));
    auto commonPostings = idx.lookup("common");
    EXPECT_EQ(commonPostings.size(), static_cast<std::size_t>(kDocs));
}

TEST(InvertedIndexTest, NonExistentTokenLookup) {
    InvertedIndex idx;
    idx.addDocument(makeMeta(1, "a.txt"), {"hello"});
    auto postings = idx.lookup("nonexistent");
    EXPECT_TRUE(postings.empty());
}

TEST(InvertedIndexTest, AllDocuments) {
    InvertedIndex idx;
    idx.addDocument(makeMeta(1, "a.txt"), {"foo"});
    idx.addDocument(makeMeta(2, "b.txt"), {"bar"});
    auto docs = idx.allDocuments();
    EXPECT_EQ(docs.size(), 2u);
}
