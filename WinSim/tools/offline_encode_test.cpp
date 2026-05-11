#include "../encoder/encoder_types.h"
#include "../encoder/osd_overlay.h"
#include "../encoder/raw16_preprocess.h"
#include "../encoder/rk_mpp_encoder.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

static void FillRaw16Gradient(std::vector<uint8_t>* raw, int width, int height,
	int frame_index, InputPixelFormat input_pixel_format)
{
	raw->resize(static_cast<size_t>(width) * static_cast<size_t>(height) * 2U);
	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			uint16_t value = 0;
			if (input_pixel_format == InputPixelFormat::Gray10Le16)
			{
				value = static_cast<uint16_t>((x + frame_index * 4) & 0x3FF);
			}
			else
			{
				value = static_cast<uint16_t>(((x + frame_index * 4) & 0xFF) << 8);
			}
			size_t off = (static_cast<size_t>(y) * static_cast<size_t>(width) + x) * 2U;
			(*raw)[off] = static_cast<uint8_t>(value & 0xFF);
			(*raw)[off + 1] = static_cast<uint8_t>(value >> 8);
		}
	}
}

int main(int argc, char** argv)
{
	EncoderPipelineConfig cfg;
	int frame_count = 60;

	if (argc > 1)
	{
		cfg.width = atoi(argv[1]);
	}
	if (argc > 2)
	{
		cfg.height = atoi(argv[2]);
	}
	if (argc > 3)
	{
		cfg.output_path = argv[3];
	}
	if (argc > 4)
	{
		frame_count = atoi(argv[4]);
	}
	if (argc > 5 && (strcmp(argv[5], "gray10le16") == 0 || strcmp(argv[5], "gray10") == 0))
	{
		cfg.input_pixel_format = InputPixelFormat::Gray10Le16;
		cfg.prefer_main10 = true;
		cfg.raw16_shift = 2;
		cfg.raw16_white_level = 1023;
	}

	cfg.codec = VideoCodec::H265;
	cfg.osd_test_enable = true;
	cfg.output_path = cfg.output_path.empty() ? "/tmp/offline_raw16_osd.h265" : cfg.output_path;

	const int hor_stride = AlignUp(cfg.width, 16);
	const int ver_stride = AlignUp(cfg.height, 16);

	RkMppEncoder encoder;
	if (!encoder.Open(cfg, hor_stride, ver_stride))
	{
		fprintf(stderr, "encoder open failed: %s\n", encoder.LastError().c_str());
		return 1;
	}

	std::vector<uint8_t> raw;
	std::vector<uint8_t> nv12;
	std::vector<OsdObject> objects = MakeTestOsdObjects(cfg.width, cfg.height);

	for (int i = 0; i < frame_count; ++i)
	{
		FillRaw16Gradient(&raw, cfg.width, cfg.height, i, cfg.input_pixel_format);
		if (!Raw16ToNv12(raw.data(), raw.size(), cfg.width, cfg.height,
				hor_stride, ver_stride, cfg.raw16_shift, true,
				cfg.raw16_mapping_mode,
				cfg.raw16_black_level,
				cfg.raw16_white_level,
				cfg.raw16_auto_low_clip_permille,
				cfg.raw16_auto_high_clip_permille,
				&nv12))
		{
			fprintf(stderr, "RAW16 conversion failed at frame %d\n", i);
			return 1;
		}

		DrawOsdNv12(nv12.data(), cfg.width, cfg.height, hor_stride, ver_stride, objects);
		if (!encoder.EncodeFrame(nv12.data(), nv12.size(), i))
		{
			fprintf(stderr, "encode failed at frame %d: %s\n", i, encoder.LastError().c_str());
			return 1;
		}
	}

	encoder.Close();
	printf("wrote %d frames to %s\n", frame_count, cfg.output_path.c_str());
	return 0;
}
