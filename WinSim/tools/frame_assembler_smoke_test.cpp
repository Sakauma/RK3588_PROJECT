#include "../encoder/frame_assembler.h"

#include <algorithm>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

static void FillGray10Le16(std::vector<uint8_t>* frame, int width, int height)
{
	frame->resize(static_cast<size_t>(width) * static_cast<size_t>(height) * 2U);
	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			const uint16_t value = static_cast<uint16_t>((x + y) & 0x3ff);
			const size_t off = (static_cast<size_t>(y) * static_cast<size_t>(width) + x) * 2U;
			(*frame)[off] = static_cast<uint8_t>(value & 0xff);
			(*frame)[off + 1] = static_cast<uint8_t>(value >> 8);
		}
	}
}

int main(int argc, char** argv)
{
	int width = 1920;
	int height = 1080;
	size_t packet_bytes = 4096;

	if (argc > 1)
	{
		width = atoi(argv[1]);
	}
	if (argc > 2)
	{
		height = atoi(argv[2]);
	}
	if (argc > 3)
	{
		packet_bytes = static_cast<size_t>(strtoul(argv[3], NULL, 10));
	}

	if (width <= 0 || height <= 0 || packet_bytes == 0)
	{
		fprintf(stderr, "invalid arguments\n");
		return 1;
	}

	std::vector<uint8_t> frame;
	FillGray10Le16(&frame, width, height);

	FrameAssembler assembler;
	assembler.Configure(width, height, InputPixelFormat::Gray10Le16, false);

	RawFrame out;
	size_t offset = 0;
	size_t packets = 0;
	while (offset < frame.size())
	{
		const size_t take = std::min(packet_bytes, frame.size() - offset);
		const bool has_frame = assembler.PushPacket(frame.data() + offset, take, &out);
		offset += take;
		packets++;

		if (offset < frame.size() && has_frame)
		{
			fprintf(stderr, "frame completed too early after packet %zu\n", packets);
			return 1;
		}
	}

	if (out.sequence != 0 || out.data.size() != frame.size() ||
		memcmp(out.data.data(), frame.data(), frame.size()) != 0)
	{
		fprintf(stderr, "reassembled frame mismatch: sequence=%llu size=%zu expected=%zu\n",
			static_cast<unsigned long long>(out.sequence),
			out.data.size(),
			frame.size());
		return 1;
	}

	printf("PASS: assembled %dx%d gray10le16 frame from %zu packets (%zu bytes)\n",
		width,
		height,
		packets,
		frame.size());
	return 0;
}
