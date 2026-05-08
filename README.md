# RK3588_PROJECT

RK3588 RAW16 image stream encoder project. The current application receives image payloads through the bundled PCIe SDK, assembles RAW16 grayscale frames, converts them to NV12, optionally draws OSD boxes, and sends frames to Rockchip MPP for H.264/H.265 hardware encoding.

The transport layer is intentionally treated as a hardware-SDK boundary. Today the input adapter is PCIe/DMA; if the hardware team later provides an SRIO SDK, the algorithm layer should keep using the same payload handoff into `EncoderPipeline::SubmitPacket`.

## Layout

- `RK3588API/`: vendor SDK headers for the current hardware input path.
- `WinSim/ConsoleApplication1/`: current PCIe/DMA input adapter and application entrypoint.
- `WinSim/encoder/`: RAW16 frame assembly, preprocessing, OSD, MPP encoder, and pipeline modules.
- `rk3588.ini.example`: runtime configuration template.

## Dependencies

On the RK3588 target, install or build:

- Rockchip MPP: https://github.com/rockchip-linux/mpp
- Rockchip librga: https://github.com/airockchip/librga
- The vendor runtime library `libRK3588.so` and driver `cvgDrv.ko` from the SDK package.

`libRK3588.so*` and `cvgDrv.ko` are intentionally not tracked in Git. Place them beside the executable or in the target library/driver paths documented by the board image.

## Build

```bash
cmake -S WinSim -B WinSim/build_arm64 \
  -DCMAKE_TOOLCHAIN_FILE=WinSim/toolchain-arm64.cmake \
  -DRK3588API_INCLUDE_DIR=$PWD/RK3588API \
  -DRK3588API_LIBRARY=$PWD/libRK3588.so
cmake --build WinSim/build_arm64
```

On a native RK3588 system with MPP installed:

```bash
cmake -S WinSim -B build-rk3588 \
  -DRK3588API_INCLUDE_DIR=$PWD/RK3588API \
  -DRK3588API_LIBRARY=$PWD/libRK3588.so
cmake --build build-rk3588
```

## Runtime

Copy `rk3588.ini.example` to `rk3588.ini`, then set `FRAME_WIDTH`, `FRAME_HEIGHT`, DMA channel numbers, codec, bitrate, and output path. First validation should use `VIDEO_CODEC=h265`, `OSD_TEST_ENABLE=1`, and a short capture, then play the generated stream:

```bash
ffplay -f hevc /tmp/rk3588_capture.h265
```

For H.264 use:

```bash
ffplay -f h264 /tmp/rk3588_capture.h264
```

## Offline Encoder Test

On the RK3588 target, run the synthetic test before connecting the hardware SDK input:

```bash
./build-rk3588/offline_encode_test 1920 1080 /tmp/offline_raw16_osd.h265 60
ffplay -f hevc /tmp/offline_raw16_osd.h265
```

This generates RAW16 grayscale frames, draws two test OSD boxes, and encodes through MPP.
