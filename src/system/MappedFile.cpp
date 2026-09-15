#include "orderbook_core/system/MappedFile.hpp"

#include <filesystem>
#include <utility>

namespace fs = std::filesystem;

namespace rushevich::system {
MappedFile::MappedFile(const fs::path& path) {
    auto descriptor = ::open(path.c_str(), O_RDONLY);
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

    _handle = PosixFileHandle { std::move(descriptor) };
    _size = static_cast<size_t>(file_stats.st_size);
    _buf = start;
    // We are going to read through the data sequentially - this madvise call allows the OS to
    // ‘aggressively read ahead’ the pages that we need
    ::madvise(_buf, _size, MADV_SEQUENTIAL);
}

MappedFile::~MappedFile() { close(); }

MappedFile::MappedFile(MappedFile&& other) noexcept : _handle { std::move(other._handle) } {}

MappedFile& MappedFile::operator=(MappedFile&& other) noexcept {
    // Need to close current descriptor, and acquire the other
    if (this != &other) {
        _handle = std::move(other._handle);
    }
    return *this;
}

[[nodiscard]] std::span<const std::byte> MappedFile::data() const { return { _buf, _size }; }

void MappedFile::close() {
    _handle.reset();
    if (_buf != nullptr) {
        ::munmap(_buf, _size); // returns the mapping back to the OS
    }
}

PosixFileHandle::PosixFileHandle(int&& fd) noexcept : _fd { std::move(fd) } {}

PosixFileHandle::PosixFileHandle(PosixFileHandle&& other) noexcept : _fd { std::move(other._fd) } {}

PosixFileHandle& PosixFileHandle::operator=(PosixFileHandle&& other) noexcept {
    if (this != &other) {
        reset(other._fd);
    }
    return *this;
}

[[nodiscard]] bool PosixFileHandle::is_open() const noexcept { return _fd != invalid_fd; }
PosixFileHandle::~PosixFileHandle() noexcept { reset(); }

// Reset should close the current file descriptor if needed and then take on the value of the input
// argument
void PosixFileHandle::reset(int other) noexcept {
    if (_fd != invalid_fd) {
        ::close(_fd);
        _fd = other < 0 ? invalid_fd : other;
    }
}

} // namespace rushevich::system
