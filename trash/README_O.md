# GPS-Denied Autonomy

A Raspberry Pi 5-based C++/ROS 2 platform for GPS-denied localization using a global-shutter camera and IMU.

**Current milestone (23 October 2026):** the IMU pipeline is operational end-to-end in ROS 2. A standalone C++ `CameraDriver` for the OV9281 now acquires frames through `libcamera`, including stream configuration, buffer allocation, request submission, and completion callbacks. Repeated start/stop capture has been tested. **Clean camera shutdown still needs verification, and camera frames are not yet published to ROS 2.**

---

## Tech Stack

### Hardware
- Raspberry Pi 5, 4 GB
- ArduCam OV9281 global-shutter monochrome camera
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
- **Build tooling:** GCC, CMake, `ament_cmake`, `colcon`
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
               ArduCam OV9281              ICM-20948
                    │                           │
                    ▼                           ▼
                libcamera              Linux I²C / libi2c
                    │                           │
                    ▼                           ▼
             C++ CameraDriver           Custom C++ IMU Driver
                    │                           │
                    ▼                           ▼
          [planned] ROS 2 Image          ROS 2 IMU Publisher
                 Publisher                      │
                    │                           │
                    ▼                           ▼
           /camera/image_raw               /imu/data_raw
                    │                           │
                    └─────────────┬─────────────┘
                                  ▼
                      [planned] Timing + Calibration
                                  │
                                  ▼
                       [planned] Visual-Inertial
                                Odometry
                                  │
                                  ▼
                        [goal] 6-DoF Pose Estimate
                                  │
                                  ▼
                           ROS 2 / Vehicle API
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

Accelerometer, gyroscope, temperature, and AK09916 magnetometer access are implemented. Accelerometer and gyroscope measurements are published through ROS 2.

**Camera path — standalone C++ capture working; ROS 2 integration pending**

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
   → frame metadata + mapped buffer access (mmap)
   → OpenCV frame conversion / test image output
   → request reuse + requeue
→ CameraDriver::stop()
→ [next] safe frame handoff to a ROS 2 publisher
→ [next] sensor_msgs/msg/Image on /camera/image_raw
```

The refactor separates camera acquisition, stream configuration, buffer allocation, request construction, capture control, and resource cleanup into a `CameraDriver` class. The current test code uses OpenCV to convert captured frames and write a `frame.png` for inspection. This is a debugging step, **not** the final frame-delivery mechanism.

### Latest camera test results

- OV9281 discovered and configured through `libcamera`.
- Frame buffers allocated and capture requests submitted successfully.
- `requestCompleted` callbacks received with frame metadata; image-buffer mapping and OpenCV processing exercised.
- Capture runs for approximately 10 seconds in the standalone test.
- A `start() → stop() → start() → stop()` test also ran for approximately 20 seconds.
- **Open issue:** the process previously logged `ERROR Camera camera.cpp:600 Removing camera while still in use` during exit. Final destruction/release behavior must be retested after wiring `cleanup()` into the `CameraDriver` destructor. Successful frame capture does not yet imply a clean shutdown.

### Standalone camera test build

From the directory containing `main.cpp` and `arducamov9281.cpp`, with the relevant development libraries available:

```bash
g++ -std=c++17 main.cpp arducamov9281.cpp \
  $(pkg-config --cflags --libs libcamera opencv4) \
  -o main

./main
```

This command builds the standalone test executable, **not** a ROS 2 camera-publisher node.

---

## Road Map

| Stage | Status |
|---|---|
| Low-level ICM-20948 C++ driver | ✅ Complete |
| Accelerometer / gyroscope acquisition | ✅ Complete |
| AK09916 magnetometer access | ✅ Complete |
| ROS 2 IMU publisher | ✅ Complete |
| ~100 Hz `/imu/data_raw` stream | ✅ Verified |
| Build Raspberry Pi `libcamera` inside Docker | ✅ Complete |
| Detect OV9281 from custom C++ application | ✅ Complete |
| Configure camera stream | ✅ Tested |
| Allocate frame buffers and prepare capture requests | ✅ Tested |
| Receive camera frames and completion callbacks | ✅ Tested |
| Refactor capture into reusable `CameraDriver` class | ✅ Implemented |
| Repeated camera start / stop | ✅ Tested |
| Clean shutdown, destructor and error-path cleanup | 🚧 Verification pending |
| Replace per-frame PNG writes with safe frame handoff | ⏳ Next |
| ROS 2 camera publisher (`/camera/image_raw`) | ⏳ Next |
| Camera / IMU timestamp validation | ⏳ Planned |
| IMU noise + bias characterization | ⏳ Planned |
| Camera intrinsic calibration | ⏳ Planned |
| Camera–IMU extrinsic + temporal calibration | ⏳ Planned |
| Visual-Inertial Odometry integration | ⏳ Planned |
| Drift / trajectory evaluation | ⏳ Planned |
| Diagnostics, watchdogs and recovery | ⏳ Planned |

---

## Next Engineering Steps

1. **Finalize lifecycle handling:** ensure `stop()` prevents request requeueing, disconnects the callback, and completes before buffers are destroyed; use `cleanup()` for partial initialization failures and object destruction. Confirm that the libcamera shutdown warning no longer occurs.
2. **Separate capture from processing:** avoid synchronous `cv::imwrite()` in the completion callback. Copy or otherwise transfer completed frame data safely before the corresponding libcamera buffer is reused.
3. **Create a ROS 2 camera publisher:** wrap `CameraDriver` in a node and publish frames as `sensor_msgs/msg/Image` on `/camera/image_raw`, with consistent encoding, dimensions, stride handling, and timestamps.
4. **Validate the sensor streams:** test the camera topic alongside `/imu/data_raw`, then measure timing and begin camera/IMU calibration.

**Known prototype constraints:** the current OpenCV conversion path uses hard-coded `800 × 600`, `CV_8UC4`, and a stride of `3200` bytes. These must be derived from the negotiated `libcamera` stream configuration before treating the driver as format-independent. Callback concurrency and start/stop error paths also need testing beyond the successful capture runs.

---

## Goals

### Near-term
Establish a reliable synchronized sensor backbone:

```text
OV9281 frames + ICM-20948 measurements
                  ↓
        timestamped ROS 2 streams
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
