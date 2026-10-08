#include <gtest/gtest.h>
#include "IndexManager.h"
#include "SearchEngine.h"
#include <filesystem>
#include <fstream>
#include <chrono>

namespace fs = std::filesystem;

class TempDir2 {
public:
    TempDir2() {
        path_ = fs::temp_directory_path() / ("mfse_persist_" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()));
        fs::create_directories(path_);
    }
    ~TempDir2() {
        std::error_code ec;
        fs::remove_all(path_, ec);
    }
    const fs::path& path() const { return path_; }
    fs::path filePath(const std::string& name) const { return path_ / name; }
    void createFile(const std::string& name, const std::string& content) {
        std::ofstream f(path_ / name);
        f << content;
    }
private:
    fs::path path_;
};

TEST(PersistenceTest, SaveAndLoadEmptyIndex) {
    TempDir2 tmp;
    auto idxPath = tmp.filePath("empty.bin");

    IndexManager mgr1(1);
    EXPECT_TRUE(mgr1.saveIndex(idxPath));

    IndexManager mgr2(1);
    EXPECT_TRUE(mgr2.loadIndex(idxPath));
    EXPECT_EQ(mgr2.index().documentCount(), 0u);
}

TEST(PersistenceTest, SaveAndLoadIndexWithFiles) {
    TempDir2 docDir;
    docDir.createFile("doc1.txt", "machine learning algorithms");
    docDir.createFile("doc2.txt", "deep neural networks");
    docDir.createFile("doc3.cpp", "int main() { return 0; }");

    TempDir2 tmp;
    auto idxPath = tmp.filePath("index.bin");

    IndexManager mgr1(1);
    std::size_t indexed = mgr1.indexDirectory(docDir.path());
    ASSERT_GT(indexed, 0u);
    EXPECT_TRUE(mgr1.saveIndex(idxPath));

    IndexManager mgr2(1);
    EXPECT_TRUE(mgr2.loadIndex(idxPath));
    EXPECT_EQ(mgr2.index().documentCount(), mgr1.index().documentCount());
}

TEST(PersistenceTest, SearchAfterLoad) {
    TempDir2 docDir;
    docDir.createFile("ml.txt", "machine learning is fascinating");
    docDir.createFile("other.txt", "unrelated content about cooking");

    TempDir2 tmp;
    auto idxPath = tmp.filePath("search_test.bin");

    IndexManager mgr1(1);
    mgr1.indexDirectory(docDir.path());
    mgr1.saveIndex(idxPath);

    IndexManager mgr2(1);
    mgr2.loadIndex(idxPath);

    SearchEngine engine(mgr2.index(), mgr2.tokenizer());
    auto results = engine.search("machine learning");
    ASSERT_FALSE(results.empty());
    EXPECT_EQ(results[0].filename, "ml.txt");
}

TEST(PersistenceTest, LoadNonExistentFileFails) {
    IndexManager mgr(1);
    EXPECT_FALSE(mgr.loadIndex("/this/does/not/exist.bin"));
}

TEST(PersistenceTest, LoadCorruptFileFails) {
    TempDir2 tmp;
    auto badPath = tmp.filePath("corrupt.bin");
    {
        std::ofstream f(badPath, std::ios::binary);
        f << "this is not a valid index file at all!!!!";
    }
    IndexManager mgr(1);
    EXPECT_FALSE(mgr.loadIndex(badPath));
}

TEST(PersistenceTest, ClearAfterLoad) {
    TempDir2 docDir;
    docDir.createFile("test.txt", "some content");

    TempDir2 tmp;
    auto idxPath = tmp.filePath("clear_test.bin");

    IndexManager mgr(1);
    mgr.indexDirectory(docDir.path());
    mgr.saveIndex(idxPath);
    mgr.loadIndex(idxPath);
    EXPECT_GT(mgr.index().documentCount(), 0u);
    mgr.clearIndex();
    EXPECT_EQ(mgr.index().documentCount(), 0u);
}
