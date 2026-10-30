
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


class CameraPublisher : public rclcpp::Node
{

    private: 
        CameraPublisher()
        : Node ("cam_publisher")
        {
            driver_.initialize();
            driver_.start();
            publisher_ = this -> create_publisher<sensor_msgs::msg::Image>("camera/image_raw");
           
        }

        void cam_callback(){

            auto image_raw = sensor_msgs::msg::Image();
            cv::Mat lib_image = driver.getFrame();
            image_raw.height = lib_image.rows;
            image_raw.width = lib_image.width;
            image_raw.encoding = lib_image.flags;
            while (i < lib_image.rows) {
                while(j < lib_image.step) {

                    image_raw.data = lib_image.data;
                }
            }
            
            image_raw.header.stamp = this -> now();
            image_raw.header.frame_id = "camera_link"
            publisher_->publish(image_raw);
            

            if (image_clone.empty())
            {return cv::Mat{};}

        }

 rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr publisher_;
}



int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<CameraPublisher>());
  rclcpp::shutdown();
  return 0;
}

