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
//#define DUMP_RAW_INTERLEAVES

namespace fs = std::filesystem;

// structs
#include "structs/LocationStructs.hpp"
#include "structs/StreamStructs.hpp"
#include "structs/InterleaveStructs.hpp"

template<std::endian Endian>
void writeInterleave(const BxInterleaveFileHeader<Endian>& header, u8* pData, const tSDBLocation& location, const fs::path& Output) {
	if(header.size == 0) {
		return;
	}

	auto ridFormatted = std::format("RID_{:02x}_{:06x}_Type_{}", (std::uint32_t)header.ResourceID.m_uiSectionID, (std::uint32_t)header.ResourceID.m_uiUID, interleaveTypeToString(header.type));

	// Loose files get the name of the section only, since they only have one resource instance.
	if(static_cast<u32>(header.ResourceID.m_uiUID) == 0x0 && interleaveTypeIsLoose(header.type)) {
		ridFormatted = std::format("{}", location.name);
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

int main(int argc, char** argv) {
	if(argc < 3) {
		std::cout << "Rudimentary uninterleave for SSX 3 BAM\n";
		std::cout << "Usage: " << argv[0] << " /path/to/data/worlds/bam.[x]sb [OutputFolder]\n";
		return 0;
	}

	fs::path firstPath{argv[1]};
	fs::path sdbPath{firstPath};

	// Open the SDB, which is the location database.
	sdbPath.replace_extension(".sdb");
	auto sdbMap = cMmapFile{};

	if(!sdbMap.mapFile(sdbPath)) {
		std::printf("Couldn't open the location file @ \"%s\".\n", sdbPath.string().c_str());
		return 1;
	}

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

	// file buffer we concatenate decompressed chunks into
	// these are allocated outside of the loop so that we don't
	// keep reallocating, which wastes time & memory
	std::vector<u8> fileData;
	std::vector<u8> chunkBuffer;
	std::vector<u8> refBuffer;


	std::size_t nextpos = 0;
	u32 fileNum = 0;
	u32 chunkIndex = 0;

	if(!fs::exists(output))
		fs::create_directory(output);

	while(true) {
		if(!stream)
			break;

		auto header = ReadUserType<BxStreamChunkHeader>(stream);
		if(!header.valid()) {
			std::printf("Invalid stream RIFF header???\n");
			return 1;
		}

#ifdef DUMP
		header.Dump();
#endif

		nextpos += header.size;

		chunkBuffer.resize(header.size);
		stream.read((char*)chunkBuffer.data(), header.size);

		usize refSize{};
		if(auto res = refpack::size(chunkBuffer.data(), refSize); res != refpack::Ok) {
			std::printf("Error when trying to query Refpack size: %d. Shouldn't happen!\n", res);
			return 1;
		}

		// Decompress the RefPack data
		refBuffer.resize(refSize);
		if(auto res = refpack::decompressInto(chunkBuffer.data(), refBuffer.data(), refSize); res != refpack::Ok) {
			std::printf("Error when trying to decompress Refpack data: %d. Shouldn't happen!\n", res);
			return 1;
		}
		
#ifdef DUMP
		std::printf(" - Chunk %d size (decompressed): %d bytes\n", chunkIndex, refBuffer.size());
#endif

		// append decompressed data
		fileData.insert(fileData.end(), refBuffer.begin(), refBuffer.end());

		// If the chunk we read is a CEND, it indicates the end of a interleave stream.
		if(header.magic == BxStreamChunkHeader::CEND) {
			auto* pLocation = pSdbLocationData->getLocationByChunkID(fileNum);
			auto interleaveDir = output;

#ifdef DUMP_RAW_INTERLEAVES
			auto outPath = output;
#endif

			if(pLocation == nullptr) {
				interleaveDir /= std::format("UnknownLocation{:02x}", fileNum);
#ifdef DUMP_RAW_INTERLEAVES
				outPath /= std::format("UnknownLocation{:02x}.interleave", fileNum);
#endif
			} else {
				interleaveDir /= std::format("{}", pLocation->name);
#ifdef DUMP_RAW_INTERLEAVES
				outPath /= std::format("{}.interleave", pLocation->name);
#endif
			}

			fs::create_directories(interleaveDir);

#ifdef DUMP_RAW_INTERLEAVES
			std::ofstream os(outPath.string(), std::ostream::binary);
			os.write((char*)fileData.data(), fileData.size());
			os.close();
#endif

			// Uninterleave the data we decompressed and unchunked.
			uninterleaveStream(fileData.data(), fileData.size(), *pLocation, interleaveDir);

			// Reset file data without deallocating it.
			fileData.resize(0);
			fileNum++;
		}

		// Seek to the next position.
		stream.seekg(nextpos, std::ifstream::beg);
		chunkIndex++;
	}

	return 0;
}
