#include <filesystem>
#include <cstring>
#include <string>
#include <iostream>
#include <fstream>
#include <iomanip>

#include "RefPack.h"

#include "span.h"
#include "types.h"


namespace fs = std::filesystem;

// TODO: this should probably be derived by the file name?
constexpr static char chunk_prefix[] = "BAM_";

struct BxStreamingFileHeader {
	// magic constants (TODO: these are entirely endian specific so need to be 
	// defined one way on LE, and another way on BE)
	constexpr static uint32 CBXS = 0x53584243;
	constexpr static uint32 CEND = 0x444E4543;
	
	// Either CBXS for a file chunk,
	// or CEND for a end of the file.
	uint32 magic;

	// The compressed size of the chunk.
	uint32 size;

	// operator to check if header is valid
	operator bool() {
		return magic == BxStreamingFileHeader::CBXS || 
				magic == BxStreamingFileHeader::CEND;
	}

#ifdef DUMP
	void Dump() {
		if(magic == BxStreamingFileHeader::CEND)
			std::cout << " - End of file chunk\n";

		std::cout << " - Chunk size (compressed): " << size << " bytes\n";
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

	std::string output = argv[2];

	std::ifstream stream(argv[1], std::ifstream::binary);

	if(!stream) {
		std::cout << "Could not open " << std::quoted(argv[1]) << "!\n";
		return 1;
	}

	// file buffer we concat into
	// cleared after we write the file to avoid memory woes
	std::vector<byte> fileData;

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

		std::vector<byte> chunkBuffer(header.size);
		stream.read((char*)chunkBuffer.data(), header.size);

		std::vector<byte> refBuffer = refpack::Decompress(MakeSpan(chunkBuffer.data(), header.size));
		
#ifdef DUMP
		std::cout << " - Chunk size (decompressed): " << refBuffer.size() << " bytes\n";
#endif

		// append decompressed data
		fileData.insert(fileData.end(), refBuffer.begin(), refBuffer.end());

		if(endChunk) {
			auto outPath = path / (std::string(chunk_prefix) + std::to_string(fileNum) + ".bin");
			std::ofstream os(outPath.string(), std::ostream::binary);

			std::cout << " - Total file size: " << fileData.size() / 1000 << " KB\n";

			os.write((char*)fileData.data(), fileData.size());
			os.close();

			std::cout << "Wrote unchunked file to " << std::quoted(outPath.string()) << ".\n";
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

	std::cout << "All done!\n";
	return 0;
}