# GPS-Denied Autonomy

A Raspberry Pi 5-based C++/ROS 2 platform for developing GPS-denied localization with a global-shutter camera and an IMU.

**Current milestone — 18 November 2026:** Both sensor pipelines publish data through ROS 2 Humble. The custom OV9281 camera driver is integrated into a ROS 2 node, publishing images on `/camera/image_raw` at **~29.8–29.9 Hz**. A separate ROS 2 subscriber successfully saved and displayed an **800 × 600** camera image. The IMU pipeline publishes accelerometer and gyroscope data on `/imu/data_raw` at approximately **100 Hz**. **Camera–IMU time synchronization, calibration, and visual-inertial odometry (VIO) are not yet implemented or validated.**

---

## Tech Stack

### Hardware
- Raspberry Pi 5, 4 GB
- Arducam OV9281 global-shutter monochrome camera
- TDK/InvenSense ICM-20948 IMU
- Integrated AK09916 magnetometer

### Software
- **Language:** C++17
- **OS:** Linux / Raspberry Pi OS
- **Containerization:** Docker
- **Robotics middleware:** ROS 2 Humble
- **IMU interface:** Linux `i2c-dev`, `ioctl`, `libi2c`
- **Camera interface:** `libcamera` / Raspberry Pi PiSP pipeline
- **Computer vision:** OpenCV
- **Build tooling:** GCC, CMake, `ament_cmake`, `colcon`, `pkg-config`
- **libcamera build:** Meson + Ninja
- **Analysis / tooling:** Python

---

## Architecture

```text
                         GPS-Denied Autonomy
                                  │
                    ┌─────────────┴─────────────┐
                    │                           │
                 CAMERA                        IMU
               Arducam OV9281              ICM-20948
                    │                           │
                    ▼                           ▼
                libcamera              Linux I²C / libi2c
                    │                           │
                    ▼                           ▼
             C++ CameraDriver           Custom C++ IMU Driver
                    │                           │
                    ▼                           ▼
             ROS 2 CameraPublisher        ROS 2 ImuPublisher
                    │                           │
                    ▼                           ▼
             /camera/image_raw             /imu/data_raw
               ~29.8–29.9 Hz                 ~100 Hz
                    │                           │
                    └─────────────┬─────────────┘
                                  ▼
                       [planned] Common Timing,
                         Sensor Calibration
                                  │
                                  ▼
                       [planned] Visual-Inertial
                                Odometry
                                  │
                                  ▼
                        [goal] 6-DoF Pose Estimate
                                  │
                                  ▼
                       [goal] ROS 2 / Vehicle API
```

### Current implementation

**IMU path — operational in ROS 2**

```text
ICM-20948
→ Linux I²C
→ Custom C++ driver
→ SI-unit conversion
→ ROS 2 ImuPublisher
→ sensor_msgs/msg/Imu
→ /imu/data_raw @ ~100 Hz
```

Accelerometer, gyroscope, temperature, and AK09916 magnetometer access are implemented. Accelerometer and gyroscope measurements are published through ROS 2. IMU calibration and noise/bias characterization remain open.

**Camera path — operational in ROS 2**

```text
OV9281
→ Raspberry Pi camera stack / PiSP
→ libcamera
→ CameraDriver::initialize()
   → acquireCamera()
   → configureCamera()
   → allocateBuffers()
   → createRequests()
→ CameraDriver::start()
   → requestCompleted callback
   → frame metadata + buffer access (mmap)
   → OpenCV conversion to BGR
   → mutex-protected latest-frame snapshot
   → request reuse + requeue
→ CameraPublisher (ROS 2 timer)
   → CameraDriver::getFrame()
   → skip already-published sequence numbers
   → sensor_msgs/msg/Image (bgr8)
→ /camera/image_raw @ ~29.8–29.9 Hz
→ independent ROS 2 image_saver subscriber (verified)
```

The `CameraDriver` separates acquisition, stream configuration, buffer allocation, request construction, capture control, and resource cleanup. A `CameraData` structure groups the latest OpenCV image, libcamera frame sequence number, and camera timestamp. The producer updates these together under a mutex; the getter creates a consistent snapshot, including an independent copy of the pixel data.

