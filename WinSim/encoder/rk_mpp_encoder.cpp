#include "rk_mpp_encoder.h"

#include <string.h>
#include <vector>

#ifdef HAVE_RKMPP
#include "rk_mpi.h"
#include "mpp_buffer.h"
#include "mpp_frame.h"
#include "mpp_packet.h"
#include "rk_venc_cfg.h"
#endif

RkMppEncoder::RkMppEncoder()
	: hor_stride_(0),
	  ver_stride_(0),
	  output_(NULL),
	  main10_active_(false)
#ifdef HAVE_RKMPP
	  ,
	  ctx_(NULL),
	  mpi_(NULL),
	  cfg_(NULL),
	  buf_group_(NULL)
#endif
{
}

RkMppEncoder::~RkMppEncoder()
{
	Close();
}

std::string RkMppEncoder::LastError() const
{
	return last_error_;
}

bool RkMppEncoder::Main10Active() const
{
	return main10_active_;
}

std::string RkMppEncoder::Main10FallbackReason() const
{
	return main10_fallback_reason_;
}

bool RkMppEncoder::WriteBytes(const void* data, size_t size)
{
	if (output_ == NULL || data == NULL || size == 0)
	{
		return true;
	}
	return fwrite(data, 1, size, output_) == size;
}

bool RkMppEncoder::Open(const EncoderPipelineConfig& config, int hor_stride, int ver_stride)
{
	Close();
	config_ = config;
	hor_stride_ = hor_stride;
	ver_stride_ = ver_stride;
	main10_active_ = false;
	main10_fallback_reason_.clear();

	output_ = fopen(config_.output_path.c_str(), "wb");
	if (output_ == NULL)
	{
		last_error_ = "failed to open output file";
		return false;
	}

#ifndef HAVE_RKMPP
	last_error_ = "Rockchip MPP headers/library were not found at build time";
	Close();
	return false;
#else
	MppCtx ctx = NULL;
	MppApi* mpi = NULL;
	MppEncCfg cfg = NULL;
	MppBufferGroup group = NULL;
	MppCodingType coding = (config_.codec == VideoCodec::H264) ?
		MPP_VIDEO_CodingAVC : MPP_VIDEO_CodingHEVC;
	const bool request_main10 = config_.prefer_main10 &&
		config_.input_pixel_format == InputPixelFormat::Gray10Le16 &&
		config_.codec == VideoCodec::H265;
	if (request_main10)
	{
		main10_fallback_reason_ =
			"MPP Main10 encoder input was requested, but this build uses NV12 input; falling back to 8-bit H.265 preview";
	}

	MPP_RET ret = mpp_create(&ctx, &mpi);
	if (ret != MPP_OK)
	{
		last_error_ = "mpp_create failed";
		return false;
	}

	ret = mpp_init(ctx, MPP_CTX_ENC, coding);
	if (ret != MPP_OK)
	{
		last_error_ = "mpp_init encoder failed";
		mpp_destroy(ctx);
		return false;
	}

	ret = mpp_enc_cfg_init(&cfg);
	if (ret != MPP_OK)
	{
		last_error_ = "mpp_enc_cfg_init failed";
		mpp_destroy(ctx);
		return false;
	}

	mpi->control(ctx, MPP_ENC_GET_CFG, cfg);
	mpp_enc_cfg_set_s32(cfg, "prep:width", config_.width);
	mpp_enc_cfg_set_s32(cfg, "prep:height", config_.height);
	mpp_enc_cfg_set_s32(cfg, "prep:hor_stride", hor_stride_);
	mpp_enc_cfg_set_s32(cfg, "prep:ver_stride", ver_stride_);
	mpp_enc_cfg_set_s32(cfg, "prep:format", MPP_FMT_YUV420SP);
	mpp_enc_cfg_set_s32(cfg, "rc:mode", MPP_ENC_RC_MODE_CBR);
	mpp_enc_cfg_set_s32(cfg, "rc:fps_in_flex", 0);
	mpp_enc_cfg_set_s32(cfg, "rc:fps_in_num", config_.fps);
	mpp_enc_cfg_set_s32(cfg, "rc:fps_in_denorm", 1);
	mpp_enc_cfg_set_s32(cfg, "rc:fps_out_flex", 0);
	mpp_enc_cfg_set_s32(cfg, "rc:fps_out_num", config_.fps);
	mpp_enc_cfg_set_s32(cfg, "rc:fps_out_denorm", 1);
	mpp_enc_cfg_set_s32(cfg, "rc:gop", config_.fps * 2);
	mpp_enc_cfg_set_s32(cfg, "rc:bps_target", config_.bitrate);
	mpp_enc_cfg_set_s32(cfg, "rc:bps_max", config_.bitrate * 17 / 16);
	mpp_enc_cfg_set_s32(cfg, "rc:bps_min", config_.bitrate * 15 / 16);
	mpp_enc_cfg_set_s32(cfg, "codec:type", coding);
	if (config_.codec == VideoCodec::H264)
	{
		mpp_enc_cfg_set_s32(cfg, "h264:profile", 100);
		mpp_enc_cfg_set_s32(cfg, "h264:level", 40);
		mpp_enc_cfg_set_s32(cfg, "h264:cabac_en", 1);
	}

	ret = mpi->control(ctx, MPP_ENC_SET_CFG, cfg);
	if (ret != MPP_OK)
	{
		last_error_ = "MPP_ENC_SET_CFG failed";
		mpp_enc_cfg_deinit(cfg);
		mpp_destroy(ctx);
		return false;
	}

	ret = mpp_buffer_group_get_internal(&group, MPP_BUFFER_TYPE_DRM);
	if (ret != MPP_OK)
	{
		last_error_ = "mpp_buffer_group_get_internal failed";
		mpp_enc_cfg_deinit(cfg);
		mpp_destroy(ctx);
		return false;
	}

	ctx_ = ctx;
	mpi_ = mpi;
	cfg_ = cfg;
	buf_group_ = group;

	return WriteHeader();
#endif
}

