#include "raw16_preprocess.h"

#include <algorithm>
#include <string.h>

int AlignUp(int value, int align)
{
	if (align <= 0)
	{
		return value;
	}
	return (value + align - 1) / align * align;
}

bool Raw16ToNv12(const uint8_t* raw,
	size_t raw_size,
	int width,
	int height,
	int hor_stride,
	int ver_stride,
	int shift,
	bool little_endian,
	std::vector<uint8_t>* nv12)
{
	const size_t raw_pixels = static_cast<size_t>(width) * static_cast<size_t>(height);
	const size_t y_size = static_cast<size_t>(hor_stride) * static_cast<size_t>(ver_stride);
	const size_t uv_size = y_size / 2U;

	if (raw == NULL || nv12 == NULL || width <= 0 || height <= 0 ||
		hor_stride < width || ver_stride < height || raw_size < raw_pixels * 2U)
	{
		return false;
	}

	if (shift < 0)
	{
		shift = 0;
	}
	if (shift > 15)
	{
		shift = 15;
	}

	nv12->assign(y_size + uv_size, 0x80);

	uint8_t* y_plane = nv12->data();
	for (int row = 0; row < height; ++row)
	{
		uint8_t* y_row = y_plane + static_cast<size_t>(row) * static_cast<size_t>(hor_stride);
		const uint8_t* raw_row = raw + static_cast<size_t>(row) * static_cast<size_t>(width) * 2U;
		for (int col = 0; col < width; ++col)
		{
			const uint8_t lo = raw_row[col * 2];
			const uint8_t hi = raw_row[col * 2 + 1];
			const uint16_t value = little_endian ?
				static_cast<uint16_t>(lo | (hi << 8)) :
				static_cast<uint16_t>((lo << 8) | hi);
			const uint32_t shifted = static_cast<uint32_t>(value >> shift);
			y_row[col] = static_cast<uint8_t>(std::min<uint32_t>(shifted, 255U));
		}
	}

	return true;
}
