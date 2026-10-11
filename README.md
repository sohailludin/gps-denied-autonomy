# GPS-Denied Autonomy

A Raspberry Pi 5-based C++/ROS 2 platform for GPS-denied localization using a global-shutter camera and IMU.

**Current milestone:** the IMU pipeline is operational end-to-end in ROS 2. The OV9281 camera is detected and accessible through a custom C++ `libcamera` application; request/buffer handling is currently being implemented before ROS 2 camera integration.

---

## Tech Stack

### Hardware
- Raspberry Pi 5, 4 GB
- ArduCam OV9281 global-shutter monochrome camera
- TDK/InvenSense ICM-20948 IMU
- Integrated AK09916 magnetometer

### Software
- **Language:** C++
- **OS:** Linux / Raspberry Pi OS
- **Containerization:** Docker
- **Robotics middleware:** ROS 2 Humble
- **IMU interface:** Linux `i2c-dev`, `ioctl`, `libi2c`
- **Camera interface:** `libcamera` / Raspberry Pi PiSP pipeline
- **Computer vision:** OpenCV
- **Build tooling:** CMake, `ament_cmake`, `colcon`, GCC
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
          Custom C++ Camera Layer       Custom C++ IMU Driver
                    │                           │
                    │                           ▼
                    │                   ROS 2 IMU Publisher
                    │                           │
                    │                    /imu/data_raw
                    │                           │
                    └─────────────┬─────────────┘
                                  ▼
                       Timing + Calibration
                                  │
                                  ▼
                    Visual-Inertial Odometry
                                  │
                                  ▼
                         6-DoF Pose Estimate
                                  │
                                  ▼
                         ROS 2 / Vehicle API
```

### Current implementation

**IMU path — operational**

```text
ICM-20948
→ Linux I²C
→ Custom C++ driver
→ SI-unit conversion
→ ROS 2 ImuPublisher
→ sensor_msgs/msg/Imu
→ /imu/data_raw @ ~100 Hz
```

Accelerometer, gyroscope, temperature and AK09916 magnetometer access are implemented. Accelerometer and gyroscope data are currently published through ROS 2.

**Camera path — in development**

```text
OV9281
→ Linux camera stack
→ Raspberry Pi PiSP
→ libcamera
→ Custom C++ camera application
→ Request / Buffer handling
→ [next] Frame acquisition
→ [next] ROS 2 image publisher
```

The custom C++ application can initialize `libcamera`, discover the OV9281 and access the Raspberry Pi camera pipeline. Frame request and buffer handling are the current focus.

---

## Road Map

| Stage | Status |
|---|---|
| Low-level ICM-20948 C++ driver | ✅ Complete |
| Accelerometer / gyroscope acquisition | ✅ Complete |
| AK09916 magnetometer access | ✅ Complete |
| ROS 2 IMU publisher | ✅ Complete |
| ~100 Hz `/imu/data_raw` stream | ✅ Verified |
| Build current Raspberry Pi `libcamera` inside Docker | ✅ Complete |
| Detect OV9281 from custom C++ application | ✅ Complete |
| Configure camera stream | 🚧 In progress |
| Implement request + buffer lifecycle | 🚧 In progress |
| Receive first camera frames | ⏳ Next |
| ROS 2 camera publisher | ⏳ Planned |
| Camera / IMU timestamp validation | ⏳ Planned |
| IMU noise + bias characterization | ⏳ Planned |
| Camera intrinsic calibration | ⏳ Planned |
| Camera–IMU extrinsic + temporal calibration | ⏳ Planned |
| Visual-Inertial Odometry integration | ⏳ Planned |
| Drift / trajectory evaluation | ⏳ Planned |
| Diagnostics, watchdogs and recovery | ⏳ Planned |

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
Build and understand the complete sensor-to-estimation stack rather than treating camera, IMU and VIO as black boxes. The system should have clear boundaries between:

- hardware access
- drivers
- ROS 2 transport
- timing and calibration
- state estimation
- health monitoring

### Long-term
Deliver a self-contained prototype that estimates a continuously updated **6-DoF pose without GNSS/GPS** using visual-inertial data, with measurable drift, robust sensor handling and a clean ROS 2 interface for downstream autonomous systems.
