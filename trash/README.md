# GPS-Denied Autonomy

A Raspberry Pi 5-based experimental platform for **GPS-denied localization and visual-inertial navigation**.

The project is being built **bottom-up**, starting with reliable sensor acquisition and Linux hardware interfaces, then moving through ROS 2 integration, timing, calibration, camera integration and eventually Visual-Inertial Odometry (VIO).

> **Current status:** A custom ICM-20948 C++ driver is running on real hardware and publishes accelerometer + gyroscope data as a standard ROS 2 `sensor_msgs/msg/Imu` stream at ~100 Hz.

---

## Why this project?

GPS-denied localization is not only an estimation problem. A useful system depends on a trustworthy chain from physical hardware to the estimator:

```text
Sensor hardware
      ↓
Linux device interface
      ↓
Driver
      ↓
Physical units
      ↓
ROS 2 transport
      ↓
Timing + calibration
      ↓
Camera / IMU fusion
      ↓
6-DoF pose estimate
```

Instead of starting with a high-level IMU library, I implemented the sensor path from the Linux I²C interface upward to understand and control the full stack.

The long-term goal is a compact system that estimates motion in environments where GNSS/GPS is unavailable.

---

## Current Engineering Status

| Subsystem | Status |
|---|---|
| Raspberry Pi / sensor hardware | ✅ Working |
| Linux I²C communication | ✅ Working |
| ICM-20948 accelerometer | ✅ Working |
| ICM-20948 gyroscope | ✅ Working |
| ICM-20948 temperature sensor | ✅ Working |
| AK09916 magnetometer | ✅ Working |
| Custom C++ IMU driver | ✅ Working |
| ROS 2 IMU publisher | ✅ Working |
| `/imu/data_raw` ROS topic | ✅ Working |
| ~100 Hz ROS publish rate | ✅ Verified |
| Camera ROS 2 integration | 🚧 Next |
| Camera / IMU timing | ⏳ Planned |
| Sensor calibration pipeline | ⏳ Planned |
| Visual-Inertial Odometry | ⏳ Planned |
| 6-DoF GPS-denied localization | ⏳ Planned |
| Health monitoring / recovery | ⏳ Planned |

---

## What I Implemented

### 1. Custom ICM-20948 C++ Driver

The IMU driver was implemented directly against the ICM-20948 datasheet using Linux I²C interfaces rather than wrapping a high-level sensor library.

Implemented functionality includes:

- opening `/dev/i2c-*` from C++
- selecting the ICM-20948 I²C slave address with `ioctl`
- register reads and writes through `libi2c`
- ICM-20948 register-bank switching
- accelerometer configuration and acquisition
- gyroscope configuration and acquisition
- temperature acquisition
- signed 16-bit reconstruction from high / low sensor bytes
- sensitivity handling based on sensor configuration
- conversion from raw sensor values into physical units
- basic I²C transaction error handling

### Output units

| Sensor | Driver output |
|---|---:|
| Accelerometer | `m/s²` |
| Gyroscope | `rad/s` |
| Magnetometer | `T` |
| Temperature | `°C` |

The driver is intentionally independent from ROS 2 so the hardware layer can be tested separately.

---

### 2. AK09916 Magnetometer Integration

The ICM-20948 contains an AK09916 magnetometer that is accessed through the ICM-20948's auxiliary I²C master.

The communication path is:

```text
Raspberry Pi 5
      |
      | Linux I²C
      v
ICM-20948 @ 0x69
      |
      | Auxiliary I²C master
      v
AK09916 @ 0x0C
```

This required configuring the ICM-20948 in two different roles:

- **I²C slave** toward the Raspberry Pi
- **I²C master** toward the AK09916

The magnetometer path was validated experimentally using stationary and moving measurements.

---

### 3. ROS 2 IMU Integration

The standalone sensor driver was integrated into a ROS 2 Humble C++ node.

Current data path:

```text
ICM-20948
    ↓
Linux I²C
    ↓
Custom C++ driver
    ↓
Accelerometer + Gyroscope
    ↓
ROS 2 ImuPublisher
    ↓
sensor_msgs/msg/Imu
    ↓
/imu/data_raw
```

The ROS 2 node:

- initializes the I²C device and IMU once at startup
- configures the accelerometer and gyroscope
- periodically reads new sensor values
- maps the measurements into `sensor_msgs/msg/Imu`
- adds a ROS timestamp
- uses the frame ID `imu_link`
- marks orientation as unavailable
- publishes the message on `/imu/data_raw`

The current timer period is **10 ms**, corresponding to a nominal **100 Hz** publish rate.

Measured using:

```bash
ros2 topic hz /imu/data_raw
```

Observed:

```text
average rate: ~100.00 Hz
```

This verifies the end-to-end path from physical sensor hardware to a standard ROS 2 topic.

> The current 100 Hz measurement validates the ROS software path. Sensor ODR, acquisition coherence, timestamp accuracy and burst reads will be treated separately before VIO integration.

---

## Software Architecture

The project is separated by responsibility:

```text
Higher-Level Autonomy / Estimation
              ↑
              |
            ROS 2
              ↑
              |
        IMU ROS Adapter
              ↑
              |
       ICM-20948 Driver
              ↑
              |
       Linux I²C / libi2c
              ↑
              |
           Hardware
```

### Responsibility split

**ICM-20948 driver**
- hardware communication
- register access
- sensor configuration
- raw-value conversion
- physical-unit output

**ROS 2 IMU node**
- node lifecycle
- acquisition timing
- ROS message creation
- timestamping
- topic publishing

