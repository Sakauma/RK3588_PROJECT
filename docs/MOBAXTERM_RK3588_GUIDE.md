# MobaXterm 连接 RK3588 调试指南

本文说明如何在 Win11 上使用 MobaXterm 连接 RK3588，并完成串口登录、固定 IP、SSH 登录、文件传输和 SDK 程序调试。当前板端系统为 Ubuntu 20.04，调试网口为 `eth0`，目标 IP 为 `192.168.1.100`。

## 1. 准备工作

需要准备：

- Win11 主机。
- MobaXterm。
- 3.3V USB-TTL 串口线。
- 网线，用于 Win11 和 RK3588 直连或接入同一交换机。
- RK3588 板端电源。

串口接线：

```text
USB-TTL GND -> RK3588 GND
USB-TTL TXD -> RK3588 RXD
USB-TTL RXD -> RK3588 TXD
```

不要接 VCC。若串口一直没有输出，可断电后交换 TXD/RXD 再试。

## 2. 确认 Win11 串口号

插入 USB-TTL 后，在 Win11 打开：

```text
设备管理器 -> 端口 (COM 和 LPT)
```

找到 USB 串口设备，例如：

```text
Prolific PL2303GT USB Serial COM Port (COM8)
USB-SERIAL CH340 (COM5)
Silicon Labs CP210x (COM6)
```

记住括号里的 `COMx`。当前调试机使用的是 `COM8`。

## 3. 用 MobaXterm 建立串口会话

打开 MobaXterm，按以下步骤新建串口会话：

```text
Session -> Serial
```

填写：

```text
Serial port: COM8
Speed: 115200
```

再确认串口参数：

```text
Data bits: 8
Stop bits: 1
Parity: None
Flow control: None
```

点击 `OK` 打开终端。给 RK3588 上电或按复位键，正常会看到启动日志。若已经启动完成，按几次 `Enter`，应看到登录提示或 shell 提示符。

常见登录：

```text
login: root
password: 空 / root / 123456
```

登录成功后应看到类似：

```text
root@RK3588-Tronlong:~#
```

## 4. 串口登录后做基础确认

在 MobaXterm 串口终端中执行：

```bash
cat /etc/os-release
uname -a
whoami
ip -br link
ip -br addr
```

确认项：

- 系统是 Ubuntu 20.04。
- 当前用户是 `root`。
- `eth0` 显示 `UP` 和 `LOWER_UP`，说明网线已连接。

如果 `eth0` 是 `DOWN` 或没有 `LOWER_UP`，先检查网线、交换机、Win11 网卡和板端网口。

## 5. 配置 RK3588 静态 IP

在串口终端执行以下命令，把 `eth0` 配置为 `192.168.1.100/24`：

```bash
mkdir -p /etc/netplan
cat >/etc/netplan/01-rk3588-eth0-static.yaml <<'EOF'
network:
  version: 2
  renderer: networkd
  ethernets:
    eth0:
      dhcp4: no
      addresses:
        - 192.168.1.100/24
      optional: true
EOF
chmod 600 /etc/netplan/01-rk3588-eth0-static.yaml
netplan generate
netplan apply
```

再增加开机兜底服务，防止系统其他网络服务在启动后清掉 IPv4 地址：

```bash
cat >/usr/local/sbin/rk3588-set-eth0-static.sh <<'EOF'
#!/bin/sh
set -eu
IFACE=eth0
ADDR=192.168.1.100/24
/sbin/ip link set "$IFACE" up
/sbin/ip addr flush dev "$IFACE" scope global || true
/sbin/ip addr add "$ADDR" dev "$IFACE"
EOF
chmod +x /usr/local/sbin/rk3588-set-eth0-static.sh

cat >/etc/systemd/system/rk3588-eth0-static.service <<'EOF'
[Unit]
Description=Set RK3588 eth0 static IP for SDK debug
After=network.target network-online.target systemd-networkd.service NetworkManager.service networking.service docker.service
Wants=network-online.target

[Service]
Type=oneshot
ExecStartPre=/bin/sleep 8
ExecStart=/usr/local/sbin/rk3588-set-eth0-static.sh
RemainAfterExit=yes

[Install]
WantedBy=multi-user.target
EOF

systemctl daemon-reload
systemctl enable rk3588-eth0-static.service
systemctl restart rk3588-eth0-static.service
```

验证：

```bash
ip -br addr show eth0
systemctl is-enabled rk3588-eth0-static.service
systemctl is-active rk3588-eth0-static.service
```

期望看到：

```text
eth0 UP 192.168.1.100/24
enabled
active
```

## 6. 配置 Win11 网卡

