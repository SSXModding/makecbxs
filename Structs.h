#pragma once
#include <modeco/types.h>

template <typename T>
inline T ReadUserType(std::istream& stream) {
	T temp {};

	stream.read((char*)&temp, sizeof(T));
	return temp;
}

mco::uint32 Read24LE(std::istream& stream) {
		mco::byte buf[3];
		
		for(int i = 0; i < 3; ++i)
			buf[i] = ReadUserType<mco::byte>(stream);
		
		return ((buf[2] << 16) | (buf[1] << 8) | buf[0]);
}

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


// TODO Doxygen

enum InterleaveFileType : mco::byte {
	MDRPossible = 2,
	Something,
	Shape = 9,
	End = 22
};

// The files in a unchunked CBXS stream file have this header beforehand
// so that they all can be interleaved
struct BxInterleaveFileHeader {
	mco::byte type;        // this is probably some sort of type identifier?
	mco::uint32 size;       // the size of the file (counting at the first byte of the file), is a u24 internally
	mco::uint32 unknown;    // unknown, but its USUALLY not the same per file

	// todo use the logger

#ifdef DUMP
	void Dump() {
		std::cout << "- Type " << (int)type << "\n";
		std::cout << "- Size " << size << "\n";
		std::cout << "- Unknown (ID)? " <<  unknown << "\n";
	}
#endif
	
	void ReadData(std::istream& stream, fs::path Output) {
		std::vector<uint8_t> data(size);
		stream.read((char*)data.data(), size);
		
		auto outPath = Output / (std::to_string(unknown) + "_" + std::to_string(type) + ".bin");
		std::ofstream os(outPath.string(), std::ostream::binary);
		
		std::cout << "- Writing uninterleaved data to " << outPath.string() << '\n';
		
		os.write((char*)data.data(), size);
	}
	
	void Read(std::istream& stream) {
		if(!stream)
			return;
		
		type = ReadUserType<mco::byte>(stream);
		size = Read24LE(stream);
		unknown = ReadUserType<mco::uint32>(stream);
	}
};