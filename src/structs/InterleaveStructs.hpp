#pragma once
#include <bit>
#include <cstdint>
#include "../u24.hpp"

/// This macro lists off all known interleaved data types.
#define INTERLEAVE_TYPES() \
    X(Material, 0x0) \
    X(Patch, 0x1) \
    X(WorldMdr, 0x2) \
    X(Instance, 0x3) \
    X(ParticleModel, 0x4) \
    X(ParticleInstance, 0x5) \
    X(Lights, 0x6) \
    X(Halo, 0x7) \
    X(Splines, 0x8) \
    X(Texture, 0x9) \
    X(Lightmap, 0xa) \
    X(VisCurtain, 0xb) \
    X(Collision, 0xc) \
    X(IcepickData, 0xd) \
    X(AIPath, 0xe) \
    X(WorldPainter, 0xf) \
    X(WScriptData, 0x10) \
    X(CameraTriggers, 0x11) \
    X(NisTable, 0x12) \
    X(Missions, 0x13) \
    X(AudioBank, 0x14) \
    X(Radar, 0x15) \
    X(Avalanche, 0x16)

// NOTE: Previously, makecbxs would use "invented" file names.
// Now that OT PSP has given us the debugging loose-loading code
// that the devs worked on the game with, we now prefer less-invented
// file names. For cases where we don't know the loose name,
// or the data isn't loaded loose even when debugging, invented names are still used.
#define KNOWN_EXTENSIONS() \
    X(Texture, ".shape") \
    X(Lightmap, ".shape") \
    X(IcepickData, ".adl") \
    X(AIPath, ".aip") \
    X(WScriptData, ".ssf") \
    X(AudioBank, ".bnk") \
    X(Avalanche, ".sav")

#define KNOWN_LOOSE_TYPES() \
    X(IcepickData) \
    X(AIPath) \
    X(WScriptData) \
    X(AudioBank) \
    X(Avalanche)

enum class InterleaveFileType : std::uint8_t {
#define X(key, value) \
    key = value,
    INTERLEAVE_TYPES()
#undef X
};

/// Converts an interleave type to string.
constexpr auto interleaveTypeToString(InterleaveFileType type) {
    switch(type) {
#define X(type, ext) case InterleaveFileType::type: return #type;
        INTERLEAVE_TYPES()
#undef X
        default:
            return "UNKNOWN TYPE";
    }
}

/// Gets the known file extension of a interleave type. Returns ".bin" for
/// types which don't have a known file extension.
constexpr auto interleaveFileExtension(InterleaveFileType type) {
    switch(type) {
#define X(type, ext) \
        case InterleaveFileType::type: return ext;
        KNOWN_EXTENSIONS()
        default:
            return ".bin";
#undef X
    }
    // just in case god dies
    return ".bin";
}

/// Returns true if a interleave type is known to be "loose".
/// Loose essentially means:
/// - One RID (with the section id, and UID always 0)
/// - One instance of the type in each world stream
constexpr bool interleaveTypeIsLoose(InterleaveFileType type) {
    switch(type) {
#define X(type) \
        case InterleaveFileType::type: return true;
        KNOWN_LOOSE_TYPES()
#undef X
        default:
            return false;
    }
}

struct tResourceID {
    std::uint32_t m_uiSectionID : 8;
    std::uint32_t m_uiUID		: 24;
};

/// Header for each file inside an interleave file.
template<std::endian Endian = std::endian::little>
struct BxInterleaveFileHeader {
    InterleaveFileType type;
    u24Endian<Endian> size;
    tResourceID ResourceID;

#ifdef DUMP
    void dump() {
        std::cout << "- Type " << (int)type << "\n";
        std::cout << "- Size " << static_cast<std::uint32_t>(size) << "\n";
        std::printf("- RID: %02x.%06x\n", (std::uint32_t)ResourceID.m_uiSectionID, (std::uint32_t)ResourceID.m_uiUID);
    }
#endif

    u8* data() {
        return reinterpret_cast<u8*>(this + 1);
    }

    auto* next() {
        return reinterpret_cast<BxInterleaveFileHeader<Endian>*>(
            reinterpret_cast<u8*>(this) + sizeof(*this) + static_cast<std::uint32_t>(size)
        );
    }
};

static_assert(sizeof(tResourceID) == 4 , "tResourceID don't works");
static_assert(sizeof(BxInterleaveFileHeader<>) == 8, "BxInterleaveFileHeader don't works");
