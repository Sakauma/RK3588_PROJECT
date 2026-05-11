# RK3588 上板调试步骤

本文记录在 Win11 主机上调试 RK3588 Ubuntu 20.04 板端环境的完整流程。当前约定 RK3588 使用 `eth0`，静态 IP 为 `192.168.1.100/24`，Win11 主机网卡需在同一网段。

## 1. 串口连接

使用 3.3V USB-TTL 串口线连接 RK3588 调试串口：

```text
USB-TTL GND -> RK3588 GND
USB-TTL TXD -> RK3588 RXD
USB-TTL RXD -> RK3588 TXD
```

不要接 VCC。Win11 设备管理器中确认串口号，例如 `COM8`。MobaXterm 或 PuTTY 串口参数：

```text
Serial port: COM8
Baudrate: 115200
Data bits: 8
Stop bits: 1
Parity: None
Flow control: None
```

登录后确认系统：

```bash
cat /etc/os-release
uname -a
whoami
```

## 2. 配置板端静态 IP

先确认实际插线网口：

```bash
ip -br link
ip -br addr
```

当前板卡使用 `eth0`。写入 netplan：

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

为防止系统其他网络服务在开机后清掉 IPv4 地址，增加 systemd 兜底服务：

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

Win11 侧验证：

```powershell
ping 192.168.1.100
ssh root@192.168.1.100
```

## 3. 安装开发工具

板端应具备：

```bash
gcc --version
g++ --version
make --version
git --version
pkg-config --version
```

当前板端没有 apt 外网源时，可从 Win11 下载 ARM64 CMake 包并拷贝到板端：

```powershell
scp cmake-4.3.2-linux-aarch64.tar.gz root@192.168.1.100:/tmp/
```

板端安装：

```bash
tar -xzf /tmp/cmake-4.3.2-linux-aarch64.tar.gz -C /opt
ln -sfn /opt/cmake-4.3.2-linux-aarch64 /opt/cmake-4.3.2
ln -sfn /opt/cmake-4.3.2/bin/cmake /usr/local/bin/cmake
ln -sfn /opt/cmake-4.3.2/bin/ctest /usr/local/bin/ctest
ln -sfn /opt/cmake-4.3.2/bin/cpack /usr/local/bin/cpack
cmake --version
```

板端没有 RTC 时，断电后系统时间会回到旧日期。编译前建议校时：

```bash
timedatectl set-time '2026-05-11 16:50:00' || date -s '2026-05-11 16:50:00'
```

## 4. 部署工程

从 GitHub 拉取，或从 Win11 打包上传：

```bash
cd /root
git clone https://github.com/Sakauma/RK3588_PROJECT.git
cd RK3588_PROJECT
```

如果板端不能访问 GitHub，可在 Win11 仓库中执行：

```powershell
git archive --format=tar.gz -o $env:TEMP\RK3588_PROJECT_HEAD.tar.gz HEAD
scp $env:TEMP\RK3588_PROJECT_HEAD.tar.gz root@192.168.1.100:/tmp/
ssh root@192.168.1.100 "rm -rf /root/RK3588_PROJECT && mkdir -p /root/RK3588_PROJECT && tar -xzf /tmp/RK3588_PROJECT_HEAD.tar.gz -C /root/RK3588_PROJECT"
```

## 5. 固定运行库路径

工程根目录包含 `libRK3588.so*`。建议在板端修正 so 链接并写入动态库路径：

```bash
cd /root/RK3588_PROJECT
rm -f libRK3588.so libRK3588.so.1
ln -s libRK3588.so.1.0.0 libRK3588.so.1
ln -s libRK3588.so.1 libRK3588.so
echo /root/RK3588_PROJECT >/etc/ld.so.conf.d/rk3588-project.conf
ldconfig
```

## 6. 配置驱动开机加载

手动加载：

```bash
insmod /root/RK3588_PROJECT/cvgDrv.ko
lsmod | grep cvg
```

配置 systemd 开机加载：

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

## 7. 编译工程

在 RK3588 板端编译：

