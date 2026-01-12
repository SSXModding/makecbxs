#pragma once
#include "../Utils.hpp"

struct Vec4 {
    f32 x;
    f32 y;
    f32 z;
    f32 w;
};

struct tSDBLocation {
    char name[16];
    u32 numSubchunks;
    u32 numChunks;
    u32 posChunks;
    u32 posSubChunks;

    u16 unk[28];
};

struct tSDBHeader {
    u16 magic;
    u16 unk;
    f32 flVersion;

    u32 numLocations;
    u32 numChunks;
    u32 numSubchunks;

    char unk2[60];

    auto* getLocations() {
        return reinterpret_cast<tSDBLocation*>(this+1);
    }

    const tSDBLocation* getLocations() const {
        return reinterpret_cast<const tSDBLocation*>(this+1);
    }

    //auto* getChunkInfo() {
    //}

    const tSDBLocation* getLocationByChunkID(u32 chunkIndex) const {
        auto* pLocations = getLocations();
        if(numLocations == 1)
            return &pLocations[0];

        for(auto i = 0; i < numLocations; ++i) {
            auto endLocation = pLocations[i].posChunks;

            if(i == numLocations - 1) {
                endLocation = numChunks;
            } else {
                endLocation += pLocations[i+1].numChunks;
            }

            if(pLocations[i].posChunks <= chunkIndex && endLocation > chunkIndex)
                return &pLocations[i];
        }

        return nullptr;
    }
};

