#ifndef RK3588_RAW16_PREPROCESS_H
#define RK3588_RAW16_PREPROCESS_H

#include <stddef.h>
#include <stdint.h>
#include <vector>

int AlignUp(int value, int align);
bool Raw16ToNv12(const uint8_t* raw,
	size_t raw_size,
	int width,
	int height,
	int hor_stride,
	int ver_stride,
	int shift,
	bool little_endian,
	std::vector<uint8_t>* nv12);

#endif
