# RK3588 RAW16 编码开发指南

## 目标链路

第一版链路为：

`hardware SDK payload -> frame_assembler -> RAW16 -> NV12 -> optional OSD -> Rockchip MPP -> .h264/.h265`

输入回调不做编码，只复制 payload 并提交给后台管线。后台线程负责组帧、预处理、OSD 和编码，避免阻塞硬件接收线程。

## 输入边界

当前版本通过 PCIe SDK 接收图像数据；后续如果硬件团队改为 SRIO 接收，本工程不实现 SRIO 协议和驱动，只对接他们提供的 SDK。算法层固定接收“完整或分片的 RAW16 payload”，入口保持为 `EncoderPipeline::SubmitPacket`，因此迁移时只替换输入适配代码，不改 RAW16 预处理、OSD、MPP 编码和测试工具。

## 官方代码学习顺序

1. 编译并运行 Rockchip MPP 官方 `mpp/test/mpi_enc_test.c`，确认目标板编码器、驱动和库可用。
2. 阅读 MPP 编码配置：宽高、stride、码率、GOP、H.264/H.265 类型和 `MPP_FMT_YUV420SP`。
3. 阅读 librga `im2d` 示例，后续可把 RAW16 预处理、缩放或拷贝迁移到 RGA；当前版本保留 CPU fallback。
4. 阅读当前 SDK 的 `ChannelRecvDataCallBack` 和 `RK3588API/include/glkimg.h`，确认 payload 是否包含 `CVG_GLKIMG_DMAHD`。SRIO SDK 到位后，对应阅读其接收回调并映射到同一 payload 接口。

## OSD 策略

- `OSD_MODE=burned-in`：上游已经把标示框叠进 RAW16，软件不再绘制，只保证 RAW16 降位后框线可见。
- `OSD_MODE=auto`：默认模式。当前没有外部 OSD 元数据时不绘制；后续收到框坐标时调用 `EncoderPipeline::SetOsdObjects` 更新叠加对象。
- `OSD_TEST_ENABLE=1`：绘制两个测试框，用于离线验证 OSD 和编码输出。

第一版 `osd_overlay` 在 NV12 上做 CPU 绘制，支持矩形框、ASCII 标签、颜色和线宽。中文、图标、多边形、半透明图层留作后续版本。

## 配置重点

- `FRAME_WIDTH`、`FRAME_HEIGHT` 必须与 RAW16 输入一致。
- `RAW16_MAP_MODE=shift` 保持原始高速路径，`RAW16_SHIFT=8` 表示取 RAW16 高 8 bit 作为亮度。
- 若需要保留更多灰度细节，优先用 `RAW16_MAP_MODE=window` 并设置 `RAW16_BLACK_LEVEL`、`RAW16_WHITE_LEVEL`，把传感器有效灰度区间映射到 0-255。
- 场景亮度变化较大时可用 `RAW16_MAP_MODE=auto_window`，通过 `RAW16_AUTO_LOW_CLIP_PERMILLE` 和 `RAW16_AUTO_HIGH_CLIP_PERMILLE` 裁掉少量离群点后自动拉伸。
- 当前官方 MPP 编码链路按 8-bit NV12 接入；官方源码中的 10-bit YUV 格式定义主要不能直接等同于可用的 H.265 Main10 编码输入。若后续必须输出 10-bit 码流，需要先在板端用供应商 MPP/驱动确认 Main10 编码支持。
- `INPUT_HAS_IMG_DMA_HEADER=1` 时，管线会跳过 `CVG_GLKIMG_DMAHD` 图像头。
- `ENCODER_QUEUE_DEPTH` 控制实时性；队列满时丢弃最旧帧。

## 验证路径

1. 先使用 MPP 官方示例确认硬件编码可用。
2. 运行 `offline_encode_test 1920 1080 /tmp/offline_raw16_osd.h265 60`，验证 RAW16、OSD 和 MPP 编码链路。
3. 最后接真实硬件 SDK 输入，观察 `packets`、`frames_in`、`encoded`、`dropped`、`errors` 统计。
