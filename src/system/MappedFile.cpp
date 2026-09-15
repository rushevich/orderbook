#include "orderbook_core/system/MappedFile.hpp"

#include <filesystem>

namespace fs = std::filesystem;

namespace rushevich::system {
MappedFile::MappedFile(const fs::path& path) {
    const auto descriptor = ::open(path.c_str(), O_RDONLY);
    if (descriptor == -1) {
        throw std::system_error(errno, std::system_category(), "unable to open file for reading");
    }
    struct stat file_stats {};
    if (fstat(descriptor, &file_stats) == -1) {
        throw std::system_error(errno, std::system_category(), "unable to fetch file stats");
    }

    auto* const start = static_cast<std::byte*>(::mmap(
        nullptr, static_cast<size_t>(file_stats.st_size), PROT_READ, MAP_PRIVATE, descriptor, 0));
    if (start == MAP_FAILED) {
        throw std::system_error(errno, std::system_category(), "failed to map file");
    }

    _handle = MappedFileHandle { .fd = descriptor,
                                 .begin = start,
                                 .size = static_cast<size_t>(file_stats.st_size) };

    // We are going to read through the data sequentially - this madvise call allows the OS to
    // ‘aggressively read ahead’ the pages that we need
    ::madvise(_handle.begin, _handle.size, MADV_SEQUENTIAL);
}

MappedFile::~MappedFile() { close(); }

MappedFile::MappedFile(MappedFile&& other) noexcept : _handle { other._handle } {}

MappedFile& MappedFile::operator=(MappedFile&& other) noexcept {
    if (this != &other) {
        _handle.close();
        _handle = other._handle;
        other._handle.reset_fields();
    }
    return *this;
}

[[nodiscard]] std::span<const std::byte> MappedFile::data() const {
    return { _handle.begin, _handle.size };
}
void MappedFile::close() {
    if (_handle.is_open()) {
        _handle.close();
        if (auto* ptr = _handle.begin; ptr != nullptr) {
            ::munmap(ptr, _handle.size); // returns the mapping back to the OS
        }
    }
}

} // namespace rushevich::system
