# FPGA 伪视频联动调试说明

本文说明当前默认链路：Windows 图片转 `gray10le16`，RK3588 通过 PCIe 下行 DMA 送入 FPGA，FPGA 按原工程 SRIO-like 分片链路完成拼接和 10bit 输出，再经 PCIe 上行 DMA 回到 RK3588 编码保存。

## 当前默认参数

- FPGA 输入模式：`VIDEO_SOURCE_MODE=2`，即 RK3588 DMA 下行伪视频注入。
- FPGA 下行注入通道：`VIDEO_LOAD_DMA_CH=0`，对应 RK 配置 `DOWN_LOOP_DMA_CHANNUM=0`。
- FPGA 上行视频通道：`VIDEO_DMA_CH=1`，对应 RK 配置 `UP_LOOP_DMA_CHANNUM=1`。
- 帧格式：`2048x2048 gray10le16`，单帧大小 `2048 * 2048 * 2 = 8388608` 字节。
- 发送分片：64B 帧头 + 4096B payload 分片。

## 转换本地图像

在 Windows 主机执行：

```powershell
cd "D:\Staff\test\RK3588 SDK"
powershell -ExecutionPolicy Bypass -File .\tools\convert_png_to_gray10le16.ps1 `
  -InputDir "D:\Ajax Mao\研二\近期工作\研二下\操他妈的技术方案\fisheye\origin\images" `
  -OutputDir "D:\Staff\test\pseudo_frames" `
  -Width 2048 -Height 2048 -Force
```

脚本会把 RGB/8bit 图片转为 FPGA 输入 raw16 容器：`gray8 -> gray10 = gray8 << 2 -> raw16 = gray10 << 6`，实际小端 16bit 样本为 `gray8 << 8`。

## 拷贝到 RK3588

```powershell
ssh root@192.168.1.100 "mkdir -p /root/RK3588_PROJECT/pseudo_frames"
scp "D:\Staff\test\pseudo_frames\*.gray10le16" root@192.168.1.100:/root/RK3588_PROJECT/pseudo_frames/
```

## RK3588 配置

确认 `rk3588.ini` 包含：

```ini
UP_LOOP_DMA_CHANNUM=1
DOWN_LOOP_DMA_CHANNUM=0
PSEUDO_SEND_ENABLE=1
PSEUDO_FRAME_DIR=/root/RK3588_PROJECT/pseudo_frames
PSEUDO_PACKET_BYTES=4096
FRAME_WIDTH=2048
FRAME_HEIGHT=2048
INPUT_PIXEL_FORMAT=gray10le16
VIDEO_OUTPUT_PATH=/tmp/rk3588_capture.h265
```

## 上板运行顺序

1. 先烧录 FPGA 新 bitstream，并重启 RK3588 让 PCIe 重新枚举。
2. 在 RK3588 上编译并加载驱动：

```bash
cd /root/RK3588_PROJECT
insmod ./cvgDrv.ko || true
cmake -S WinSim -B build-rk3588 \
  -DRK3588API_INCLUDE_DIR=$PWD/RK3588API \
  -DRK3588API_LIBRARY=$PWD/libRK3588.so
cmake --build build-rk3588 -j2
```

3. 启动主程序：

```bash
./build-rk3588/RK3588Test
```

看到 `伪视频已发送...`，并且 `packets`、`frames`、`encoded` 持续增长，即表示下行注入、FPGA 回传、RK 编码链路在跑。

## 结果检查

```bash
ls -lh /tmp/rk3588_capture.h265
```

拷回 Windows 播放：

```powershell
scp root@192.168.1.100:/tmp/rk3588_capture.h265 .
ffplay -f hevc rk3588_capture.h265
```

若有 PCIe 设备但没有数据，优先检查 `UP_LOOP_DMA_CHANNUM`/`DOWN_LOOP_DMA_CHANNUM` 是否与当前 FPGA 参数对应。
