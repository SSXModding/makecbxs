#pragma once

#include <vector>
#include <span>
#include <cstdint>

#include "Utils.hpp"

namespace refpack {

	enum Error {
		Ok = 0,
		InvalidArgument = -1,
		InvalidSize = -2,
		NotRefpack = -3
	};

	/// Queries the decompressed size of a RefPack compressed data stream.
	Error size(const u8* pSrc, usize& outSize);

	/// Decompress Refpack data into a user-preallocated buffer.
	/// Size must match the decompressed size, which can be determined
	/// using the [size()] function.
	Error decompressInto(const u8* pSrc, u8* pDst, usize dstLen);

} // namespace refpack
