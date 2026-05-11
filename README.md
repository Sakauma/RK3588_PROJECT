# RK3588_PROJECT

本工程用于 RK3588 RAW16 图像流编码。当前程序通过随 SDK 提供的 PCIe 输入链路接收图像 payload，组装 RAW16 灰度帧，转换为 NV12，可选叠加 OSD 标示框，然后调用 Rockchip MPP 硬件编码为 H.264/H.265 裸码流。

输入传输层被视为硬件 SDK 边界：当前适配 PCIe/DMA；如果后续硬件团队提供 SRIO SDK，算法层仍复用 `EncoderPipeline::SubmitPacket` 作为 payload 入口，只替换输入适配代码。

## 目录结构

- `RK3588API/`：当前硬件输入链路的厂商 SDK 头文件。
- `WinSim/ConsoleApplication1/`：PCIe/DMA 输入适配和程序入口。
- `WinSim/encoder/`：RAW16 组帧、预处理、OSD、MPP 编码和管线模块。
- `docs/DEVELOPMENT_GUIDE.md`：RAW16 编码、OSD 和后续 SRIO 对接开发指南。
- `docs/RK3588_DEBUG_GUIDE.md`：Win11 通过串口/SSH 调试 RK3588 的完整上板步骤。
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

然后设置 `FRAME_WIDTH`、`FRAME_HEIGHT`、DMA 通道、`VIDEO_CODEC`、`VIDEO_BITRATE`、RAW16 灰度映射和输出路径。

灰度细节优先时建议使用：

```ini
RAW16_MAP_MODE=window
RAW16_BLACK_LEVEL=1024
RAW16_WHITE_LEVEL=12000
```

如果场景亮度变化较大，可使用 `RAW16_MAP_MODE=auto_window`。首次验证建议设置 `VIDEO_CODEC=h265`、`OSD_TEST_ENABLE=1`，短时间采集后播放裸流：

```bash
ffplay -f hevc /tmp/rk3588_capture.h265
```

H.264 输出使用：

```bash
ffplay -f h264 /tmp/rk3588_capture.h264
```

## 离线编码测试

接入真实硬件输入前，可先在 RK3588 目标板运行模拟 RAW16 测试：

```bash
./build-rk3588/offline_encode_test 1920 1080 /tmp/offline_raw16_osd.h265 60
ffplay -f hevc /tmp/offline_raw16_osd.h265
```

该测试会生成 RAW16 灰度渐变帧，绘制两个测试 OSD 框，并通过 MPP 编码输出 H.265 裸码流。

## 当前限制

- 当前输出为 `.h264` / `.h265` 裸码流，不做 MP4 封装和 RTSP 推流。
- 当前落地链路是 RAW16 灰度优化后转 8-bit NV12，再交给 MPP 编码。
- H.265 Main10 / 10-bit 编码尚未确认，需要在目标板供应商 MPP 和驱动环境中单独验证。
- SRIO 接收不在本工程实现范围内；后续只对接硬件团队提供的 SDK。
