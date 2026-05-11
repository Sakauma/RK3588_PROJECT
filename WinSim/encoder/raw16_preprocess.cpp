#include "raw16_preprocess.h"

#include <algorithm>
#include <array>
#include <string.h>

namespace
{
uint16_t ReadRaw16Pixel(const uint8_t* pixel, bool little_endian)
{
	const uint8_t lo = pixel[0];
	const uint8_t hi = pixel[1];
	return little_endian ?
		static_cast<uint16_t>(lo | (hi << 8)) :
		static_cast<uint16_t>((lo << 8) | hi);
}

int ClampLevel(int value)
{
	return std::max(0, std::min(value, 65535));
}

void FindAutoWindow(const uint8_t* raw,
	int width,
	int height,
	bool little_endian,
	int low_clip_permille,
	int high_clip_permille,
	int* black_level,
	int* white_level)
{
	const size_t pixel_count = static_cast<size_t>(width) * static_cast<size_t>(height);
	std::array<uint32_t, 4096> histogram;
	histogram.fill(0);

	for (size_t i = 0; i < pixel_count; ++i)
	{
		const uint16_t value = ReadRaw16Pixel(raw + i * 2U, little_endian);
		histogram[value >> 4]++;
	}

	low_clip_permille = std::max(0, std::min(low_clip_permille, 400));
	high_clip_permille = std::max(0, std::min(high_clip_permille, 400));

	const uint64_t low_target = pixel_count * static_cast<size_t>(low_clip_permille) / 1000U;
	const uint64_t high_target = pixel_count * static_cast<size_t>(high_clip_permille) / 1000U;

	uint64_t cumulative = 0;
	int low_bin = 0;
	for (size_t i = 0; i < histogram.size(); ++i)
	{
		cumulative += histogram[i];
		if (cumulative > low_target)
		{
			low_bin = static_cast<int>(i);
			break;
		}
	}

	cumulative = 0;
	int high_bin = static_cast<int>(histogram.size() - 1U);
	for (int i = static_cast<int>(histogram.size()) - 1; i >= 0; --i)
	{
		cumulative += histogram[static_cast<size_t>(i)];
		if (cumulative > high_target)
		{
			high_bin = i;
			break;
		}
	}

	*black_level = ClampLevel(low_bin * 16);
	*white_level = ClampLevel(high_bin * 16 + 15);
	if (*white_level <= *black_level)
	{
		*black_level = 0;
		*white_level = 65535;
	}
}

uint8_t MapRaw16ToY8(uint16_t value, int shift, int black_level, int white_level)
{
	if (white_level <= black_level)
	{
		const uint32_t shifted = static_cast<uint32_t>(value >> shift);
		return static_cast<uint8_t>(std::min<uint32_t>(shifted, 255U));
	}

	if (value <= black_level)
	{
		return 0;
	}
	if (value >= white_level)
	{
		return 255;
	}

	const uint32_t range = static_cast<uint32_t>(white_level - black_level);
	const uint32_t scaled = (static_cast<uint32_t>(value - black_level) * 255U + range / 2U) / range;
	return static_cast<uint8_t>(std::min<uint32_t>(scaled, 255U));
}
}

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
	Raw16MappingMode mapping_mode,
	int black_level,
	int white_level,
	int auto_low_clip_permille,
	int auto_high_clip_permille,
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

	black_level = ClampLevel(black_level);
	white_level = ClampLevel(white_level);
	if (mapping_mode == Raw16MappingMode::AutoWindow)
	{
		FindAutoWindow(raw, width, height, little_endian,
			auto_low_clip_permille, auto_high_clip_permille,
			&black_level, &white_level);
	}
	else if (mapping_mode == Raw16MappingMode::Shift)
	{
		black_level = 0;
		white_level = 0;
	}

	nv12->assign(y_size + uv_size, 0x80);

	uint8_t* y_plane = nv12->data();
	for (int row = 0; row < height; ++row)
	{
		uint8_t* y_row = y_plane + static_cast<size_t>(row) * static_cast<size_t>(hor_stride);
		const uint8_t* raw_row = raw + static_cast<size_t>(row) * static_cast<size_t>(width) * 2U;
		for (int col = 0; col < width; ++col)
		{
			const uint16_t value = ReadRaw16Pixel(raw_row + col * 2, little_endian);
			y_row[col] = MapRaw16ToY8(value, shift, black_level, white_level);
		}
	}

	return true;
}

bool PackedNv12ToMppNv12(const uint8_t* packed_nv12,
	size_t packed_nv12_size,
	int width,
	int height,
	int hor_stride,
	int ver_stride,
	std::vector<uint8_t>* nv12)
{
	const size_t pixel_count = static_cast<size_t>(width) * static_cast<size_t>(height);
	const size_t packed_size = pixel_count * 3U / 2U;
	const size_t y_size = static_cast<size_t>(hor_stride) * static_cast<size_t>(ver_stride);
	const size_t uv_size = y_size / 2U;

	if (packed_nv12 == NULL || nv12 == NULL || width <= 0 || height <= 0 ||
		hor_stride < width || ver_stride < height || packed_nv12_size < packed_size)
	{
		return false;
	}

	nv12->assign(y_size + uv_size, 0x80);

	uint8_t* y_plane = nv12->data();
	uint8_t* uv_plane = y_plane + y_size;
	const uint8_t* src_y = packed_nv12;
	const uint8_t* src_uv = packed_nv12 + pixel_count;
	const int uv_height = height / 2;

	for (int row = 0; row < height; ++row)
	{
		memcpy(y_plane + static_cast<size_t>(row) * static_cast<size_t>(hor_stride),
			src_y + static_cast<size_t>(row) * static_cast<size_t>(width),
			static_cast<size_t>(width));
	}

	for (int row = 0; row < uv_height; ++row)
	{
		memcpy(uv_plane + static_cast<size_t>(row) * static_cast<size_t>(hor_stride),
			src_uv + static_cast<size_t>(row) * static_cast<size_t>(width),
			static_cast<size_t>(width));
	}

	return true;
}
