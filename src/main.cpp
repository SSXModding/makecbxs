#include <filesystem>
#include <cstring>
#include <string>
#include <iostream>
#include <fstream>
#include <iomanip>

#include "RefPack.h"
#include "Utils.hpp"
#include "mmapfile.hpp"

// Options
//#define DUMP

namespace fs = std::filesystem;

// structs
#include "structs/LocationStructs.hpp"
#include "structs/StreamStructs.hpp"
#include "structs/InterleaveStructs.hpp"

template<std::endian Endian>
void writeInterleave(const BxInterleaveFileHeader<Endian>& header, u8* pData, const tSDBLocation& location, const fs::path& Output) {
	auto ridFormatted = std::format("RID_{:02x}_{:06x}_Type_{}", (std::uint32_t)header.ResourceID.m_uiSectionID, (std::uint32_t)header.ResourceID.m_uiUID, interleaveTypeToString(header.type));

	// Loose files get the name of the section only, since they only have one resource instance.
	if(interleaveTypeIsLoose(header.type)) {
		// a bit hacky
		if(static_cast<u32>(header.ResourceID.m_uiUID) == 0x0) {
			ridFormatted = std::format("{}", location.name);
		} else {
			ridFormatted = std::format("{}_{:d}", location.name, static_cast<u32>(header.ResourceID.m_uiUID));
		}
	}

	auto outPath = Output / (ridFormatted);
	outPath.replace_extension(interleaveFileExtension(header.type));

	// do the dance
	{
		std::ofstream os(outPath.string(), std::ostream::binary);
		os.write(reinterpret_cast<char*>(pData), static_cast<std::uint32_t>(header.size));
	}

	std::cout << "- Wrote uninterleaved file to " << outPath.string() << '\n';
}

template<std::endian Endian = std::endian::little>
void uninterleaveStream(u8* pData, std::size_t dataSize, const tSDBLocation& location, const fs::path& outDir) {
	auto pInterleave = reinterpret_cast<BxInterleaveFileHeader<Endian>*>(pData);
	std::size_t trackedDataSize = 0;
	while(trackedDataSize < dataSize) {
#ifdef DUMP
		pInterleave->dump();
#endif
		writeInterleave(*pInterleave, pInterleave->data(), location, outDir);
		trackedDataSize += static_cast<std::uint32_t>(pInterleave->size) + sizeof(BxInterleaveFileHeader<Endian>);
		pInterleave = pInterleave->next();
	}
}

template<std::endian Endian = std::endian::little>
int decompressChunks(u8* pChunkData, usize chunkDataSize, tSDBHeader* pSDBHeader, const fs::path& outDir) {
	// file buffers we work with
	// they're allocated outside of the loop so that we don't
	// keep reallocating, which wastes time & memory
	std::vector<u8> fileData;
	std::vector<u8> refBuffer;

	std::size_t nextpos = 0;
	u32 fileNum = 0;
	u32 chunkIndex = 0;

	if(!fs::exists(outDir))
		fs::create_directory(outDir);

	usize trackedSize = 0;
	auto* pHeader = reinterpret_cast<BxStreamChunkHeader*>(pChunkData);

	while(trackedSize < chunkDataSize) {
		if(!pHeader->valid()) {
			std::printf("Invalid stream RIFF header???\n");
			return 1;
		}

#ifdef DUMP
		pHeader->Dump();
#endif

		usize refSize{};
		if(auto res = refpack::size(pHeader->data(), refSize); res != refpack::Ok) {
			std::printf("Error when trying to query Refpack size: %d. Shouldn't happen!\n", res);
			return 1;
		}

		// Decompress the RefPack data
		refBuffer.resize(refSize);
		if(auto res = refpack::decompressInto(pHeader->data(), refBuffer.data(), refSize); res != refpack::Ok) {
			std::printf("Error when trying to decompress Refpack data: %d. Shouldn't happen!\n", res);
			return 1;
		}

#ifdef DUMP
		std::printf(" - Chunk %d size (decompressed): %d bytes\n", chunkIndex, refBuffer.size());
#endif

		// append decompressed data
		fileData.insert(fileData.end(), refBuffer.begin(), refBuffer.end());

		// If the chunk we read is a CEND, it indicates the end of a interleave stream.
		if(pHeader->magic == BxStreamChunkHeader::CEND) {
			auto* pLocation = pSDBHeader->getLocationByChunkID(fileNum);
			assert(pLocation != nullptr && "shouldn't ever be null");

			auto interleaveDir = outDir / pLocation->name;

			fs::create_directories(interleaveDir);

			// Uninterleave the data we decompressed and unchunked.
			uninterleaveStream<Endian>(fileData.data(), fileData.size(), *pLocation, interleaveDir);

			// Reset file data without deallocating it.
			fileData.resize(0);
			fileNum++;
		}

		// Go to the next chunk.
		pHeader = pHeader->next();
		trackedSize += pHeader->size;
		chunkIndex++;
	}

	return 0;
}

int main(int argc, char** argv) {
	if(argc < 3) {
		std::cout << "Rudimentary uninterleave for SSX 3 BAM\n";
		std::cout << "Usage: " << argv[0] << " /path/to/data/worlds/bam.[x]sb [OutputFolder]\n";
		return 0;
	}

	fs::path ssbPath{argv[1]};
	fs::path sdbPath{ssbPath};

	sdbPath.replace_extension(".sdb");
	auto sdbMap = cMmapFile{};
	auto ssbMap = cMmapFile{};

	// Map both the stream file and the location database files into memory.
	if(!ssbMap.mapFile(ssbPath)) {
		std::printf("Couldn't open the stream file @ \"%s\".\n", ssbPath.string().c_str());
		return 1;
	}

	if(!sdbMap.mapFile(sdbPath)) {
		std::printf("Couldn't open the location file @ \"%s\".\n", sdbPath.string().c_str());
		return 1;
	}

	// Open the SDB, which is the location database, and make sure it's valid.
	auto* pSdbLocationData = reinterpret_cast<tSDBHeader*>(sdbMap.getBase());
	if(pSdbLocationData->magic != 0x3800) {
		std::printf("Invalid location database.\n");
		return 1;
	}

	auto output = fs::path(argv[2]);
	std::ifstream stream(argv[1], std::ifstream::binary);

	if(!stream) {
		std::printf("Could not open \"", argv[1], "\"!");
		return 1;
	}

	return decompressChunks<std::endian::little>(reinterpret_cast<u8*>(ssbMap.getBase()), ssbMap.getSize(), pSdbLocationData, output);
}
