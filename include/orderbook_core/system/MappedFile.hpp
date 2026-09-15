#pragma once

#include <fcntl.h>
#include <filesystem>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

namespace fs = std::filesystem;
namespace rushevich::system {

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
    struct MappedFileHandle {
        static constexpr int invalid_fd = -1;
        int fd { invalid_fd };
        std::byte* begin { nullptr };
        size_t size {};
        [[nodiscard]] bool is_open() const { return fd != invalid_fd && begin != nullptr; }
        void close() const { ::close(fd); }
        void reset_fields() {
            fd = invalid_fd;
            begin = nullptr;
            size = 0;
        }
    };

    MappedFileHandle _handle;

    void close();
};

} // namespace rushevich::system
