#pragma once
#include "orderbook_core/system/MappedFile.hpp"

#include <fcntl.h>
#include <filesystem>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

namespace fs = std::filesystem;
namespace rushevich::feed {

// MappedFile represents a file that is mapped into the virtual address space of the process using a
// call to ‘mmap’
// The reader is designed to manage advancing a message buffer that comes from a file
class FileReader {
public:
    explicit FileReader(fs::path&& path);

private:
    system::MappedFile _file;
};
} // namespace rushevich::feed
