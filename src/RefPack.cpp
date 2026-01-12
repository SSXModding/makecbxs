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
		return static_cast<u32>(*reinterpret_cast<const u24Endian<std::endian::big>*>(in));
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
		const std::uint8_t* in = pSrc;
		std::uint8_t* outptr = pDst;

		// Command variables
		std::uint8_t first;
		std::uint8_t second;
		std::uint8_t third;
		std::uint8_t fourth;

		std::uint32_t proc_len;
		std::uint32_t ref_run;
		std::uint8_t* ref_ptr;

		// perform null & 0 size check
		if(!in)
			return InvalidArgument;

		std::uint16_t signature = ((in[0] << 8) | in[1]);
		in += sizeof(std::uint16_t);

		// skip uint24 compressed size field
		if(signature & 0x0100)
			in += 3;

		// Read the uint24 decompressed size and verify that the destination length
		// matches it. If it doesn't match, we give up and don't enter the decompression loop.
		if(auto decompressedSize = readU24FromPtr<std::endian::big>(in); decompressedSize != dstLen) {
			return InvalidSize;
		}

		in += 3;

		while(true) {
			// Retrive the first command byte
			first = *in++;

			if(!(first & 0x80)) {
				// 2-byte command: 0DDRRRPP DDDDDDDD
				second = *in++;

				proc_len = first & 0x03;

				for(std::uint32_t i = 0; i < proc_len; i++)
					*outptr++ = *in++;

				ref_ptr = outptr - ((first & 0x60) << 3) - second - 1;
				ref_run = ((first >> 2) & 0x07) + 3;

				for(std::uint32_t i = 0; i < ref_run; ++i)
					*outptr++ = *ref_ptr++;

			} else if(!(first & 0x40)) {
				// 3-byte command: 10RRRRRR PPDDDDDD DDDDDDDD
				second = *in++;
				third = *in++;

				proc_len = second >> 6;

				for(std::uint32_t i = 0; i < proc_len; ++i)
					*outptr++ = *in++;

				ref_ptr = outptr - ((second & 0x3f) << 8) - third - 1;
				ref_run = (first & 0x3f) + 4;

				for(std::uint32_t i = 0; i < ref_run; ++i)
					*outptr++ = *ref_ptr++;

			} else if(!(first & 0x20)) {
				// 4-byte command: 110DRRPP DDDDDDDD DDDDDDDD RRRRRRRR
				second = *in++;
				third = *in++;
				fourth = *in++;

				proc_len = first & 0x03;

				for(std::uint32_t i = 0; i < proc_len; ++i)
					*outptr++ = *in++;

				ref_ptr = outptr - ((first & 0x10) << 12) - (second << 8) - third - 1;
				ref_run = ((first & 0x0c) << 6) + fourth + 5;

				for(std::uint32_t i = 0; i < ref_run; ++i)
					*outptr++ = *ref_ptr++;
			} else {
				// 1-byte command: 111PPPPP

				proc_len = (first & 0x1f) * 4 + 4;

				if(proc_len <= 0x70) {
					// no stop flag

					for(std::uint32_t i = 0; i < proc_len; ++i)
						*outptr++ = *in++;

				} else {
					// has a stop flag
					proc_len = first & 0x3;

					for(std::uint32_t i = 0; i < proc_len; ++i)
						*outptr++ = *in++;

					break;
				}
			}
		}

		return Ok;
	}

} // namespace refpack