bool RkMppEncoder::WriteHeader()
{
#ifndef HAVE_RKMPP
	return false;
#else
	MppCtx ctx = static_cast<MppCtx>(ctx_);
	MppApi* mpi = static_cast<MppApi*>(mpi_);
	MppPacket packet = NULL;
	std::vector<uint8_t> header_buffer(64U * 1024U);

	if (config_.codec != VideoCodec::H264 && config_.codec != VideoCodec::H265)
	{
		return true;
	}

	if (mpp_packet_init(&packet, header_buffer.data(), header_buffer.size()) != MPP_OK)
	{
		last_error_ = "mpp_packet_init header failed";
		return false;
	}

	MPP_RET ret = mpi->control(ctx, MPP_ENC_GET_HDR_SYNC, packet);
	if (ret != MPP_OK)
	{
		mpp_packet_deinit(&packet);
		last_error_ = "MPP_ENC_GET_HDR_SYNC failed";
		return false;
	}

	void* ptr = mpp_packet_get_pos(packet);
	size_t len = mpp_packet_get_length(packet);
	const bool ok = WriteBytes(ptr, len);
	mpp_packet_deinit(&packet);
	if (!ok)
	{
		last_error_ = "failed to write encoder header";
	}
	return ok;
#endif
}

bool RkMppEncoder::EncodeFrame(const uint8_t* nv12, size_t nv12_size, uint64_t pts)
{
#ifndef HAVE_RKMPP
	(void)nv12;
	(void)nv12_size;
	(void)pts;
	last_error_ = "Rockchip MPP is unavailable";
	return false;
#else
	if (ctx_ == NULL || mpi_ == NULL || buf_group_ == NULL || nv12 == NULL)
	{
		last_error_ = "encoder is not open";
		return false;
	}

	const size_t frame_size = static_cast<size_t>(hor_stride_) * static_cast<size_t>(ver_stride_) * 3U / 2U;
	if (nv12_size < frame_size)
	{
		last_error_ = "NV12 input buffer is smaller than configured frame";
		return false;
	}

	MppCtx ctx = static_cast<MppCtx>(ctx_);
	MppApi* mpi = static_cast<MppApi*>(mpi_);
	MppBufferGroup group = static_cast<MppBufferGroup>(buf_group_);
	MppBuffer buffer = NULL;
	MppFrame frame = NULL;
	MppPacket packet = NULL;

	MPP_RET ret = mpp_buffer_get(group, &buffer, frame_size);
	if (ret != MPP_OK)
	{
		last_error_ = "mpp_buffer_get failed";
		return false;
	}

	memcpy(mpp_buffer_get_ptr(buffer), nv12, frame_size);
	mpp_buffer_sync_end(buffer);

	ret = mpp_frame_init(&frame);
	if (ret != MPP_OK)
	{
		last_error_ = "mpp_frame_init failed";
		mpp_buffer_put(buffer);
		return false;
	}

	mpp_frame_set_width(frame, config_.width);
	mpp_frame_set_height(frame, config_.height);
	mpp_frame_set_hor_stride(frame, hor_stride_);
	mpp_frame_set_ver_stride(frame, ver_stride_);
	mpp_frame_set_fmt(frame, MPP_FMT_YUV420SP);
	mpp_frame_set_pts(frame, static_cast<RK_S64>(pts));
	mpp_frame_set_buffer(frame, buffer);

	ret = mpi->encode_put_frame(ctx, frame);
	mpp_frame_deinit(&frame);
	mpp_buffer_put(buffer);
	if (ret != MPP_OK)
	{
		last_error_ = "encode_put_frame failed";
		return false;
	}

	ret = mpi->encode_get_packet(ctx, &packet);
	if (ret != MPP_OK)
	{
		last_error_ = "encode_get_packet failed";
		return false;
	}

	if (packet != NULL)
	{
		void* ptr = mpp_packet_get_pos(packet);
		size_t len = mpp_packet_get_length(packet);
		const bool ok = WriteBytes(ptr, len);
		mpp_packet_deinit(&packet);
		if (!ok)
		{
			last_error_ = "failed to write encoded packet";
			return false;
		}
	}

	return true;
#endif
}

void RkMppEncoder::Close()
{
	main10_active_ = false;
#ifdef HAVE_RKMPP
	if (cfg_ != NULL)
	{
		mpp_enc_cfg_deinit(static_cast<MppEncCfg>(cfg_));
		cfg_ = NULL;
	}
	if (buf_group_ != NULL)
	{
		mpp_buffer_group_put(static_cast<MppBufferGroup>(buf_group_));
		buf_group_ = NULL;
	}
	if (ctx_ != NULL)
	{
		mpp_destroy(static_cast<MppCtx>(ctx_));
		ctx_ = NULL;
		mpi_ = NULL;
	}
#endif
	if (output_ != NULL)
	{
		fflush(output_);
		fclose(output_);
		output_ = NULL;
	}
}
