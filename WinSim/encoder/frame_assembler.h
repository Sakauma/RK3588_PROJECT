#ifndef RK3588_FRAME_ASSEMBLER_H
#define RK3588_FRAME_ASSEMBLER_H

#include "encoder_types.h"
#include <stddef.h>
#include <stdint.h>
#include <vector>

class FrameAssembler
{
public:
	FrameAssembler();

	void Configure(int width, int height, bool input_has_img_dma_header);
	bool PushPacket(const uint8_t* payload, size_t payload_size, RawFrame* out_frame);
	void Reset();

private:
	size_t FrameBytes() const;

	int width_;
	int height_;
	bool input_has_img_dma_header_;
	uint64_t next_sequence_;
	std::vector<uint8_t> pending_;
};

#endif