This separation prevents ROS-specific logic from leaking into the hardware driver and makes the driver independently testable.

---

## Hardware

| Component | Device |
|---|---|
| Compute | Raspberry Pi 5, 4 GB |
| Camera | ArduCam OV9281 global-shutter camera |
| IMU | TDK / InvenSense ICM-20948 |
| Magnetometer | AK09916 integrated with the ICM-20948 |

---

## Software Stack

| Layer | Technology |
|---|---|
| Operating system | Linux / Raspberry Pi OS |
| Containerization | Docker |
| Robotics middleware | ROS 2 Humble |
| Runtime language | C++ |
| Hardware interface | Linux `i2c-dev` / `libi2c` |
| ROS build system | `ament_cmake` / `colcon` |
| General build tooling | CMake / GCC |
| Computer vision | OpenCV |
| Analysis / tooling | Python |

---

## ROS 2 Package Structure

Current ROS package:

```text
imu_driver_ros/
├── include/
│   └── imu_driver_ros/
│       └── icm.hpp
├── src/
│   ├── icm20948.cpp
│   └── imu_publisher.cpp
├── CMakeLists.txt
└── package.xml
```

The surrounding ROS workspace follows the standard layout:

```text
workspace/
├── src/
│   └── imu_driver_ros/
├── build/
├── install/
└── log/
```

`build/`, `install/` and `log/` are generated artifacts and are not part of the source code.

---

## Build and Run

The project is developed inside a ROS 2 Humble Docker environment.

From the ROS workspace root:

```bash
cd /workspace
colcon build --packages-select imu_driver_ros
source install/setup.bash
```

Run the IMU publisher:

```bash
ros2 run imu_driver_ros imu_publisher
```

Inspect the published messages:

```bash
ros2 topic echo /imu/data_raw
```

Check the publish frequency:

```bash
ros2 topic hz /imu/data_raw
```

---

## Engineering Topics Covered

This project has required practical work across several abstraction layers:

- Linux file descriptors
- Linux character devices
- `/dev/i2c-*`
- `ioctl`
- I²C slave addressing
- register-level sensor programming
- register-bank switching
- bit manipulation
- two's-complement sensor data
- signed 16-bit reconstruction
- sensor sensitivity and unit conversion
- auxiliary I²C communication
- C++ interface / implementation separation
- driver vs application responsibility
- ROS 2 nodes
- ROS 2 publishers and topics
- ROS messages
- callbacks and timers
- ROS workspaces
- `package.xml`
- CMake
- `ament_cmake`
- `colcon`
- Docker-based hardware development
- physical coordinate frames
- sensor timing and synchronization concepts

---

## Validation Performed

The current implementation has been tested on physical hardware.

Validation includes:

- successful I²C communication with the ICM-20948
- accelerometer response to orientation / movement
- gyroscope response to rotational motion
- stationary gyroscope measurements near zero with expected sensor noise
- temperature readout
- AK09916 magnetic-field measurements
- stationary and moving magnetometer tests
- ROS 2 message publication
- ROS timestamps and `imu_link` frame assignment
- continuous `/imu/data_raw` publication
- measured ROS publish rate of approximately 100 Hz

---

## Current Limitations

The current implementation is a functional engineering prototype, not a production-ready navigation system.

Not yet implemented:

- coherent burst reads
- precise hardware timestamp synchronization
- finalized sensor output data rates
- accelerometer / gyroscope bias calibration
- magnetometer hard-iron / soft-iron calibration
- camera–IMU extrinsic calibration
- camera–IMU time synchronization
- covariance estimation
- VIO
- drift characterization
- automatic health monitoring
- watchdogs / recovery mechanisms
- production-grade error propagation

These are intentionally treated as later milestones rather than hidden behind the current prototype status.

---

## Next Milestones

1. Integrate the OV9281 global-shutter camera into ROS 2
2. Validate camera frame rate and timestamps
3. Improve IMU acquisition with coherent / burst reads
4. Characterize IMU noise and bias
5. Build the calibration pipeline
6. Determine camera–IMU extrinsics
7. Synchronize camera and IMU timing
8. Integrate a VIO estimator
9. Evaluate trajectory drift on real-world test sequences
10. Add diagnostics, health monitoring and recovery behavior

---

## Long-Term Goal

The long-term objective is a self-contained GPS-denied localization module that can be powered on and provide a continuously updated 6-DoF pose estimate from camera and IMU data.

Target pipeline:

```text
Global-Shutter Camera ──────┐
                            │
                            ├──> Timing / Calibration
                            │          ↓
IMU ────────────────────────┘        VIO
                                       ↓
                                6-DoF Pose
                                       ↓
                              ROS 2 / Vehicle API
```

Later iterations will focus on robustness, degraded-sensor behavior, diagnostics and recovery.

---

## Design Philosophy

The project prioritizes **understanding, measurable behavior and clean system boundaries** over quickly connecting pre-built components.

Each layer is developed and validated before relying on it at the next level:

```text
Hardware
   ↓
Reliable sensor acquisition
   ↓
Physical measurements
   ↓
ROS 2 transport
   ↓
Timing + calibration
   ↓
Visual-inertial estimation
   ↓
GPS-denied 6-DoF localization
```

The goal is not only to make the system run, but to understand why it works, where it can fail, and how each layer contributes to the final state estimate.

---

## Project Status

**Active development.**

The current milestone is a verified sensor-to-ROS backbone. The next major phase is camera integration and visual-inertial estimation.
