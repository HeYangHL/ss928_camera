# AGENTS.md - SD3403 Object Detection Project

## Project Overview

C++ embedded project for SD3403 (SS928V100) ARM hardware: MIPI camera capture,
AI object detection (YOLOv5 on NPU via svp_acl), multi-target tracking
(Kalman/Hungarian), H.264 encoding, and RTSP streaming (live555). Built with
CMake and cross-compiled for `aarch64` using the mix210 toolchain.

## Environment

- Cross-compiler: `aarch64-mix210-linux-g++` (gcc 7.3.0) at
  `/opt/linux/x86-arm/aarch64-mix210-linux/bin/`
- SS928 SDK: `/home/ebaina/ss928v100_sdk/SS928V100_SDK_V2.0.2.1` (hardcoded as
  `SS928_SDK` in `CMakeLists.txt`)
- Vendored prebuilt `.so` libs: `3rdLibrary/opencv/`, `3rdLibrary/rtsp/`,
  `3rdLibrary/openssl/`; build output goes to `build/`

## Build Commands

All builds must run from `build/`; the toolchain and SDK paths are hardcoded in
the root `CMakeLists.txt`. Examples are added via `add_subdirectory`. There is
**no lint configuration** (no clang-format, no `.clang-tidy`); style is manual.

```bash
cd build && cmake .. && make          # configure + full build
cd build && make                      # incremental build
cd build && make sd3403_obj           # main application
cd build && make obj_detec            # model/detection test example
cd build && make sd3403_decode        # decode example
cd build && ./sd3403_obj              # run (needs camera + RTSP HW)
```

## Testing (single "test")

There is **no unit-test framework** (no CTest, no `enable_testing`). The closest
thing to a single test is the model example `obj_detec`, which runs one image
through the YOLOv5 NPU pipeline:

```bash
cd build && cmake .. && make obj_detec
cd build && ./obj_detec
```

It reads `./711_VL_1012.jpeg`, `./model/yolov5s_yuv_original.om`, and
`./model/yolov5_rpn.txt` from the working directory and writes `output_N.jpg`.
It exercises NPU (svp_acl) code paths, so it must run on target hardware — it
will not work in an emulator or on x86.

## Known Build Issues

1. `CMakeLists.txt:61` lists `rtsp/live555/GetGrame.cpp` (typo — the file is
   `GetFrame.cpp`). That source is not compiled, so linking `sd3403_obj` fails
   with undefined references to `GetFrame::Set_Video_Frame`,
   `GetFrame::Init_Socket`, and `GetFrame::Get_Video_Frame`. Fix the typo and
   reconfigure before linking `sd3403_obj`.
2. `examples/decode/CMakeLists.txt` uses `SS928_SDK_PATH` (undefined; the root
   defines `SS928_SDK`), so the decode example's SDK include dir resolves empty.

## Code Style

### Formatting
- 4 spaces indentation, no tabs; max line length 120 characters
- Braces on the same line for control statements

### Header Guards
```cpp
#ifndef _SD3403_FILE_NAME_HPP_
#define _SD3403_FILE_NAME_HPP_
#endif
```
(Exceptions: `def.h` uses `_DEF_H_`; `rtsp.hpp` uses `_RTSP_HPP_`.)

### Include Order (preferred)
1. Standard C/C++ (`<stdio.h>`, `<iostream>`, `<thread>`, `<vector>`)
2. System/POSIX (`<sys/types.h>`, `<unistd.h>`, `<semaphore.h>`)
3. SDK (`hi_*.h`, `ot_*.h`) and ACL (`svp_acl*.h`)
4. Third-party (`opencv2/*.hpp`)
5. Local (`"def.h"`, `"sd3403_*.hpp"`, `"Fifo_Buffer.hpp"`)

Note: existing files are inconsistent (e.g. `def.h` puts OpenCV before SDK;
`svp_npu.hpp` uses `using namespace std;`). Match the file you edit; prefer the
order above in new files.

### Naming Conventions

| Element | Convention | Example |
|---------|------------|---------|
| Classes | CamelCase | `SD3403_Manage`, `Obj_Detec`, `SVP_NNN` |
| Functions | CamelCase | `getSensorFifo()`, `RunFifoThread()` |
| Member vars | No prefix, `_` or `g_` for globals | `g_sig_flag`, `venc_chn`, `_enc_run` |
| Constants | UPPER_CASE | `WIDTH_3840`, `HEIGHT_2160`, `constexpr SCALE_SIZE` |
| Macros | UPPER_CASE | `check_return()`, `ss_print()` |
| Enums | UPPER_CASE / CamelCase values | `PIC_1080P`, `SD3403_RC_CBR`, `sd3403_sns_type` |
| Structs | CamelCase, `sd3403_` prefix in `def.h` | `sd3403_venc_chn_param` |
| SDK types | `hi_*` / `td_*` prefixes kept as-is | `hi_s32`, `td_u32` |