The sensor is monochrome, but the current implementation converts the capture buffer to **`bgr8`** for ROS 2 publication. A future `mono8` path could avoid unnecessary conversion and reduce bandwidth.

### Verified camera results — 18 November 2026

- The OV9281 is detected and configured through libcamera inside the Docker-based ROS 2 environment.
- Framebuffer allocation, queued capture requests, completion callbacks, and requeueing operate continuously; sequence numbers increase across captured frames.
- The ROS 2 node builds and starts using libcamera **0.7.2**.
- `/camera/image_raw` publishes at approximately **29.8–29.9 messages/s** in a sustained `ros2 topic hz` test.
- The transmitted image was saved by the independent ROS 2 `image_saver` node and visually inspected: **800 × 600 pixels**, recognizable grayscale scene.
- `header.frame_id` is set to `camera_frame`.
- The current `header.stamp` uses **`this->now()` at publisher processing time**, not the original sensor acquisition timestamp. This is intentionally temporary and **not sufficient for VIO synchronization**.
- Earlier standalone testing also exercised `start() → stop() → start() → stop()`. Clean ROS 2 shutdown and all failure paths are **not yet independently verified**.

### Build and run the ROS 2 camera publisher

Run these commands **inside the configured Docker container** from the ROS 2 workspace root (`/workspace` in the tested environment). This assumes the ROS 2 Humble dependencies and the locally built libcamera/PiSP stack are already available.

```bash
cd /workspace
source /opt/ros/humble/setup.bash

colcon build --packages-select camera_publisher --symlink-install \
  --cmake-args -DPKG_CONFIG_USE_CMAKE_PREFIX_PATH=FALSE

source install/setup.bash
ros2 run camera_publisher camera_publisher
```

In another container terminal, with the ROS 2 environment sourced:

```bash
ros2 topic hz /camera/image_raw
ros2 topic echo /camera/image_raw --once --field header
```

To capture a single image without a graphical display, install the ROS 2 image tools if necessary and run the saver node:

```bash
apt update && apt install -y ros-humble-image-view

ros2 run image_view image_saver \
  --ros-args \
  -r image:=/camera/image_raw \
  -p save_all_image:=false \
  -p filename_format:=/workspace/camera_test%04i.jpg
```

In a separate terminal, trigger a save and inspect the generated file:

```bash
ros2 service call /save std_srvs/srv/Empty '{}'
```

`rqt_image_view` is also an option when a graphical display is available. In the tested headless container, it failed with `could not connect to display`; `image_saver` worked as an alternative.

### Standalone camera test build

For the separate standalone test application, from the directory containing `main.cpp` and `arducamov9281.cpp`:

```bash
g++ -std=c++17 main.cpp arducamov9281.cpp \
  $(pkg-config --cflags --libs libcamera opencv4) \
  -o main

./main
```

This builds the standalone test executable, **not** the ROS 2 camera publisher.

### Build/debugging note: multiple libcamera installations

The container had two libcamera installations. A ROS 2 build initially loaded **libcamera 0.1.0** from `/opt/ros/humble/lib`, despite a separate **libcamera 0.7.2** installation under `/usr/local/lib/aarch64-linux-gnu`.

- `pkg-config --modversion libcamera` in the shell reported `0.7.2`.
- CMake's `FindPkgConfig` initially found version `0.1.0` because additional CMake prefix paths influenced package discovery.
- Building with `-DPKG_CONFIG_USE_CMAKE_PREFIX_PATH=FALSE` allowed CMake to discover `0.7.2` in the tested environment.
- `ldd` was used to verify that the resulting executable loaded `libcamera.so.0.7`.
- A separate missing `libpisp` runtime dependency was diagnosed and fixed locally; the precise fix is not documented here.

Useful diagnostic commands:

```bash
pkg-config --modversion libcamera
pkg-config --variable=libdir libcamera
ldd /workspace/install/camera_publisher/lib/camera_publisher/camera_publisher | grep libcamera
```

Do **not** mix library files or create cross-version symlinks to work around incompatible libcamera SONAMEs. The `rosdep` key `libcamera` resolves to `ros-humble-libcamera` in this environment; avoid inadvertently replacing or mixing it with the local `/usr/local` installation.

---

