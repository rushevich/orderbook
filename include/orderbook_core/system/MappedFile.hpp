#pragma once

#include <fcntl.h>
#include <filesystem>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

namespace fs = std::filesystem;
namespace rushevich::system {
class PosixFileHandle {
public:
    PosixFileHandle() = default;
    explicit PosixFileHandle(int&& fd) noexcept;
    ~PosixFileHandle() noexcept;

    PosixFileHandle(PosixFileHandle&& other) noexcept;
    PosixFileHandle& operator=(PosixFileHandle&& other) noexcept;

    PosixFileHandle(const PosixFileHandle&) = delete;
    PosixFileHandle& operator=(const PosixFileHandle&) = delete;

    [[nodiscard]] bool is_open() const noexcept;
    void reset(int other = invalid_fd) noexcept;

private:
    static constexpr int invalid_fd = -1;
    int _fd { invalid_fd };
    // std::byte* begin { nullptr };
    // size_t size {};
};

class MappedFile {
public:
    explicit MappedFile(const fs::path& path);

    ~MappedFile();

    [[nodiscard]] std::span<const std::byte> data() const;

    MappedFile(const MappedFile&) = delete;
    MappedFile operator=(const MappedFile&) = delete;

    MappedFile(MappedFile&& other) noexcept;
    MappedFile& operator=(MappedFile&& other) noexcept;

private:
    // TODO: Rewrite this to be semantically more like an actual RAII type

    PosixFileHandle _handle;

    std::byte* _buf { nullptr };
    size_t _size { 0 };
    void close();
};

} // namespace rushevich::system
