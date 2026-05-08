#include "frame_assembler.h"

#include <algorithm>
#include <string.h>

#pragma pack(push, 4)
struct ImgDmaHeaderCompat
{
	unsigned char ucPktType;
	unsigned char ucStEnd;
	unsigned short usPayloadLen;
	unsigned short usImgRows;
	unsigned short usImgCols;
	int nFrameCount;
	int nPktCount;
	unsigned int orgImgSeqNum;
	unsigned short spImgRow;
	unsigned short spImgCol;
	int nResv2[2];
};
#pragma pack(pop)

FrameAssembler::FrameAssembler()
	: width_(0),
	  height_(0),
	  input_has_img_dma_header_(false),
	  next_sequence_(0)
{
}

void FrameAssembler::Configure(int width, int height, bool input_has_img_dma_header)
{
	width_ = width;
	height_ = height;
	input_has_img_dma_header_ = input_has_img_dma_header;
	Reset();
}

void FrameAssembler::Reset()
{
	next_sequence_ = 0;
	pending_.clear();
}

size_t FrameAssembler::FrameBytes() const
{
	if (width_ <= 0 || height_ <= 0)
	{
		return 0;
	}
	return static_cast<size_t>(width_) * static_cast<size_t>(height_) * 2U;
}

bool FrameAssembler::PushPacket(const uint8_t* payload, size_t payload_size, RawFrame* out_frame)
{
	const uint8_t* frame_payload = payload;
	size_t frame_payload_size = payload_size;
	const size_t frame_bytes = FrameBytes();

	if (payload == NULL || out_frame == NULL || frame_bytes == 0)
	{
		return false;
	}

	if (input_has_img_dma_header_)
	{
		if (payload_size <= sizeof(ImgDmaHeaderCompat))
		{
			return false;
		}
		frame_payload += sizeof(ImgDmaHeaderCompat);
		frame_payload_size -= sizeof(ImgDmaHeaderCompat);
	}

	if (pending_.empty() && frame_payload_size == frame_bytes)
	{
		out_frame->sequence = next_sequence_++;
		out_frame->data.assign(frame_payload, frame_payload + frame_payload_size);
		return true;
	}

	size_t copied = 0;
	while (copied < frame_payload_size)
	{
		const size_t need = frame_bytes - pending_.size();
		const size_t take = std::min(need, frame_payload_size - copied);
		pending_.insert(pending_.end(), frame_payload + copied, frame_payload + copied + take);
		copied += take;

		if (pending_.size() == frame_bytes)
		{
			out_frame->sequence = next_sequence_++;
			out_frame->data.swap(pending_);
			pending_.clear();
			return true;
		}
	}

	return false;
}
