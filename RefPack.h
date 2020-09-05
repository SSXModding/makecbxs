#pragma once

#include <vector>
#include "types.h"
#include "span.h"

namespace refpack {

	/**
	 * Decompress a RefPack source.
	 * 
	 * \param[in] compressed Compressed span of data.
	 * \return A decompressed buffer.
	 */
	std::vector<byte> Decompress(Span<byte> compressed);

} // namespace refpack