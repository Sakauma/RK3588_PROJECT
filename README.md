# RK3588 V7 PCIe/SRIO Live Camera Test

本工程用于 RK3588 接收 V7 前端板发送的 SRIO SWRITE 图像数据，按 V7 地址映射拼接灰度帧，并通过 RK3588 本机 HTTP MJPEG 服务给笔记本浏览器实时预览。

当前现场链路为：

```text
V7 FPGA -> SRIO SWRITE 图像包 -> RK3588 PCIe SDK 回调
    -> bank0/bank1 乒乓帧缓冲 -> raw16 min-max 8bit -> libjpeg
    -> HTTP MJPEG http://192.168.1.100:7766
```

## 目录结构

- `WinSim/ConsoleApplication1/`：主程序入口、SRIO SWRITE 测试发送、V7 接收回调、拼帧、MJPEG 服务。
- `RK3588API/`：当前板端 SDK 头文件。
- `libRK3588.so*`：当前 SDK 运行库，部署时放在 `RK3588Test` 同目录。
- `cvgDrv.ko`：当前 RK3588 PCIe 驱动模块。
- `rk3588.ini`：现场运行配置，默认 DMA 通道为 `0/0`，HTTP MJPEG 端口为 `7766`。
- `docs/V7_LIVE_CAMERA_RUNBOOK.md`：从 SSH 连接、加载驱动、编译到运行验证的步骤。

## 关键行为

- 只把 `ftype=0x6` 的 SWRITE 包作为图像数据。
- `ftype=0xA` doorbell 只作为帧结束/状态通知。
- 第 0 行为参数行，不进入图像；第 1-2048 行映射为 `2048x2048` 灰度图像。
- `ftype=0x5` 等非 SWRITE 包只做限频统计和可选 raw 留样，不参与拼图。
- BAR/DDR 直接读取路径默认关闭，主流程只用 SWRITE 包拼帧，避免 doorbell 后访问 BAR 导致崩溃。
- 输出层使用自维护 HTTP MJPEG server，不再使用 `opencv-mobile httpjpg`。
- 如果没有新帧，MJPEG server 会按 `STREAM_REPEAT_FPS` 重复发送最后一帧，避免浏览器后打开时黑屏。

## 编译

推荐在 RK3588 板端编译：

```bash
cd /root/rk3588_v7
insmod ./cvgDrv.ko 2>/dev/null || true

cmake -S WinSim -B WinSim/build_deploy \
  -DRK3588API_INCLUDE_DIR=/root/rk3588_v7/RK3588API \
  -DRK3588_LIB_PATH=/root/rk3588_v7/libRK3588.so
cmake --build WinSim/build_deploy -j$(nproc 2>/dev/null || echo 2)
cp WinSim/build_deploy/RK3588Test /root/rk3588_v7/RK3588Test
chmod +x /root/rk3588_v7/RK3588Test
```

依赖：

- `cmake`
- `make`
- `g++`
- `libjpeg-dev`
- 当前目录内的 `libRK3588.so*`
- 当前目录内的 `cvgDrv.ko`

## 运行

```bash
cd /root/rk3588_v7
insmod ./cvgDrv.ko 2>/dev/null || true
./RK3588Test
```

程序启动后会监听：

```text
http://192.168.1.100:7766
```

浏览器打开该地址即可查看 live camera 画面。

## 当前现场判断

历史调试中，RK3588 收到的 doorbell 稳定停在 28 次左右，且接收入口计数 `g_uSrioRecvCount` 不再增长。因为该计数在 SDK 回调入口立即递增，所以后续如果仍有任何包进入 RK3588，无论解析是否成功，计数都应继续变化。

因此 28 帧停止问题更像 V7 前端发送侧停止发包，而不是 RK3588 输出线程或 HTTP MJPEG 线程限制。HTTP 层已经改为重复发送最后一帧，用来单独排除浏览器黑屏问题。
