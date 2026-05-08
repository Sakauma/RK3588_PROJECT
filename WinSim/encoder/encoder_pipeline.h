#ifndef RK3588_ENCODER_PIPELINE_H
#define RK3588_ENCODER_PIPELINE_H

#include "encoder_types.h"
#include "frame_assembler.h"
#include "rk_mpp_encoder.h"

#include <stddef.h>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>

class EncoderPipeline
{
public:
	EncoderPipeline();
	~EncoderPipeline();

	bool Start(const EncoderPipelineConfig& config);
	void Stop();
	bool SubmitPacket(int dma_chan, const volatile unsigned char* payload, size_t payload_size);
	void SetOsdObjects(uint64_t frame_sequence, const std::vector<OsdObject>& objects);
	EncoderPipelineStats GetStats() const;
	std::string LastError() const;

private:
	void WorkerLoop();
	void EnqueueFrame(const RawFrame& frame);
	std::vector<OsdObject> ObjectsForFrame(uint64_t frame_sequence) const;

	EncoderPipelineConfig config_;
	FrameAssembler assembler_;
	RkMppEncoder encoder_;
	int hor_stride_;
	int ver_stride_;
	bool running_;
	bool stop_requested_;
	std::string last_error_;

	mutable std::mutex mutex_;
	std::condition_variable cond_;
	std::deque<RawFrame> queue_;
	std::vector<OsdObject> latest_osd_;
	EncoderPipelineStats stats_;
	std::thread worker_;
};

#endif
