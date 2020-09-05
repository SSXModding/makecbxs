#include <filesystem>
#include <cstring>
#include <string>
#include <iostream>
#include <fstream>
#include <iomanip>
#include "RefPack.h"

namespace fs = std::filesystem;

constexpr static char chunk_prefix[] = "BAM_";

struct BxStreamingFileHeader {
	// Either CBXS for a file chunk,
	// or CEND for a end of the file.
	char magic[4];

	// The compressed size of the chunk.
	uint32 size;

	// hacky operator
	operator bool() {
		return size != 0;
	}

	void Dump() {
		std::cout << "Chunk information:\n";

		if(!std::strcmp(magic, "CEND"))
			std::cout << " - End of file chunk\n";
		else
			std::cout << " - Regular chunk\n";

		std::cout << " - Chunk size (compressed): " << std::hex << size << std::dec << " bytes\n";
	}
};

// this probably doesn't copy elide,
// i'm not sure
template <typename T>
inline T ReadUserType(std::istream& stream) {
	T temp {};

	stream.read((char*)&temp, sizeof(T));
	return temp;
}

void help(char* progname) {
	std::cout << progname << " usage: " << progname << " /path/to/BAM.[x]SB [OutputFolder]\n";
}

int main(int argc, char** argv) {
	if(argc < 3) {
		help(argv[0]);
		return 0;
	}

	std::cout << "BAM.[x]SB streaming file extractor\n";
	std::cout << "(C) 2020 Lily\n";

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

		header.Dump();

		if(!std::strncmp(header.magic, "CEND", 4))
			endChunk = true;

		nextpos += header.size;

		std::vector<byte> chunkBuffer(header.size);
		stream.read((char*)chunkBuffer.data(), header.size);

		std::vector<byte> refBuffer = refpack::Decompress(MakeSpan(chunkBuffer.data(), header.size));

		// append decompressed data
		fileData.insert(fileData.end(), refBuffer.begin(), refBuffer.end());

		if(endChunk) {
			auto outPath = path / (std::string(chunk_prefix) + std::to_string(fileNum) + ".bin");
			std::ofstream os(outPath.string(), std::ostream::binary);

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