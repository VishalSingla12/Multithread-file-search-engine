#include <gtest/gtest.h>
#include "Tokenizer.h"

class TokenizerTest : public ::testing::Test {
protected:
    Tokenizer tokWithStopWords{true, 2};
    Tokenizer tokNoStopWords{false, 2};
};

TEST_F(TokenizerTest, EmptyStringReturnsEmpty) {
    auto tokens = tokWithStopWords.tokenize("");
    EXPECT_TRUE(tokens.empty());
}

TEST_F(TokenizerTest, BasicTokenization) {
    auto tokens = tokNoStopWords.tokenize("Hello World");
    ASSERT_EQ(tokens.size(), 2u);
    EXPECT_EQ(tokens[0], "hello");
    EXPECT_EQ(tokens[1], "world");
}

TEST_F(TokenizerTest, LowercasesInput) {
    auto tokens = tokNoStopWords.tokenize("MachINE LeaRNING");
    ASSERT_EQ(tokens.size(), 2u);
    EXPECT_EQ(tokens[0], "machine");
    EXPECT_EQ(tokens[1], "learning");
}

TEST_F(TokenizerTest, RemovesPunctuation) {
    auto tokens = tokNoStopWords.tokenize("hello, world! foo.");
    ASSERT_EQ(tokens.size(), 3u);
    EXPECT_EQ(tokens[0], "hello");
    EXPECT_EQ(tokens[1], "world");
    EXPECT_EQ(tokens[2], "foo");
}

TEST_F(TokenizerTest, StopWordRemoval) {
    auto tokens = tokWithStopWords.tokenize("the quick brown fox");
    // 'the' is a stop word
    for (const auto& t : tokens) {
        EXPECT_NE(t, "the");
    }
    EXPECT_FALSE(tokens.empty());
}

TEST_F(TokenizerTest, NoStopWordRemoval) {
    auto tokens = tokNoStopWords.tokenize("the quick brown fox");
    bool hasThe = false;
    for (const auto& t : tokens) if (t == "the") hasThe = true;
    EXPECT_TRUE(hasThe);
}

TEST_F(TokenizerTest, MinLengthFiltering) {
    Tokenizer tok(false, 4);
    auto tokens = tok.tokenize("a ab abc abcd");
    // Only 'abcd' has length >= 4
    ASSERT_EQ(tokens.size(), 1u);
    EXPECT_EQ(tokens[0], "abcd");
}

TEST_F(TokenizerTest, DuplicateTokensPreserved) {
    auto tokens = tokNoStopWords.tokenize("hello hello hello");
    EXPECT_EQ(tokens.size(), 3u);
}

TEST_F(TokenizerTest, UniqueTokensDeduplicates) {
    auto tokens = tokNoStopWords.uniqueTokens("hello hello hello world");
    EXPECT_EQ(tokens.size(), 2u);
}

TEST_F(TokenizerTest, CppStyleCode) {
    auto tokens = tokNoStopWords.tokenize("std::vector<int> v; // comment");
    // Should tokenise without crash and return some tokens
    EXPECT_FALSE(tokens.empty());
}

TEST_F(TokenizerTest, SpecialCppToken) {
    auto tokens = tokNoStopWords.tokenize("c++");
    ASSERT_FALSE(tokens.empty());
    EXPECT_EQ(tokens[0], "c++");
}

TEST_F(TokenizerTest, AddCustomStopWord) {
    Tokenizer tok(true, 2);
    tok.addStopWord("machine");
    EXPECT_TRUE(tok.isStopWord("machine"));
    auto tokens = tok.tokenize("machine learning");
    for (const auto& t : tokens) EXPECT_NE(t, "machine");
}

TEST_F(TokenizerTest, WhitespaceOnlyReturnsEmpty) {
    auto tokens = tokNoStopWords.tokenize("   \t\n   ");
    EXPECT_TRUE(tokens.empty());
}

TEST_F(TokenizerTest, NumbersAreTokens) {
    auto tokens = tokNoStopWords.tokenize("version 42 build 100");
    bool has42 = false;
    for (const auto& t : tokens) if (t == "42") has42 = true;
    EXPECT_TRUE(has42);
}
