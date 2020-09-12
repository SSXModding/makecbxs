#include <filesystem>
#include <cstring>
#include <string>
#include <iostream>
#include <fstream>
#include <iomanip>

#include "RefPack.h"

#include <modeco/Logger.h>
#include "stdout_sink.h"

#include <modeco/span.h>
#include <modeco/types.h>

static StdoutSink sink;

static mco::Logger logger = mco::Logger::CreateChannel("MakeCBXS");

namespace fs = std::filesystem;

// TODO: this should probably be derived by the file name?
constexpr static char chunk_prefix[] = "BAM_";

struct BxStreamingFileHeader {
	// magic constants (TODO: these are entirely endian specific so need to be 
	// defined one way on LE, and another way on BE)
	constexpr static mco::uint32 CBXS = 0x53584243;
	constexpr static mco::uint32 CEND = 0x444E4543;
	
	// Either CBXS for a file chunk,
	// or CEND for a end of the file.
	mco::uint32 magic;

	// The compressed size of the chunk.
	mco::uint32 size;

	// operator to check if header is valid
	operator bool() {
		return magic == BxStreamingFileHeader::CBXS || 
				magic == BxStreamingFileHeader::CEND;
	}

#ifdef DUMP
	void Dump() {
		if(magic == BxStreamingFileHeader::CEND)
			logger.info(" - This chunk is a end chunk");

		logger.info(" - Chunk size (compressed): ", size, " bytes");
	}
#endif

};

template <typename T>
inline T ReadUserType(std::istream& stream) {
	T temp {};

	stream.read((char*)&temp, sizeof(T));
	return temp;
}

int main(int argc, char** argv) {
	if(argc < 3) {
		std::cout << "BAM.[x]SB streaming file extractor tool\n";
		std::cout << "Usage: " << argv[0] << " /path/to/data/worlds/bam.[x]sb [OutputFolder]\n";
		return 0;
	}

	mco::Logger::SetSink(&sink);

	std::string output = argv[2];

	std::ifstream stream(argv[1], std::ifstream::binary);

	if(!stream) {
		logger.error("Could not open \"", argv[1], "\"!");
		return 1;
	}

	// file buffer we concat into
	// cleared after we write the file to avoid memory woes
	std::vector<mco::byte> fileData;

	// set to true when we encounter a CEND
	bool endChunk = false;

	std::size_t nextpos = 0;

	int fileNum = 0;

	auto path = fs::path(output);

	if(!fs::exists(path))
		fs::create_directory(path);

	while(true) {
		if(!stream)
			break;

		BxStreamingFileHeader header = ReadUserType<BxStreamingFileHeader>(stream);

		// dirty hack since ReadUserType default-constructs on error
		// could use optional<T> but I'm Lazy
		if(!header)
			break;

#ifdef DUMP
		header.Dump();
#endif

		if(header.magic == BxStreamingFileHeader::CEND)
			endChunk = true;

		nextpos += header.size;

		std::vector<mco::byte> chunkBuffer(header.size);
		stream.read((char*)chunkBuffer.data(), header.size);

		std::vector<mco::byte> refBuffer = refpack::Decompress(mco::MakeSpan(chunkBuffer.data(), header.size));
		
#ifdef DUMP
		logger.info(" - Chunk size (decompressed): ", refBuffer.size(), " bytes");
#endif

		// append decompressed data
		fileData.insert(fileData.end(), refBuffer.begin(), refBuffer.end());

		if(endChunk) {
			auto outPath = path / (std::string(chunk_prefix) + std::to_string(fileNum) + ".bin");
			std::ofstream os(outPath.string(), std::ostream::binary);

			logger.info(" - Total file size: ", fileData.size() / 1000, " KB");

			os.write((char*)fileData.data(), fileData.size());
			os.close();

			logger.info("Wrote decompressed and unchunked file to \"", outPath.string(), "\".");
			fileData.clear();
			++fileNum;
		}

		// Seek to the next position.
		stream.seekg(nextpos, std::ifstream::beg);

		// Reset stuff if we need to

		if(endChunk)
			endChunk = false;

		chunkBuffer.clear();
	}

	logger.info("All done!");
	return 0;
}