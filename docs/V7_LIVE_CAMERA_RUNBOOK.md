# V7 Live Camera 运行手册

本文按现场联调流程整理，便于从 Windows 电脑通过 SSH 操作 RK3588。

## 1. SSH 连接

RK3588 默认地址：

```bash
ssh root@192.168.1.100
```

进入工程目录：

```bash
cd /root/rk3588_v7
```

## 2. 检查驱动和 PCIe

```bash
insmod ./cvgDrv.ko 2>/dev/null || true
lsmod | grep cvgDrv
ls -l /dev/cvgdev00
lspci
dmesg | tail -80
```

期望结果：

- `lsmod` 能看到 `cvgDrv`。
- `/dev/cvgdev00` 存在。
- `lspci` 能看到 RK3588 PCI bridge 和 V7/采集卡设备。
- `dmesg` 中 `cvg_dev_probe: success`。

## 3. 编译

```bash
cmake -S WinSim -B WinSim/build_deploy \
  -DRK3588API_INCLUDE_DIR=/root/rk3588_v7/RK3588API \
  -DRK3588_LIB_PATH=/root/rk3588_v7/libRK3588.so

cmake --build WinSim/build_deploy -j$(nproc 2>/dev/null || echo 2)
cp WinSim/build_deploy/RK3588Test /root/rk3588_v7/RK3588Test
chmod +x /root/rk3588_v7/RK3588Test
```

## 4. 启动程序

```bash
cd /root/rk3588_v7
./RK3588Test
```

启动日志中需要看到：

```text
从配置文件读取通道号: 上行=0, 下行=0
V7 MJPEG server ready: http://0.0.0.0:7766
V7 live stream: enable
V7 MJPEG stream: jpeg_quality=85, repeat_fps=10
Current mode: listen-only
```

程序默认只接收，只有输入 `s` 才会发送一次测试 SWRITE。普通回车只刷新统计，不会制造测试包干扰现场数据。

## 5. 浏览器查看

笔记本浏览器打开：

```text
http://192.168.1.100:7766
```

如果已经收到过至少一帧，HTTP MJPEG server 会按 `STREAM_REPEAT_FPS` 重复发送最后一帧；即使 V7 后续暂时停止发包，浏览器也不应黑屏。

## 6. 刷新统计

如果程序通过管道启动，可向 stdin 写入回车刷新统计：

```bash
printf '\n' >/tmp/rk3588test.stdin
tail -120 /tmp/rk3588test_live.log
ss -tanp | grep ':7766'
dmesg | tail -80
```

重点看：

```text
V7接收包: 已接收=...
V7 doorbell: recv=...
V7 frame output: queued=...
V7 stream: encoded=...
V7 MJPEG: clients=..., repeat=...
```

判断方法：

- `V7接收包` 继续增长：RK3588 回调仍在收到上游包。
- `doorbell recv` 不增长但 `non-SWRITE recv` 增长：需要分析非 SWRITE 包格式，例如 `ftype=0x5`。
- 三者都不增长：上游 V7 很可能停止发送。
- `stream encoded` 达到 queued 且 `q=0, mem_q=0, spool_q=0`：输出队列已经清空，HTTP 输出层没有堵住接收。

## 7. 当前关键配置

```ini
UP_LOOP_DMA_CHANNUM=0
DOWN_LOOP_DMA_CHANNUM=0
TRANSFER_ENABLE=0
STREAM_ENABLE=1
STREAM_PORT=7766
STREAM_JPEG_QUALITY=85
STREAM_REPEAT_FPS=10
STREAM_MAX_CLIENTS=4
STREAM_SEND_TIMEOUT_MS=800
STREAM_MAX_MEM_FRAMES=8
STREAM_SPOOL_PATH=/tmp/pcie_yuv/spool/
STREAM_MAX_SPOOL_GB=8
STREAM_PARTIAL_ENABLE=1
STREAM_MIN_IMAGE_PACKETS=28000
RAW_CAPTURE_ENABLE=0
SWRITE_TRACE_ENABLE=0
```

## 8. 退出程序

交互式运行时输入：

```text
q
```

如果使用后台管道启动：

```bash
printf 'q\n' >/tmp/rk3588test.stdin
```

不要直接重启程序来验证同一现场问题；如果必须重启程序，建议同时重启前端板，避免旧硬件状态影响判断。
