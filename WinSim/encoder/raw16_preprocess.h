#ifndef RK3588_RAW16_PREPROCESS_H
#define RK3588_RAW16_PREPROCESS_H

#include <stddef.h>
#include <stdint.h>
#include <vector>

#include "encoder_types.h"

int AlignUp(int value, int align);
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
	std::vector<uint8_t>* nv12);

#endif
