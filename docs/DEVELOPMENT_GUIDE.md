# RK3588 RAW16 编码开发指南

## 目标链路

第一版链路为：

`PCIe DMA payload -> frame_assembler -> RAW16 -> NV12 -> optional OSD -> Rockchip MPP -> .h264/.h265`

DMA 回调不做编码，只复制 payload 并提交给后台管线。后台线程负责组帧、预处理、OSD 和编码，避免阻塞上行 DMA。

## 官方代码学习顺序

1. 编译并运行 Rockchip MPP 官方 `mpp/test/mpi_enc_test.c`，确认目标板编码器、驱动和库可用。
2. 阅读 MPP 编码配置：宽高、stride、码率、GOP、H.264/H.265 类型和 `MPP_FMT_YUV420SP`。
3. 阅读 librga `im2d` 示例，后续可把 RAW16 预处理、缩放或拷贝迁移到 RGA；当前版本保留 CPU fallback。
4. 阅读本 SDK 的 `ChannelRecvDataCallBack` 和 `RK3588API/include/glkimg.h`，确认 payload 是否包含 `CVG_GLKIMG_DMAHD`。

## OSD 策略

- `OSD_MODE=burned-in`：上游已经把标示框叠进 RAW16，软件不再绘制，只保证 RAW16 降位后框线可见。
- `OSD_MODE=auto`：默认模式。当前没有外部 OSD 元数据时不绘制；后续收到框坐标时调用 `EncoderPipeline::SetOsdObjects` 更新叠加对象。
- `OSD_TEST_ENABLE=1`：绘制两个测试框，用于离线验证 OSD 和编码输出。

第一版 `osd_overlay` 在 NV12 上做 CPU 绘制，支持矩形框、ASCII 标签、颜色和线宽。中文、图标、多边形、半透明图层留作后续版本。

## 配置重点

- `FRAME_WIDTH`、`FRAME_HEIGHT` 必须与 RAW16 输入一致。
- `RAW16_SHIFT=8` 表示取 RAW16 高 8 bit 作为亮度。
- `INPUT_HAS_IMG_DMA_HEADER=1` 时，管线会跳过 `CVG_GLKIMG_DMAHD` 图像头。
- `ENCODER_QUEUE_DEPTH` 控制实时性；队列满时丢弃最旧帧。

## 验证路径

1. 先使用 MPP 官方示例确认硬件编码可用。
2. 运行 `offline_encode_test 1920 1080 /tmp/offline_raw16_osd.h265 60`，验证 RAW16、OSD 和 MPP 编码链路。
3. 最后接真实 DMA 输入，观察 `packets`、`frames_in`、`encoded`、`dropped`、`errors` 统计。
