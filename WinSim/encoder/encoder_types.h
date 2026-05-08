#ifndef RK3588_ENCODER_TYPES_H
#define RK3588_ENCODER_TYPES_H

#include <stdint.h>
#include <string>
#include <vector>

enum class VideoCodec
{
	H264,
	H265
};

enum class Raw16MappingMode
{
	Shift,
	Window,
	AutoWindow
};

struct YuvColor
{
	uint8_t y;
	uint8_t u;
	uint8_t v;
};

struct OsdObject
{
	int x;
	int y;
	int w;
	int h;
	std::string label;
	YuvColor color;
	int line_width;
};

struct EncoderPipelineConfig
{
	int width = 1920;
	int height = 1080;
	int fps = 30;
	int bitrate = 8000000;
	int queue_depth = 3;
	int raw16_shift = 8;
	int raw16_black_level = 0;
	int raw16_white_level = 65535;
	int raw16_auto_low_clip_permille = 5;
	int raw16_auto_high_clip_permille = 5;
	bool raw16_little_endian = true;
	bool input_has_img_dma_header = false;
	bool osd_enable = true;
	bool osd_test_enable = false;
	Raw16MappingMode raw16_mapping_mode = Raw16MappingMode::Shift;
	VideoCodec codec = VideoCodec::H265;
	std::string osd_mode = "auto";
	std::string output_path = "/tmp/rk3588_capture.h265";
};

struct RawFrame
{
	uint64_t sequence = 0;
	std::vector<uint8_t> data;
};

struct EncoderPipelineStats
{
	uint64_t packets = 0;
	uint64_t frames_in = 0;
	uint64_t frames_encoded = 0;
	uint64_t frames_dropped = 0;
	uint64_t encode_errors = 0;
};

#endif
