#!/bin/bash

sudo apt update && sudo apt upgrade -y

sudo bash shell/docker_launch.sh

cd src/camera_driver_ros/camera_publisher/src