如果 Win11 和 RK3588 直连，Win11 网卡应设置在同一网段，例如：

```text
IP 地址: 192.168.1.56
子网掩码: 255.255.255.0
默认网关: 留空
DNS: 留空
```

Win11 PowerShell 验证：

```powershell
ping 192.168.1.100
```

能收到回复后，再测试 SSH：

```powershell
ssh root@192.168.1.100
```

如果 PowerShell 能 SSH，MobaXterm 也可以 SSH。

## 7. 用 MobaXterm 建立 SSH 会话

在 MobaXterm 中新建 SSH 会话：

```text
Session -> SSH
```

填写：

```text
Remote host: 192.168.1.100
Specify username: root
Port: 22
```

点击 `OK`。第一次连接会提示保存 host key，选择接受。登录成功后，MobaXterm 左侧通常会出现 SFTP 文件浏览器。

建议后续主要使用 SSH 会话调试，串口只用于网络坏掉或系统启动异常时救援。

## 8. 使用 MobaXterm 传文件

SSH 会话连接成功后，左侧 SFTP 面板可以直接拖拽文件。

常用路径：

```text
板端工程目录: /root/RK3588_PROJECT
临时目录: /tmp
输出码流: /tmp/*.h265 或 /tmp/*.h264
```

如果左侧没有 SFTP 面板，可检查：

```text
Settings -> Configuration -> SSH -> SFTP browser
```

也可以在 Win11 PowerShell 使用 `scp`：

```powershell
scp .\RK3588_PROJECT_HEAD.tar.gz root@192.168.1.100:/tmp/
scp root@192.168.1.100:/tmp/offline_test.h265 .
```

## 9. 在 SSH 终端配置开发环境

进入 RK3588 后检查工具：

```bash
gcc --version
g++ --version
make --version
cmake --version
pkg-config --cflags --libs rockchip_mpp
pkg-config --cflags --libs librga
```

如果没有 CMake，可上传 ARM64 CMake 包后安装：

```bash
tar -xzf /tmp/cmake-4.3.2-linux-aarch64.tar.gz -C /opt
ln -sfn /opt/cmake-4.3.2-linux-aarch64 /opt/cmake-4.3.2
ln -sfn /opt/cmake-4.3.2/bin/cmake /usr/local/bin/cmake
ln -sfn /opt/cmake-4.3.2/bin/ctest /usr/local/bin/ctest
ln -sfn /opt/cmake-4.3.2/bin/cpack /usr/local/bin/cpack
cmake --version
```

板端没有 RTC 时，断电后系统时间可能错误。编译前可临时校时：

```bash
timedatectl set-time '2026-05-11 16:50:00' || date -s '2026-05-11 16:50:00'
```

## 10. 部署工程

若板端能访问 GitHub：

```bash
cd /root
git clone https://github.com/Sakauma/RK3588_PROJECT.git
cd RK3588_PROJECT
```

若板端不能访问 GitHub，先在 Win11 本地仓库打包：

```powershell
git archive --format=tar.gz -o $env:TEMP\RK3588_PROJECT_HEAD.tar.gz HEAD
scp $env:TEMP\RK3588_PROJECT_HEAD.tar.gz root@192.168.1.100:/tmp/
```

然后在 MobaXterm SSH 终端解包：

```bash
rm -rf /root/RK3588_PROJECT
mkdir -p /root/RK3588_PROJECT
tar -xzf /tmp/RK3588_PROJECT_HEAD.tar.gz -C /root/RK3588_PROJECT
cd /root/RK3588_PROJECT
```

## 11. 配置动态库和驱动

修正 SDK 动态库软链接：

```bash
cd /root/RK3588_PROJECT
rm -f libRK3588.so libRK3588.so.1
ln -s libRK3588.so.1.0.0 libRK3588.so.1
ln -s libRK3588.so.1 libRK3588.so
echo /root/RK3588_PROJECT >/etc/ld.so.conf.d/rk3588-project.conf
ldconfig
```

加载 `cvgDrv.ko`：

```bash
insmod /root/RK3588_PROJECT/cvgDrv.ko
lsmod | grep cvg
```

配置开机自动加载：

```bash
cat >/etc/systemd/system/cvgdrv.service <<'EOF'
[Unit]
Description=Load cvgDrv PCIe SDK driver
After=local-fs.target

[Service]
Type=oneshot
ExecStart=/bin/sh -c 'lsmod | grep -q "^cvgDrv" || /sbin/insmod /root/RK3588_PROJECT/cvgDrv.ko'
ExecStop=/bin/sh -c 'lsmod | grep -q "^cvgDrv" && /sbin/rmmod cvgDrv || true'
RemainAfterExit=yes

[Install]
WantedBy=multi-user.target
EOF

systemctl daemon-reload
systemctl enable cvgdrv.service
systemctl restart cvgdrv.service
systemctl status cvgdrv.service --no-pager
```

