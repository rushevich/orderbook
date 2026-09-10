#pragma once
#include <cerrno>
#include <cstddef>
#include <fcntl.h>
#include <filesystem>
#include <sys/mman.h>
#include <sys/stat.h>
#include <system_error>
#include <unistd.h>

namespace fs = std::filesystem;
namespace reader {
namespace detail {
struct MappedFileHandle {
    static constexpr int invalid_fd = -1;
    int fd { invalid_fd };
    std::byte* begin { nullptr };
    size_t size {};
    [[nodiscard]] bool is_open() const { return fd != invalid_fd && begin != nullptr; }
    void close() const { ::close(fd); }
};

// MappedFile represents a file that is mapped into the virtual address space of the process using a
// call to ‘mmap’
class MappedFile {
public:
    explicit MappedFile(const fs::path& path) {
        const auto descriptor = ::open(path.c_str(), O_RDONLY);
        if (descriptor == -1) {
            throw std::system_error(errno, std::system_category(),
                                    "unable to open file for reading");
        }
        struct stat file_stats {};
        if (fstat(descriptor, &file_stats) == -1) {
            throw std::system_error(errno, std::system_category(), "unable to fetch file stats");
        }

        auto* const start
            = static_cast<std::byte*>(::mmap(nullptr, static_cast<size_t>(file_stats.st_size),
                                             PROT_READ, MAP_PRIVATE, descriptor, 0));
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

    ~MappedFile() { close(); }

    [[nodiscard]] std::span<const std::byte> data() const {
        return { _handle.begin, _handle.size };
    }

    MappedFile(const MappedFile&) = delete;
    MappedFile operator=(const MappedFile&) = delete;

    MappedFile(MappedFile&& other) noexcept : _handle { other._handle } {}
    MappedFile& operator=(MappedFile&& other) noexcept {
        if (this != &other) {
            close();
            _handle = other._handle;
        }
        return *this;
    }

private:
    MappedFileHandle _handle;

    void close() {
        if (_handle.is_open()) {
            _handle.close();
            if (auto* ptr = _handle.begin; ptr != nullptr) {
                ::munmap(ptr, _handle.size); // returns the mapping back to the OS
            }
        }
    }
};
} // namespace detail
// The reader is designed to manage advancing a message buffer that comes from a file
class FileReader {
public:
    explicit FileReader(fs::path&& path);

private:
    detail::MappedFile _file;
};
} // namespace reader
