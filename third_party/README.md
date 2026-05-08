# Third-party Official Sources

This project intentionally does not vendor large third-party source trees.

Use official Rockchip repositories on the RK3588 build host:

```bash
git clone https://github.com/rockchip-linux/mpp.git
git clone https://github.com/airockchip/librga.git
```

Build and install them according to each repository's upstream instructions, then configure this project with:

```bash
cmake -S WinSim -B build-rk3588 \
  -DMPP_INCLUDE_DIR=/path/to/mpp/inc \
  -DMPP_LIBRARY=/path/to/librockchip_mpp.so \
  -DRGA_INCLUDE_DIR=/path/to/librga/include \
  -DRGA_LIBRARY=/path/to/librga.so
```