### SDK Types (from `source/def.h` and `svp_npu.hpp`)
- `hi_s32`/`td_s32`: return type (HI_SUCCESS=0, failure=negative); `hi_bool`/`td_bool`: boolean;
  `hi_void`/`td_void`: void; `hi_u32`/`hi_u8`/`hi_ulong`: unsigned ints; `hi_char`/`td_char`: char;
  `hi_float`: float

## Error Handling & Logging

Return `hi_s32` from functions that can fail. Use `check_return`:
```cpp
check_return(hi_mpi_sys_bind(&bind_attr), "sys bind");
```
It prints a red error (function/line/hex code) and `return`s the error code.

For multi-step init with cleanup, use the goto pattern (see
`source/sd3403_main.cpp`):
```cpp
hi_s32 ret = HI_SUCCESS;
if (init_module() != HI_SUCCESS) {
    ss_print("init failed!\n");
    goto init_failed;
}
return HI_SUCCESS;
init_failed:
    disable_module();
    return ret;
```

NPU code may also use `svp_check_exps_return()` / `svp_check_exps_goto()`.

Logging macros (all in `source/def.h`):
```cpp
ss_print("Info %d\n", value);       // prints "[func]-line: msg"
svp_trace_err("Error: %d\n", err);  // red-colored stderr
svp_trace_info("Info: %s\n", info); // plain stderr
```
It prints a red error (function/line/hex code) and `return`s the error code.

For multi-step init with cleanup, use the goto pattern (see
`source/sd3403_main.cpp`):
```cpp
hi_s32 ret = HI_SUCCESS;
if (init_module() != HI_SUCCESS) {
    ss_print("init failed!\n");
    goto init_failed;
}
return HI_SUCCESS;
init_failed:
    disable_module();
    return ret;
```

NPU code may also use `svp_check_exps_return()` / `svp_check_exps_goto()`.

Logging macros (all in `source/def.h`):
```cpp
ss_print("Info %d\n", value);       // prints "[func]-line: msg"
svp_trace_err("Error: %d\n", err);  // red-colored stderr
svp_trace_info("Info: %s\n", info); // plain stderr
```

## Memory & Threading

- Prefer `nullptr`; legacy code still uses `NULL` (e.g. `Fifo_Buffer.hpp`) — match the file
- Use `std::thread`; store heap-allocated thread pointers as members
  (`new std::thread(...)`); always join/detach them in `stop_all_thread()`
- Use `sem_t` (`sem_post`/`sem_wait`) for signal-handler sync
- NPU work buffers must be freed before model unload; follow create→execute→destroy
  sequence in `svp_npu.hpp`; always release MPP resources on shutdown
  (disable order in `sd3403_main.cpp`)

## Project Structure

```
source/           main(), def.h (macros, types, shared structs)
sys/vi/vpss/      MPP system, video input, video processing (SD3403_SYS/VI/VPSS)
venc/rgn/         H.264 encoding, region/overlay (SD3403_VENC/RGN)
manage/common/    orchestrator (SD3403_Manage), shared helpers
fifo_buffer/      thread-safe FIFO (FifoBuffer, List, PTH_LOCK)
queue/            ring queue (ringqueue)
svp_npu/          NPU inference + tracking (SVP_NNN)
rtsp/             live555 RTSP server
examples/         model (obj_detec), decode (sd3403_decode)
doc/              design notes (Chinese)
3rdLibrary/       vendored opencv/rtsp/openssl
tools/            source builds of opencv (not part of the build)
build/            CMake output
```

## Key Patterns

### SDK Initialization (source/sd3403_main.cpp)
1. `manage.enable_sys()` - init system/MPP
2. `manage.enable_vi_vpss()` - sensor + video input/processing
3. `manage.enable_venc()` - H.264 encoding
4. `manage.Run_Fifo_Thread()` - start FIFO/stream threads
5. `manage.start_rtsp()` / `enable_rtsp()` - RTSP streaming

### Compile Switches
- `RTSP_LIVE555_ON` — defined in `CMakeLists.txt:74` for `sd3403_obj`; switches
  RTSP between live555 and the socket-based `GetFrame` path
- `PROC_OLD` — defined in `source/def.h:416`; selects the legacy NPU model
  init/inference path in `svp_npu.cpp` and `obj_detection.cpp`

### Sensor Types (`sd3403_sns_type` in def.h)
- `OV_OS08A20_MIPI_8M_30FPS_12BIT`, `OV_OS04A10_MIPI_4M_30FPS_12BIT`
- `SONY_IMX485_MIPI_8M_30FPS_12BIT`, `SONY_IMX347_SLAVE_MIPI_4M_30FPS_12BIT`

## Important Notes

1. Cross-compilation is required — do not build natively on x86
2. No automated tests or CI; manual verification on target hardware
3. SS928 SDK must exist at the hardcoded `SS928_SDK` path
4. Vendored `.so` libs are linked by `file(GLOB ...)`; do not reorder/remove
5. Include `"def.h"` for common types, structs, and macros
6. `obj_detec` needs model/image files present in its working directory
