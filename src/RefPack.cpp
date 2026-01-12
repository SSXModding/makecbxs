#include "RefPack.h"

#include <bit>
#include <cassert>

#include "u24.hpp"

namespace refpack {

	template<std::endian Endian>
	constexpr u32 readU24FromPtr(const u8* in) {
		// a bit eldrich-horror, but this allows using the u24 type in the code already,
		// which is a bit better for code-sharing. Also means we can get rid of the utils helpers
		// which kind of suck anyways
		return static_cast<u32>(*reinterpret_cast<const u24Endian<Endian>*>(in));
	}

	// TODO: refpack compression

	Error size(const u8* pSrc, usize& size) {
		if(!pSrc)
			return InvalidArgument;


		if(pSrc[1] != 0xfb)
			return NotRefpack;

		std::uint16_t signature = ((pSrc[0] << 8) | pSrc[1]);
		pSrc += sizeof(std::uint16_t);

		// skip uint24 compressed size field
		if(signature & 0x0100)
			pSrc += 3;

		// read the uint24 decompressed size
		size = static_cast<usize>(readU24FromPtr<std::endian::big>(pSrc));
		return Ok;
	}

	Error decompressInto(const u8* pSrc, u8* pDst, usize dstLen) {
		// Command variables
		u8 first{};
		u8 second{};
		u8 third{};
		u8 fourth{};

		u32 procLen{};
		u32 refRun{};
		u8* pRef{nullptr};

		// perform null & 0 size check
		if(!pSrc)
			return InvalidArgument;

		if(pSrc[1] != 0xfb)
			return NotRefpack;

		auto signature = u16((pSrc[0] << 8) | pSrc[1]);
		pSrc += sizeof(u16);

		// skip uint24 compressed size field
		if(signature & 0x0100)
			pSrc += 3;

		// Read the uint24 decompressed size and verify that the destination length
		// matches it. If it doesn't match, we give up and don't enter the decompression loop.
		if(auto decompressedSize = readU24FromPtr<std::endian::big>(pSrc); decompressedSize != dstLen) {
			return InvalidSize;
		}

		pSrc += 3;

		while(true) {
			// Retrive the first command byte
			first = *pSrc++;

			if(!(first & 0x80)) {
				// 2-byte command: 0DDRRRPP DDDDDDDD
				second = *pSrc++;

				procLen = first & 0x03;

				for(u32 i = 0; i < procLen; i++)
					*pDst++ = *pSrc++;

				pRef = pDst - ((first & 0x60) << 3) - second - 1;
				refRun = ((first >> 2) & 0x07) + 3;

				for(u32 i = 0; i < refRun; ++i)
					*pDst++ = *pRef++;

			} else if(!(first & 0x40)) {
				// 3-byte command: 10RRRRRR PPDDDDDD DDDDDDDD
				second = *pSrc++;
				third = *pSrc++;

				procLen = second >> 6;

				for(u32 i = 0; i < procLen; ++i)
					*pDst++ = *pSrc++;

				pRef = pDst - ((second & 0x3f) << 8) - third - 1;
				refRun = (first & 0x3f) + 4;

				for(u32 i = 0; i < refRun; ++i)
					*pDst++ = *pRef++;

			} else if(!(first & 0x20)) {
				// 4-byte command: 110DRRPP DDDDDDDD DDDDDDDD RRRRRRRR
				second = *pSrc++;
				third = *pSrc++;
				fourth = *pSrc++;

				procLen = first & 0x03;

				for(u32 i = 0; i < procLen; ++i)
					*pDst++ = *pSrc++;

				pRef = pDst - ((first & 0x10) << 12) - (second << 8) - third - 1;
				refRun = ((first & 0x0c) << 6) + fourth + 5;

				for(u32 i = 0; i < refRun; ++i)
					*pDst++ = *pRef++;
			} else {
				// 1-byte command: 111PPPPP

				procLen = (first & 0x1f) * 4 + 4;

				if(procLen <= 0x70) {
					// no stop flag

					for(u32 i = 0; i < procLen; ++i)
						*pDst++ = *pSrc++;

				} else {
					// has a stop flag
					procLen = first & 0x3;

					for(u32 i = 0; i < procLen; ++i)
						*pDst++ = *pSrc++;

					break;
				}
			}
		}

		return Ok;
	}

} // namespace refpack
