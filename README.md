# RK3588_PROJECT

FPGA + RK3588 上板联调步骤见 `docs/FPGA_RK3588_BOARD_BRINGUP.md`，包含 SSH、驱动加载、编译、离线编码测试和 PCIe 实测流程。

本工程用于 RK3588 接收 FPGA 预处理后的视频流并编码保存。当前硬件链路为：

`FPGA(SRIO/视频输入 + OSD + 16bit->10bit) -> PCIe gray10le16 -> RK3588 NV12准备 -> Rockchip MPP -> .h264/.h265`

FPGA 当前输出 `gray10le16`：每个像素占 16bit little-endian 容器，低 10bit 有效，OSD 已经作为像素内容烧录进图像。RK3588 侧不再负责 OSD 和原始位深转换，只做 PCIe 收帧、灰度帧到 MPP 8-bit NV12 输入准备、H.264/H.265 硬件编码和裸码流保存。

输入传输层被视为硬件 SDK 边界：当前适配 PCIe/DMA；如果后续硬件团队提供 SRIO SDK，算法层仍复用 `EncoderPipeline::SubmitPacket` 作为 payload 入口，只替换输入适配代码。

## 目录结构

- `RK3588API/`：当前硬件输入链路的厂商 SDK 头文件。
- `WinSim/ConsoleApplication1/`：PCIe/DMA 输入适配和程序入口。
- `WinSim/encoder/`：帧组装、gray10le16/NV12 输入准备、MPP 编码和管线模块；旧 RAW16/软件 OSD 路径作为备用保留。
- `docs/DEVELOPMENT_GUIDE.md`：FPGA 预处理后编码链路、配置和后续 SRIO SDK 对接开发指南。
- `docs/RK3588_DEBUG_GUIDE.md`：Win11 通过串口/SSH 调试 RK3588 的完整上板步骤。
- `docs/MOBAXTERM_RK3588_GUIDE.md`：使用 MobaXterm 连接、传输文件和调试 RK3588 的分步指南。
- `rk3588.ini.example`：运行时配置模板。

## 依赖

在 RK3588 目标板上需要安装或编译：

- Rockchip MPP：https://github.com/rockchip-linux/mpp
- Rockchip librga：https://github.com/airockchip/librga
- 本仓库根目录中的厂商运行库 `libRK3588.so*`
- 本仓库根目录中的板端驱动 `cvgDrv.ko`

`libRK3588.so*` 和 `cvgDrv.ko` 已随仓库提交，clone 后可直接用于当前 SDK 对应的板端环境。若后续更换板端镜像、内核或厂商 SDK 版本，需要同步替换这些二进制文件。

## 编译

推荐在 RK3588 Linux 板端编译：

```bash
cmake -S WinSim -B build-rk3588 \
  -DRK3588API_INCLUDE_DIR=$PWD/RK3588API \
  -DRK3588API_LIBRARY=$PWD/libRK3588.so
cmake --build build-rk3588
```

使用 ARM64 交叉编译工具链时：

```bash
cmake -S WinSim -B WinSim/build_arm64 \
  -DCMAKE_TOOLCHAIN_FILE=WinSim/toolchain-arm64.cmake \
  -DRK3588API_INCLUDE_DIR=$PWD/RK3588API \
  -DRK3588API_LIBRARY=$PWD/libRK3588.so
cmake --build WinSim/build_arm64
```

普通 Windows 本机目前不能完整编译主程序，因为原 SDK 代码依赖 Linux 驱动环境和 `gettimeofday`、`mkdir` 等接口。普通 Linux PC 若没有 RK3588 MPP 和厂商库，也只能做有限的源码检查，不能验证真实硬件编码。

## 运行配置

复制配置模板：

```bash
cp rk3588.ini.example rk3588.ini
```

然后设置 `FRAME_WIDTH`、`FRAME_HEIGHT`、DMA 通道、`VIDEO_CODEC`、`VIDEO_BITRATE`、灰度映射和输出路径。默认值匹配当前 FPGA 输出：

```ini
INPUT_PIXEL_FORMAT=gray10le16
RAW16_MAP_MODE=window
RAW16_SHIFT=2
RAW16_BLACK_LEVEL=0
RAW16_WHITE_LEVEL=1023
OSD_MODE=burned-in
OSD_ENABLE=0
```

如果场景亮度变化较大，可使用 `RAW16_MAP_MODE=auto_window`。首次验证建议设置 `VIDEO_CODEC=h265`，短时间采集后播放裸流：

```bash
ffplay -f hevc /tmp/rk3588_capture.h265
```

H.264 输出使用：

```bash
ffplay -f h264 /tmp/rk3588_capture.h264
```

## 离线编码测试

接入真实硬件输入前，可先在 RK3588 目标板运行模拟 gray10le16 测试：

```bash
./build-rk3588/offline_encode_test 1920 1080 /tmp/offline_gray10_osd.h265 60 gray10le16
ffplay -f hevc /tmp/offline_gray10_osd.h265
```

该测试会生成 gray10le16 灰度渐变帧，模拟已烧录 OSD 的可见性检查，并通过 MPP 编码输出 H.265 裸码流。若后续 FPGA 改为直接输出 NV12，可用 `offline_encode_test ... nv12` 验证预留路径。

## 当前限制

- 当前输出为 `.h264` / `.h265` 裸码流，不做 MP4 封装和 RTSP 推流。
- 当前 FPGA 输出不是 NV12/YUV，而是 gray10le16；RK3588 仍需准备 8-bit NV12 输入给 MPP。
- H.265 Main10 / 10-bit 编码尚未确认，需要在目标板供应商 MPP 和驱动环境中单独验证。
- SRIO 接收不在本工程实现范围内；后续只对接硬件团队提供的 SDK。
