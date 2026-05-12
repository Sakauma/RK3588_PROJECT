# RK3588 FPGA 预处理后编码开发指南

## 目标链路

当前主链路为：

`FPGA(SRIO/视频输入 + OSD + 16bit->10bit) -> PCIe gray10le16 -> frame_assembler -> NV12 encoder input -> Rockchip MPP -> .h264/.h265`

输入回调不做编码，只复制 payload 并提交给后台管线。后台线程负责组帧、MPP 输入准备和编码，避免阻塞硬件接收线程。OSD 和原始位深转换已在 FPGA 完成。

## 输入边界

当前版本通过 PCIe SDK 接收图像数据；后续如果硬件团队改为 SRIO 接收，本工程不实现 SRIO 协议和驱动，只对接他们提供的 SDK。算法层固定接收“完整或分片的 gray10le16 payload”，入口保持为 `EncoderPipeline::SubmitPacket`，因此迁移时只替换输入适配代码，不改组帧、编码输入准备、MPP 编码和测试工具。

根据 FPGA RTL，当前视频出口为 `gray10le16`：每 64bit 包含 4 个 16bit little-endian 像素容器，低 10bit 有效。默认伪视频联调帧大小为 `2048 * 2048 * 2 = 8388608` 字节，对应 `VIDEO_DMA_CH=1`。

## 官方代码学习顺序

1. 编译并运行 Rockchip MPP 官方 `mpp/test/mpi_enc_test.c`，确认目标板编码器、驱动和库可用。
2. 阅读 MPP 编码配置：宽高、stride、码率、GOP、H.264/H.265 类型和 `MPP_FMT_YUV420SP`。
3. 阅读 librga `im2d` 示例，后续可把灰度映射、stride 拷贝或缩放迁移到 RGA；当前版本保留 CPU fallback。
4. 阅读当前 SDK 的 `ChannelRecvDataCallBack` 和 `RK3588API/include/glkimg.h`，确认 payload 是否包含 `CVG_GLKIMG_DMAHD`。SRIO SDK 到位后，对应阅读其接收回调并映射到同一 payload 接口。

## OSD 策略

- `OSD_MODE=burned-in`：默认模式。FPGA 已经把标示框叠进 gray10le16 图像，RK3588 软件不再绘制 OSD。
- `OSD_MODE=auto`：仅作为旧备用路径。后续若需要软件叠加，可调用 `EncoderPipeline::SetOsdObjects` 更新叠加对象。
- `OSD_TEST_ENABLE=1`：绘制两个测试框，用于离线验证 OSD 和编码输出。

`osd_overlay` 在 NV12 上做 CPU 绘制，保留给离线测试和回退模式；主链路不启用。

## 配置重点

- `FRAME_WIDTH`、`FRAME_HEIGHT` 必须与 FPGA 拼接输出一致；当前伪视频复制单路验证为 `2048x2048`。
- `INPUT_PIXEL_FORMAT=gray10le16` 表示 10bit 灰度像素放在 16bit little-endian 容器中，帧大小仍是 `width * height * 2`。
- `INPUT_PIXEL_FORMAT=nv12` 是预留模式；只有 FPGA 后续直接输出 packed NV12 时才启用。
- `PREFER_MAIN10=0` 是默认值；当前工程按 8-bit NV12 接入 MPP，真 10-bit 编码需另行验证。
- `RAW16_MAP_MODE=window` 是 gray10le16 默认映射，`RAW16_BLACK_LEVEL=0`、`RAW16_WHITE_LEVEL=1023` 覆盖 10bit 全范围。
- 若需要保留更多灰度细节，优先用 `RAW16_MAP_MODE=window` 并设置 `RAW16_BLACK_LEVEL`、`RAW16_WHITE_LEVEL`，把传感器有效灰度区间映射到 0-255。
- 场景亮度变化较大时可用 `RAW16_MAP_MODE=auto_window`，通过 `RAW16_AUTO_LOW_CLIP_PERMILLE` 和 `RAW16_AUTO_HIGH_CLIP_PERMILLE` 裁掉少量离群点后自动拉伸。
- 当前工程在未确认 Main10 输入格式前按 8-bit NV12 接入 MPP；若后续必须输出真 10-bit 码流，需要先在板端用供应商 MPP/驱动确认 Main10 编码支持。
- `INPUT_HAS_IMG_DMA_HEADER=1` 时，管线会跳过 `CVG_GLKIMG_DMAHD` 图像头。
- `ENCODER_QUEUE_DEPTH` 控制实时性；队列满时丢弃最旧帧。

## 验证路径

1. 先使用 MPP 官方示例确认硬件编码可用。
2. 运行 `offline_encode_test 1920 1080 /tmp/offline_gray10_osd.h265 60 gray10le16`，验证 gray10le16 到 MPP 8-bit NV12 输入准备和编码链路。
3. 最后接真实硬件 SDK 输入，观察 `packets`、`frames_in`、`encoded`、`dropped`、`main10_fallbacks`、`errors` 统计。
