FROM ros:humble-ros-base

# ROS / project dependencies
RUN apt-get update && apt-get install -y \
    ros-humble-cv-bridge \
    ros-humble-image-transport \
    ros-humble-vision-opencv \
    ros-humble-sensor-msgs \
    ros-humble-rclcpp \
    ros-humble-std-msgs \
    ros-humble-camera-ros \
    ros-humble-v4l2-camera \
    libopencv-dev \
    libi2c-dev \
    libjpeg-dev \
    libgphoto2-dev \
    libevent-dev \
    && rm -rf /var/lib/apt/lists/*

# Dependencies required to build libcamera
RUN apt-get update && apt-get install -y \
    git \
    g++ \
    pkg-config \
    meson \
    ninja-build \
    python3-pip \
    python3-yaml \
    python3-ply \
    python3-jinja2 \
    libyaml-dev \
    libgnutls28-dev \
    openssl \
    libboost-dev \
    libglib2.0-dev \
    libgstreamer-plugins-base1.0-dev \
    && rm -rf /var/lib/apt/lists/*
RUN python3 -m pip install --upgrade meson
RUN meson --version

# Build Raspberry Pi libcamera from source
WORKDIR /opt

RUN git clone --depth 1 https://github.com/raspberrypi/libcamera.git


WORKDIR /opt/libcamera

RUN meson setup build \
    --buildtype=release \
    -Dpipelines=rpi/vc4,rpi/pisp \
    -Dipas=rpi/vc4,rpi/pisp \
    -Dv4l2=true \
    -Dgstreamer=disabled \
    -Dtest=false \
    -Dlc-compliance=disabled \
    -Dcam=disabled \
    -Dqcam=disabled \
    -Ddocumentation=disabled \
    -Dpycamera=disabled

RUN ninja -C build
RUN ninja -C build install

WORKDIR /workspace

CMD ["bash"]