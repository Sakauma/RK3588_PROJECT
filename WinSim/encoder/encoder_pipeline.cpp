#include "encoder_pipeline.h"

#include "osd_overlay.h"
#include "raw16_preprocess.h"

#include <algorithm>
#include <stdio.h>

EncoderPipeline::EncoderPipeline()
	: hor_stride_(0),
	  ver_stride_(0),
	  running_(false),
	  stop_requested_(false)
{
}

EncoderPipeline::~EncoderPipeline()
{
	Stop();
}

bool EncoderPipeline::Start(const EncoderPipelineConfig& config)
{
	Stop();

	if (config.width <= 0 || config.height <= 0 || config.fps <= 0 ||
		config.bitrate <= 0 || config.queue_depth <= 0)
	{
		last_error_ = "invalid encoder configuration";
		return false;
	}

	config_ = config;
	hor_stride_ = AlignUp(config_.width, 16);
	ver_stride_ = AlignUp(config_.height, 16);
	assembler_.Configure(config_.width, config_.height, config_.input_has_img_dma_header);

	if (!encoder_.Open(config_, hor_stride_, ver_stride_))
	{
		last_error_ = encoder_.LastError();
		return false;
	}

	{
		std::lock_guard<std::mutex> lock(mutex_);
		running_ = true;
		stop_requested_ = false;
		queue_.clear();
		latest_osd_.clear();
		stats_ = EncoderPipelineStats();
	}

	worker_ = std::thread(&EncoderPipeline::WorkerLoop, this);
	return true;
}

void EncoderPipeline::Stop()
{
	{
		std::lock_guard<std::mutex> lock(mutex_);
		stop_requested_ = true;
	}
	cond_.notify_all();
	if (worker_.joinable())
	{
		worker_.join();
	}
	encoder_.Close();

	std::lock_guard<std::mutex> lock(mutex_);
	running_ = false;
	stop_requested_ = false;
	queue_.clear();
}

bool EncoderPipeline::SubmitPacket(int dma_chan, const volatile unsigned char* payload, size_t payload_size)
{
	(void)dma_chan;
	if (payload == NULL || payload_size == 0)
	{
		return false;
	}

	std::vector<uint8_t> packet(payload_size);
	for (size_t i = 0; i < payload_size; ++i)
	{
		packet[i] = payload[i];
	}

	RawFrame frame;
	const bool has_frame = assembler_.PushPacket(packet.data(), packet.size(), &frame);
	{
		std::lock_guard<std::mutex> lock(mutex_);
		stats_.packets++;
	}

	if (has_frame)
	{
		EnqueueFrame(frame);
	}

	return true;
}

void EncoderPipeline::EnqueueFrame(const RawFrame& frame)
{
	std::lock_guard<std::mutex> lock(mutex_);
	if (!running_)
	{
		return;
	}

	while (static_cast<int>(queue_.size()) >= config_.queue_depth)
	{
		queue_.pop_front();
		stats_.frames_dropped++;
	}

	queue_.push_back(frame);
	stats_.frames_in++;
	cond_.notify_one();
}

void EncoderPipeline::SetOsdObjects(uint64_t frame_sequence, const std::vector<OsdObject>& objects)
{
	(void)frame_sequence;
	std::lock_guard<std::mutex> lock(mutex_);
	latest_osd_ = objects;
}

std::vector<OsdObject> EncoderPipeline::ObjectsForFrame(uint64_t frame_sequence) const
{
	(void)frame_sequence;
	std::lock_guard<std::mutex> lock(mutex_);
	if (config_.osd_test_enable)
	{
		return MakeTestOsdObjects(config_.width, config_.height);
	}
	return latest_osd_;
}

void EncoderPipeline::WorkerLoop()
{
	std::vector<uint8_t> nv12;

	for (;;)
	{
		RawFrame frame;
		{
			std::unique_lock<std::mutex> lock(mutex_);
			cond_.wait(lock, [this]() {
				return stop_requested_ || !queue_.empty();
			});

			if (stop_requested_ && queue_.empty())
			{
				break;
			}

			frame = queue_.front();
			queue_.pop_front();
		}

		const bool converted = Raw16ToNv12(frame.data.data(),
			frame.data.size(),
			config_.width,
			config_.height,
			hor_stride_,
			ver_stride_,
			config_.raw16_shift,
			config_.raw16_little_endian,
			&nv12);

		if (!converted)
		{
			std::lock_guard<std::mutex> lock(mutex_);
			stats_.encode_errors++;
			last_error_ = "RAW16 to NV12 conversion failed";
			continue;
		}

		if (config_.osd_enable && config_.osd_mode != "burned-in")
		{
			const std::vector<OsdObject> objects = ObjectsForFrame(frame.sequence);
			if (!objects.empty())
			{
				DrawOsdNv12(nv12.data(), config_.width, config_.height,
					hor_stride_, ver_stride_, objects);
			}
		}

		if (encoder_.EncodeFrame(nv12.data(), nv12.size(), frame.sequence))
		{
			std::lock_guard<std::mutex> lock(mutex_);
			stats_.frames_encoded++;
		}
		else
		{
			std::lock_guard<std::mutex> lock(mutex_);
			stats_.encode_errors++;
			last_error_ = encoder_.LastError();
		}
	}
}

EncoderPipelineStats EncoderPipeline::GetStats() const
{
	std::lock_guard<std::mutex> lock(mutex_);
	return stats_;
}

std::string EncoderPipeline::LastError() const
{
	std::lock_guard<std::mutex> lock(mutex_);
	return last_error_;
}