## 12. 编译工程

在 MobaXterm SSH 终端执行：

```bash
cd /root/RK3588_PROJECT
cmake -S WinSim -B build-rk3588 \
  -DRK3588API_INCLUDE_DIR=/root/RK3588_PROJECT/RK3588API \
  -DRK3588API_LIBRARY=/root/RK3588_PROJECT/libRK3588.so
cmake --build build-rk3588 -j2
```

生成：

```text
/root/RK3588_PROJECT/build-rk3588/RK3588Test
/root/RK3588_PROJECT/build-rk3588/offline_encode_test
```

可创建快捷命令：

```bash
cat >/usr/local/bin/rk3588-test <<'EOF'
#!/bin/sh
cd /root/RK3588_PROJECT
exec /root/RK3588_PROJECT/build-rk3588/RK3588Test "$@"
EOF
chmod +x /usr/local/bin/rk3588-test
ln -sfn /root/RK3588_PROJECT/build-rk3588/offline_encode_test /usr/local/bin/offline_encode_test
```

## 13. 运行离线编码测试

离线测试不依赖 PCIe 图像输入，先用它确认 MPP 编码可用：

```bash
offline_encode_test 320 240 /tmp/offline_test.h265 5
ls -lh /tmp/offline_test.h265
```

输出文件大于 0 字节即可。需要在 Win11 播放时，用 MobaXterm 左侧 SFTP 面板下载，或执行：

```powershell
scp root@192.168.1.100:/tmp/offline_test.h265 .
ffplay -f hevc offline_test.h265
```

## 14. 运行主程序

编辑配置：

```bash
cd /root/RK3588_PROJECT
cp -n rk3588.ini.example rk3588.ini
vi rk3588.ini
```

重点字段：

```ini
UP_LOOP_DMA_CHANNUM=5
DOWN_LOOP_DMA_CHANNUM=4
FRAME_WIDTH=1920
FRAME_HEIGHT=1080
VIDEO_CODEC=h265
VIDEO_OUTPUT_PATH=/tmp/rk3588_capture.h265
INPUT_PIXEL_FORMAT=gray10le16
RAW16_MAP_MODE=window
RAW16_SHIFT=2
RAW16_WHITE_LEVEL=1023
OSD_MODE=burned-in
OSD_ENABLE=0
```

启动：

```bash
rk3588-test
```

若硬件链路正常，程序应能扫描到板卡、打开通道、注册回调并持续编码。若输出码流路径为 `/tmp/rk3588_capture.h265`，可检查：

```bash
ls -lh /tmp/rk3588_capture.h265
```

## 15. MobaXterm 使用建议

- 串口会话会独占 COM 口；如果其他工具要访问串口，先关闭 MobaXterm 的串口标签页。
- 网络可用后优先用 SSH，不要长期依赖串口操作。
- 调试长日志时，在终端标签页右键，使用日志保存功能保存输出。
- 左侧 SFTP 面板适合下载 `.h265`、日志文件和配置文件。
- 如果 SSH 突然断开，先回到串口查看 `ip -br addr` 和 `systemctl status ssh`。

## 16. 常见问题

### 串口黑屏

检查：

- COM 号是否正确。
- 波特率是否为 `115200`。
- Flow control 是否为 `None`。
- TXD/RXD 是否需要互换。
- 板子是否已经上电。

### SSH 连接不上

Win11 执行：

```powershell
ping 192.168.1.100
```

板端串口执行：

```bash
ip -br addr show eth0
systemctl status ssh --no-pager
ss -lntp | grep ':22'
```

### 主程序提示 `not find KU card`

这说明 RK3588 软件环境已启动，但 PCIe/KU 板卡没有被 SDK 扫描到。检查：

```bash
lsmod | grep cvg
dmesg | grep -i pcie | tail -n 80
lspci 2>/dev/null || true
```

若出现 `PCIe Link Fail`，优先检查 FPGA/KU 板供电、bitstream、PCIe 线缆和 RK3588 PCIe 口配置。

### `cvgDrv.ko` 加载失败

执行：

```bash
insmod /root/RK3588_PROJECT/cvgDrv.ko
dmesg | tail -n 50
uname -a
```

若出现 `invalid module format`，说明驱动和当前内核版本不匹配，需要换对应内核版本的 `cvgDrv.ko`。
