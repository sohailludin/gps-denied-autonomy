
//Bibliotheken

#include <iomanip>
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>



#include "../include/camera.hpp"
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/image.hpp"


using namespace std::chrono_literals;
CameraDriver driver_;

pointer = driver_.requestComplete.&ptr;

class CameraPublisher : public rclcpp::Node
{

    private: 
        CameraPublisher()
        : Node ("cam_publisher")
        {
            driver_.initialize();
            publisher_ = this -> create_publisher<sensor_msgs::msg::Image>("camera/image_raw");
           
        }

        void cam_callback(){
            driver_.start();
            auto image_raw = sensor_msgs::msg::Image();
            image_raw.data = driver_.requestComplete.image(600, 800, CV_8UC4, ptr, 3200)
            image_raw.header.stamp = this -> now();
            image_raw.header.frame_id = "camera_link"
            publisher_->publish(image_raw);

            std::this_thread::sleep_for(std::chrono::seconds(10));
            driver.stop();

            if (image_clone.empty())
    {return cv::Mat{};}

        }

    rclcpp::TimerBase::SharedPtr timer_;
  rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr publisher_;
}



int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<CameraPublisher>());
  rclcpp::shutdown();
  return 0;
}

