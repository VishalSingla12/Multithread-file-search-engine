#include <gtest/gtest.h>
#include "FileScanner.h"
#include "FileMetadata.h"
#include <filesystem>
#include <fstream>
#include <vector>
#include <mutex>

namespace fs = std::filesystem;

// Helper: create a temporary directory with files
class TempDir {
public:
    TempDir() {
        path_ = fs::temp_directory_path() / ("mfse_test_" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()));
        fs::create_directories(path_);
    }
    ~TempDir() {
        std::error_code ec;
        fs::remove_all(path_, ec);
    }
    const fs::path& path() const { return path_; }
    void createFile(const std::string& name, const std::string& content = "test content") {
        std::ofstream f(path_ / name);
        f << content;
    }
    void createSubdir(const std::string& name) {
        fs::create_directories(path_ / name);
    }
    void createFileInSubdir(const std::string& dir, const std::string& file, const std::string& content = "") {
        std::ofstream f(path_ / dir / file);
        f << content;
    }
private:
    fs::path path_;
};

TEST(FileScannerTest, EmptyDirectory) {
    TempDir tmp;
    std::vector<FileMetadata> found;
    std::mutex mu;
    FileScanner scanner([&](FileMetadata m) {
        std::lock_guard<std::mutex> lk(mu);
        found.push_back(std::move(m));
    });
    auto count = scanner.scan(tmp.path());
    EXPECT_EQ(count, 0u);
    EXPECT_TRUE(found.empty());
}

TEST(FileScannerTest, SingleSupportedFile) {
    TempDir tmp;
    tmp.createFile("hello.txt", "hello world");
    std::vector<FileMetadata> found;
    std::mutex mu;
    FileScanner scanner([&](FileMetadata m) {
        std::lock_guard<std::mutex> lk(mu);
        found.push_back(std::move(m));
    });
    auto count = scanner.scan(tmp.path());
    EXPECT_EQ(count, 1u);
    ASSERT_EQ(found.size(), 1u);
    EXPECT_EQ(found[0].extension, "txt");
    EXPECT_EQ(found[0].filename, "hello.txt");
}

TEST(FileScannerTest, UnsupportedFileSkipped) {
    TempDir tmp;
    tmp.createFile("image.png", "not text");
    tmp.createFile("doc.pdf",   "pdf content");
    std::vector<FileMetadata> found;
    std::mutex mu;
    FileScanner scanner([&](FileMetadata m) {
        std::lock_guard<std::mutex> lk(mu);
        found.push_back(std::move(m));
    });
    auto count = scanner.scan(tmp.path());
    EXPECT_EQ(count, 0u);
    EXPECT_TRUE(found.empty());
}

TEST(FileScannerTest, RecursiveTraversal) {
    TempDir tmp;
    tmp.createSubdir("sub1");
    tmp.createSubdir("sub1/sub2");
    tmp.createFile("root.cpp", "int main(){}");
    tmp.createFileInSubdir("sub1", "helper.h", "void foo();");
    tmp.createFileInSubdir("sub1/sub2", "deep.py", "print('hello')");
    std::vector<FileMetadata> found;
    std::mutex mu;
    FileScanner scanner([&](FileMetadata m) {
        std::lock_guard<std::mutex> lk(mu);
        found.push_back(std::move(m));
    });
    auto count = scanner.scan(tmp.path());
    EXPECT_EQ(count, 3u);
    EXPECT_EQ(found.size(), 3u);
}

TEST(FileScannerTest, InvalidPathReturnsZero) {
    std::vector<FileMetadata> found;
    FileScanner scanner([&](FileMetadata m) { found.push_back(std::move(m)); });
    auto count = scanner.scan("/this/path/does/not/exist/ever");
    EXPECT_EQ(count, 0u);
    EXPECT_TRUE(found.empty());
}

TEST(FileScannerTest, IsSupportedExtension) {
    EXPECT_TRUE(FileScanner::isSupportedExtension("txt"));
    EXPECT_TRUE(FileScanner::isSupportedExtension("cpp"));
    EXPECT_TRUE(FileScanner::isSupportedExtension("py"));
    EXPECT_TRUE(FileScanner::isSupportedExtension("json"));
    EXPECT_FALSE(FileScanner::isSupportedExtension("png"));
    EXPECT_FALSE(FileScanner::isSupportedExtension("pdf"));
    EXPECT_FALSE(FileScanner::isSupportedExtension("exe"));
    EXPECT_FALSE(FileScanner::isSupportedExtension(""));
}

TEST(FileScannerTest, FileMetadataPopulated) {
    TempDir tmp;
    tmp.createFile("test.md", "# Title\nSome content here");
    FileMetadata captured;
    FileScanner scanner([&](FileMetadata m) { captured = std::move(m); });
    scanner.scan(tmp.path());
    EXPECT_EQ(captured.extension, "md");
    EXPECT_EQ(captured.filename, "test.md");
    EXPECT_GT(captured.fileSize, 0u);
    EXPECT_NE(captured.id, 0u);
    EXPECT_FALSE(captured.absolutePath.empty());
}

TEST(FileScannerTest, EmptyFileIsDiscovered) {
    TempDir tmp;
    tmp.createFile("empty.txt", "");
    std::vector<FileMetadata> found;
    std::mutex mu;
    FileScanner scanner([&](FileMetadata m) {
        std::lock_guard<std::mutex> lk(mu);
        found.push_back(std::move(m));
    });
    scanner.scan(tmp.path());
    // Empty files are discovered (scanner doesn't read content)
    EXPECT_EQ(found.size(), 1u);
}
