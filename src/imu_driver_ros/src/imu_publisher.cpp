// Copyright 2016 Open Source Robotics Foundation, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <vector>


#include "imu_driver_ros/icm.hpp"
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/imu.hpp"

const int addr = 0x69;
__u8 pwr = 0x06;
using namespace std;

using namespace std::chrono_literals;

/* This example creates a subclass of Node and uses std::bind() to register a
 * member function as a callback from the timer. */

class ImuPublisher : public rclcpp::Node
{
public:
  ImuPublisher()
  : Node("imu_publisher")
  {
    file_ = linux_i2c_file_open("/dev/i2c-1");
    linux_i2c_bus_adress_access(file_, addr);
    icm_cancel_sleep_mode(file_, pwr);
    
    acc_sens_ = configure_accelerometer(file_);
    gyro_sens_ = configure_gyroskop(file_);
    publisher_ = this->create_publisher<sensor_msgs::msg::Imu>("imu/data_raw", 10);
    timer_ = this->create_wall_timer(
     10ms, std::bind(&ImuPublisher::timer_callback, this));
    
  }

private:
  void timer_callback()
        {

          
          struct Vector Acc = read_accelerometer(file_, acc_sens_);
          struct Vector Gyro = read_gyroksop(file_, gyro_sens_);


          auto message = sensor_msgs::msg::Imu();
          message.linear_acceleration.x = Acc.x;
          message.linear_acceleration.y = Acc.y;
          message.linear_acceleration.z = Acc.z;

          message.angular_velocity.x = Gyro.x;
          message.angular_velocity.y = Gyro.y;
          message.angular_velocity.z = Gyro.z;

          message.orientation_covariance[0] = -1.0;
          message.header.stamp = this->now();
          message.header.frame_id = "imu_link";

          publisher_->publish(message);
        }
        int file_;
        int acc_sens_;
        float gyro_sens_;
        rclcpp::TimerBase::SharedPtr timer_;
        rclcpp::Publisher<sensor_msgs::msg::Imu>::SharedPtr publisher_;
        
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<ImuPublisher>());
  rclcpp::shutdown();
  return 0;
}
