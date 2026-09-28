#!/bin/bash
source /opt/ros/humble/setup.bash

ros2 run camera_ros camera_node \
  --ros-args \
  -p camera_name:=ov9281 \
  -p image_size:=[1280,800] \
  -p frame_rate:=30.0 \
  -p camera_info_url:=file:///workspace/ov9281.yaml