## Road Map

| Stage | Status |
|---|---|
| Low-level ICM-20948 C++ driver | ✅ Complete |
| Accelerometer / gyroscope acquisition | ✅ Complete |
| AK09916 magnetometer access | ✅ Complete |
| ROS 2 IMU publisher | ✅ Complete |
| ~100 Hz `/imu/data_raw` stream | ✅ Verified |
| Build Raspberry Pi libcamera inside Docker | ✅ Complete |
| Detect OV9281 from custom C++ application | ✅ Complete |
| Configure camera stream | ✅ Tested |
| Allocate frame buffers and prepare capture requests | ✅ Tested |
| Receive camera frames and completion callbacks | ✅ Tested |
| Refactor capture into reusable `CameraDriver` class | ✅ Implemented |
| Repeated standalone camera start / stop | ✅ Tested |
| Mutex-protected camera-frame handoff | ✅ Implemented |
| ROS 2 camera publisher (`/camera/image_raw`) | ✅ Running and tested |
| Sustained ~29.8–29.9 Hz ROS 2 image publication | ✅ Verified |
| Save and visually inspect image via independent ROS 2 subscriber | ✅ Verified |
| Per-frame sequence deduplication | ✅ Implemented; detailed validation pending |
| Clean shutdown, destructor and error-path cleanup | 🚧 Verification pending |
| Camera capture-time timestamps on a shared IMU time basis | ⏳ Not yet implemented |
| Camera / IMU timestamp validation | ⏳ Planned |
| IMU noise + bias characterization | ⏳ Planned |
| Camera intrinsic calibration | ⏳ Planned |
| Camera–IMU extrinsic + temporal calibration | ⏳ Planned |
| Visual-Inertial Odometry integration | ⏳ Planned |
| Drift / trajectory evaluation | ⏳ Planned |
| Diagnostics, watchdogs and recovery | ⏳ Planned |

---

## Next Engineering Steps

1. **Preserve and harden the working camera pipeline:** verify a clean ROS 2 shutdown, request cancellation/requeue behavior, partial-initialization cleanup, and errors returned by `queueRequest()`. Reduce verbose per-frame debug output.
2. **Validate image semantics and performance:** confirm encoding, `step`, buffer continuity, frame identity, and dropped/duplicated-frame behavior; prefer early duplicate checks before pixel copying. Derive width, height, format, and stride from the negotiated `libcamera` configuration instead of hard-coded values.
3. **Establish correct camera acquisition timestamps:** verify the reference clock of the specific libcamera timestamp in use; map it onto a common time base and compare it with IMU measurement timing. The current `this->now()` header is only a transport smoke-test timestamp.
4. **Calibrate sensors:** determine OV9281 intrinsics/distortion, IMU biases/noise, and camera–IMU spatial and temporal alignment.
5. **Prototype visual-inertial estimation:** bring the validated streams into an initial VIO pipeline, then evaluate estimated trajectory, drift, and robustness.

**Known prototype constraints:** Current capture processing uses hard-coded `800 × 600`, `CV_8UC4`, and a `3200`-byte stride; images are converted to BGR and republished as `bgr8`. The ROS 2 publisher checks sequence numbers to avoid publishing the same latest frame repeatedly, but exact frame-level deduplication and dropped-frame behavior have not been separately measured. The image timestamps currently reflect ROS 2 processing time, not camera exposure time. Start/stop behavior, error handling, bandwidth, and long-duration stability need additional testing before the system is considered a reliable synchronized sensing backbone.

---

## Goals

### Near-term

Establish reliable, temporally aligned sensor streams:

```text
OV9281 capture timestamps + ICM-20948 measurement timestamps
                           ↓
                 synchronized ROS 2 streams
```

### Engineering goal

Build and understand the complete sensor-to-estimation stack rather than treating camera, IMU, and VIO as black boxes. The system should have clear boundaries between:

- hardware access
- drivers
- ROS 2 transport
- timing and calibration
- state estimation
- health monitoring

### Long-term

Deliver a self-contained prototype that estimates a continuously updated **6-DoF pose without GNSS/GPS** using visual-inertial data, with measurable drift, robust sensor handling, and a clean ROS 2 interface for downstream autonomous systems.
