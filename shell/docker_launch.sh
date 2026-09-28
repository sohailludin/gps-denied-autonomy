#!/bin/bash

docker run -it --rm \
  --privileged \
  --network host \
  --ipc host \
  -v /dev:/dev \
  --device=/dev:/dev \
  --device=/dev/video0:/dev/video0 \
  -v /run/udev:/run/udev:ro \
  -v $(pwd):/workspace \
  gps-denied:ros_system_updated