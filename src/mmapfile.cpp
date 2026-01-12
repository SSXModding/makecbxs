#include "mmapfile.hpp"
#include <filesystem>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cstdlib>
#include <cstring>

#include <cassert>

#include "Utils.hpp"


cMmapFile::cMmapFile() {
     fd = -1;
     size = 0;
     base = nullptr;
 }

cMmapFile::~cMmapFile() {
    closeFile();
}

bool cMmapFile::mapFile(const std::filesystem::path& path) {
    fd = open(path.string().c_str(), O_RDONLY);
    if(fd == -1)
        return false;
    // cache the size of the file
    lseek(fd, 0, SEEK_END);
    size = lseek(fd, 0, SEEK_CUR);
    lseek(fd, 0, SEEK_SET);

    // map the file into memory
    base = mmap(nullptr, size, PROT_READ, MAP_PRIVATE | MAP_POPULATE, fd, 0);
    if(base == MAP_FAILED) {
        close(fd);
        fd = -1;
        size = 0;
        base = nullptr;
        return false;
    }

    return true;
}

void cMmapFile::advise(std::size_t offset, std::size_t size, int advice) {
    madvise(reinterpret_cast<u8*>(base) + offset, size, advice);
}

void cMmapFile::closeFile() {
    if(fd == -1) {
        munmap(base, size);
        close(fd);
        fd = -1;
        size = 0;
        base = nullptr;
    }
}
