#include <filesystem>
#include <cstring>
#include <string>
#include <iostream>
#include <fstream>
#include <iomanip>

#include "RefPack.h"

#include <modeco/Logger.h>
#include <modeco/IostreamLoggerSink.h>

#include <modeco/span.h>
#include <modeco/types.h>

#define DUMP

namespace fs = std::filesystem;

static mco::IostreamLoggerSink sink;
static mco::Logger logger = mco::Logger::CreateLogger("MakeCBXS");

#include "Structs.h"

// TODO: this should probably be derived by the file name?
constexpr static char chunk_prefix[] = "BAM_";


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
	std::vector<char> fileData;

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
			auto interleaveDir = path / (std::string(chunk_prefix) + std::to_string(fileNum) + "_uninterleaved");
			
			fs::create_directories(interleaveDir);
			
			
			std::ofstream os(outPath.string(), std::ostream::binary);

			logger.info(" - Total file size: ", fileData.size() / 1000, " KB");

			os.write((char*)fileData.data(), fileData.size());
			os.close();

			logger.info("Wrote decompressed and unchunked (non-unstreamed) file to \"", outPath.string(), "\".");
			fileData.clear();
			++fileNum;
			
			// For usability's sake, also write out uninterleaved data
			// I wish there was a better way to do this..
			std::ifstream ifs(outPath.string(), std::istream::binary);
			BxInterleaveFileHeader interleave;
	
			while(interleave.type != (mco::byte)InterleaveFileType::End) {
				if(!ifs)
					break;
				
				interleave.Read(ifs);
		
#ifdef DUMP
				interleave.Dump();
#endif		
		
				interleave.ReadData(ifs, interleaveDir);		
			}
			
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
