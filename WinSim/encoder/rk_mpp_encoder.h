#ifndef RK3588_RK_MPP_ENCODER_H
#define RK3588_RK_MPP_ENCODER_H

#include "encoder_types.h"
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <vector>

class RkMppEncoder
{
public:
	RkMppEncoder();
	~RkMppEncoder();

	bool Open(const EncoderPipelineConfig& config, int hor_stride, int ver_stride);
	bool EncodeFrame(const uint8_t* nv12, size_t nv12_size, uint64_t pts);
	void Close();
	std::string LastError() const;
	bool Main10Active() const;
	std::string Main10FallbackReason() const;

private:
	bool WriteBytes(const void* data, size_t size);
	bool WriteHeader();

	EncoderPipelineConfig config_;
	int hor_stride_;
	int ver_stride_;
	FILE* output_;
	std::string last_error_;
	bool main10_active_;
	std::string main10_fallback_reason_;

#ifdef HAVE_RKMPP
	void* ctx_;
	void* mpi_;
	void* cfg_;
	void* buf_group_;
#endif
};

#endif
