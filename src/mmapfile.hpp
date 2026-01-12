#pragma once
#include <cstdint>
#include <filesystem>

// i roll this every day dude i've gotten used to it'
class cMmapFile {
    int fd;
    std::size_t size;
    void* base;
public:
    cMmapFile();

    ~cMmapFile();
    bool mapFile(const std::filesystem::path& path);

    void* getBase() const { return base; }
    std::size_t getSize() const { return size; }

    void advise(std::size_t offset, std::size_t size, int advice);

    void closeFile();

};
