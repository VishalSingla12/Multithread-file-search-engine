#include "FileMetadata.h"
#include <filesystem>
#include <chrono>
#include <algorithm>

FileMetadata::FileMetadata(const std::filesystem::path& path, uint64_t docId)
    : id(docId)
    , filename(path.filename().string())
    , absolutePath(std::filesystem::weakly_canonical(path).string())
{
    std::string ext = path.extension().string();
    if (!ext.empty() && ext[0] == '.') ext = ext.substr(1);
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    extension = std::move(ext);

    std::error_code ec;
    fileSize = std::filesystem::file_size(path, ec);
    if (ec) fileSize = 0;

    auto lwt = std::filesystem::last_write_time(path, ec);
    if (!ec) {
        using namespace std::chrono;
        auto sctp = time_point_cast<system_clock::duration>(
            lwt - std::filesystem::file_time_type::clock::now() +
            system_clock::now());
        lastModified = duration_cast<seconds>(sctp.time_since_epoch()).count();
    }
}
