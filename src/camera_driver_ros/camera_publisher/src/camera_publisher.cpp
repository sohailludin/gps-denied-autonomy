
//Bibliotheken

#include <iomanip>
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <vector>



#include "../include/camera.hpp"
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/image.hpp"


using namespace std::chrono_literals;



class CameraPublisher : public rclcpp::Node
{
    CameraDriver driver_;

    private: 
        CameraPublisher()
        : Node ("cam_publisher")
        {
            driver_.initialize();
            driver_.start();
            publisher_ = this -> create_publisher<sensor_msgs::msg::Image>("camera/image_raw");
            timer_ = this->create_wall_timer(
            10ms, std::bind(&CameraPublisher::timer_callback, this));
        }

        void cam_callback(){

            auto image_raw = sensor_msgs::msg::Image();
            cv::Mat lib_image = driver_.getFrame();

                if (lib_image.empty())
                {return ;
                }

            image_raw.height = lib_image.rows;
            image_raw.width = lib_image.cols;
            image_raw.step = lib_image.elemSize() * lib_image.cols;
            image_raw.encoding = "bgr8"
            
            auto byte_size = lib_image.rows * lib_image.cols * lib_image.elemSize();
            auto end_adress = lib_image.data + byte_size;

            bool continuous = lib_image.isContinuous();

                if (continuous) {

                image_raw.data.assign(lib_image.data,  end_adress);
                }
            
            image_raw.header.stamp = this -> now();
            image_raw.header.frame_id = "camera_link";
            publisher_->publish(image_raw);
            

           
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

