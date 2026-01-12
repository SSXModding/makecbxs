#pragma once
#include <cstdint>

struct BxStreamChunkHeader {
    // magic constants (TODO: these are entirely endian specific so need to be
    // defined one way on LE, and another way on BE)
    constexpr static std::uint32_t CBXS = 0x53584243;
    constexpr static std::uint32_t CEND = 0x444E4543;

    // Either CBXS for a file chunk,
    // or CEND for a end of the file.
    std::uint32_t magic;

    // The compressed size of the chunk.
    std::uint32_t size;

    // operator to check if header is valid
    bool valid() const {
        return magic == CBXS ||  magic == CEND;
    }

    #ifdef DUMP
    void Dump() {
        if(magic == CEND)
            std::printf("- This chunk is a end chunk\n");
        std::printf(" - Chunk size (compressed): %08x bytes\n", size);
    }
    #endif

};

