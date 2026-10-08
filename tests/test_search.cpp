#include <gtest/gtest.h>
#include "SearchEngine.h"
#include "InvertedIndex.h"
#include "Tokenizer.h"
#include "FileMetadata.h"

static FileMetadata makeMeta(uint64_t id, const std::string& name) {
    FileMetadata m;
    m.id           = id;
    m.filename     = name;
    m.absolutePath = "/docs/" + name;
    m.extension    = "txt";
    m.fileSize     = 500;
    m.indexed      = true;
    return m;
}

class SearchEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Doc 1: machine learning
        index.addDocument(makeMeta(1, "ml.txt"),
            {"machine", "learning", "machine", "algorithm", "machine"});
        // Doc 2: deep learning
        index.addDocument(makeMeta(2, "dl.txt"),
            {"deep", "learning", "neural", "network", "learning"});
        // Doc 3: computer vision
        index.addDocument(makeMeta(3, "cv.txt"),
            {"computer", "vision", "image", "recognition"});
        // Doc 4: machine vision
        index.addDocument(makeMeta(4, "mv.txt"),
            {"machine", "vision", "detection"});
    }

    InvertedIndex index;
    Tokenizer     tokenizer{false, 2}; // no stop words for determinism
};

TEST_F(SearchEngineTest, EmptyQueryReturnsEmpty) {
    SearchEngine engine(index, tokenizer);
    auto results = engine.search("");
    EXPECT_TRUE(results.empty());
}

TEST_F(SearchEngineTest, QueryNotInIndex) {
    SearchEngine engine(index, tokenizer);
    auto results = engine.search("quantum physics");
    EXPECT_TRUE(results.empty());
}

TEST_F(SearchEngineTest, SingleTermSearch) {
    SearchEngine engine(index, tokenizer);
    auto results = engine.search("machine");
    ASSERT_FALSE(results.empty());
    // Doc 1 has machine appearing 3 times, should be ranked highest
    EXPECT_EQ(results[0].documentId, 1u);
}

TEST_F(SearchEngineTest, MultiTermSearch) {
    SearchEngine engine(index, tokenizer);
    auto results = engine.search("machine learning");
    ASSERT_FALSE(results.empty());
    // Doc 1 matches both terms with higher frequency
    EXPECT_EQ(results[0].documentId, 1u);
}

TEST_F(SearchEngineTest, ResultsAreSortedByScore) {
    SearchEngine engine(index, tokenizer);
    auto results = engine.search("learning");
    ASSERT_GE(results.size(), 2u);
    for (std::size_t i = 1; i < results.size(); ++i) {
        EXPECT_GE(results[i - 1].score, results[i].score);
    }
}

TEST_F(SearchEngineTest, MaxResultsLimitsOutput) {
    SearchEngine engine(index, tokenizer);
    auto results = engine.search("machine learning vision", 2);
    EXPECT_LE(results.size(), 2u);
}

TEST_F(SearchEngineTest, MatchedTermsAreCorrect) {
    SearchEngine engine(index, tokenizer);
    auto results = engine.search("machine vision");
    // Doc 4 matches both 'machine' and 'vision'
    bool foundDoc4 = false;
    for (const auto& r : results) {
        if (r.documentId == 4u) {
            foundDoc4 = true;
            EXPECT_EQ(r.matchedTerms.size(), 2u);
        }
    }
    EXPECT_TRUE(foundDoc4);
}

TEST_F(SearchEngineTest, EmptyIndexReturnsEmpty) {
    InvertedIndex emptyIdx;
    Tokenizer tok(false, 2);
    SearchEngine engine(emptyIdx, tok);
    auto results = engine.search("anything");
    EXPECT_TRUE(results.empty());
}

TEST_F(SearchEngineTest, ScoresArePositive) {
    SearchEngine engine(index, tokenizer);
    auto results = engine.search("machine learning");
    for (const auto& r : results) {
        EXPECT_GT(r.score, 0.0);
    }
}

TEST_F(SearchEngineTest, ExpandQueryFindsKnownTerms) {
    SearchEngine engine(index, tokenizer);
    auto expanded = engine.expandQuery("machine learning quantum");
    // 'quantum' not in index; 'machine' and 'learning' are
    EXPECT_EQ(expanded.size(), 2u);
}

TEST_F(SearchEngineTest, ResultContainsFilename) {
    SearchEngine engine(index, tokenizer);
    auto results = engine.search("machine");
    ASSERT_FALSE(results.empty());
    EXPECT_FALSE(results[0].filename.empty());
    EXPECT_FALSE(results[0].path.empty());
}

TEST_F(SearchEngineTest, SearchIsDeterministic) {
    SearchEngine engine(index, tokenizer);
    auto r1 = engine.search("machine learning");
    auto r2 = engine.search("machine learning");
    ASSERT_EQ(r1.size(), r2.size());
    for (std::size_t i = 0; i < r1.size(); ++i) {
        EXPECT_EQ(r1[i].documentId, r2[i].documentId);
        EXPECT_DOUBLE_EQ(r1[i].score, r2[i].score);
    }
}

TEST_F(SearchEngineTest, PrefixSearchMatchesIncompleteWord) {
    SearchEngine engine(index, tokenizer);
    // "alg" should match "algorithm" in Doc 1
    auto results = engine.search("alg");
    ASSERT_FALSE(results.empty());
    EXPECT_EQ(results[0].documentId, 1u);
}

TEST_F(SearchEngineTest, SingleCharQueryMatchesPrefix) {
    SearchEngine engine(index, tokenizer);
    // "a" should match "algorithm" in Doc 1
    auto results = engine.search("a");
    ASSERT_FALSE(results.empty());
    EXPECT_EQ(results[0].documentId, 1u);
}