```bash
cd /root/RK3588_PROJECT
cmake -S WinSim -B build-rk3588 \
  -DRK3588API_INCLUDE_DIR=/root/RK3588_PROJECT/RK3588API \
  -DRK3588API_LIBRARY=/root/RK3588_PROJECT/libRK3588.so
cmake --build build-rk3588 -j2
```

生成文件：

```text
/root/RK3588_PROJECT/build-rk3588/RK3588Test
/root/RK3588_PROJECT/build-rk3588/offline_encode_test
```

建议创建快捷命令：

```bash
cat >/usr/local/bin/rk3588-test <<'EOF'
#!/bin/sh
cd /root/RK3588_PROJECT
exec /root/RK3588_PROJECT/build-rk3588/RK3588Test "$@"
EOF
chmod +x /usr/local/bin/rk3588-test
ln -sfn /root/RK3588_PROJECT/build-rk3588/offline_encode_test /usr/local/bin/offline_encode_test
```

## 8. 验证 MPP 编码链路

先跑离线编码，不依赖 PCIe 图像输入：

```bash
offline_encode_test 320 240 /tmp/offline_test.h265 5
ls -lh /tmp/offline_test.h265
```

如果文件大于 0 字节，说明 MPP、编码封装和文件输出路径可用。需要播放时，把文件拷回 Win11：

```powershell
scp root@192.168.1.100:/tmp/offline_test.h265 .
ffplay -f hevc offline_test.h265
```

## 9. 配置并运行主程序

编辑运行配置：

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
RAW16_MAP_MODE=shift
OSD_ENABLE=1
OSD_TEST_ENABLE=0
```

启动：

```bash
rk3588-test
```

主程序启动后应依次完成：

```text
cvg_scan_devcnt()
cvg_card_open()
cvg_open_chan()
cvg_pro_register_cb()
cvg_pro_start()
ChannelRecvDataCallBack()
```

输出码流检查：

```bash
ls -lh /tmp/rk3588_capture.h265
```

## 10. 重启后验证

每次改完服务后都要重启验证：

```bash
sync
reboot
```

重新 SSH 登录后执行：

```bash
ip -br addr show eth0
systemctl is-active rk3588-eth0-static.service
systemctl is-active cvgdrv.service
lsmod | grep cvg
cmake --version
offline_encode_test 320 240 /tmp/post_boot_offline.h265 2
ls -lh /tmp/post_boot_offline.h265
```

## 11. 常见问题

### SSH 不通

在 Win11 确认网卡同网段，例如：

```text
IP: 192.168.1.56
Mask: 255.255.255.0
Gateway: 留空
```

板端串口执行：

```bash
ip -br link
ip -br addr
systemctl status ssh --no-pager
ss -lntp | grep ':22'
```

### `not find KU card`

说明程序、库和驱动已运行，但 SDK 没扫描到 PCIe/KU 板卡。按顺序检查：

```bash
lsmod | grep cvg
dmesg | grep -i pcie | tail -n 80
lspci 2>/dev/null || true
```

重点排查 FPGA/KU 板是否上电、bitstream 是否正确、PCIe 线缆/插接是否正确、RK3588 PCIe 口是否启用。若 `dmesg` 出现 `PCIe Link Fail`，优先处理硬件链路。

### `cvgDrv.ko` 加载失败

查看错误：

```bash
insmod /root/RK3588_PROJECT/cvgDrv.ko
dmesg | tail -n 50
uname -a
```

若出现 `invalid module format`，说明驱动和当前内核版本不匹配，需要对应内核版本的 `cvgDrv.ko`。

### 离线编码段错误或输出 0 字节

先确认 MPP 版本和库：

```bash
ldconfig -p | grep rockchip_mpp
find /usr/include/rockchip -maxdepth 1 -type f | sort
gdb -batch -ex run -ex bt --args offline_encode_test 320 240 /tmp/test.h265 1
```

本工程已兼容当前板端 MPP 头文件：使用 `rk_venc_cfg.h`，并为 `MPP_ENC_GET_HDR_SYNC` 预分配 header buffer。

### 编译时间戳异常

当前板端没有可用 RTC，断电后系统时间可能回到 2024。编译前校时：

```bash
timedatectl set-time '2026-05-11 16:50:00' || date -s '2026-05-11 16:50:00'
```
