#pragma once

#include <vector>
#include <modeco/types.h>
#include <modeco/span.h>

namespace refpack {

	/**
	 * Decompress a RefPack source.
	 * 
	 * \param[in] compressed Compressed span of data.
	 * \return A decompressed buffer.
	 */
	std::vector<mco::byte> Decompress(mco::Span<mco::byte> compressed);

} // namespace refpack