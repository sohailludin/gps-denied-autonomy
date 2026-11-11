
//Bibliotheken

#include <iomanip>
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <vector>
#include <stdexcept>
#include <time.h>
#include <ctime>



#include "../include/camera.hpp"
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/image.hpp"


using namespace std::chrono_literals;



class CameraPublisher : 

public rclcpp::Node
{
    public:
        CameraDriver driver_;
        
        CameraPublisher()
            : Node ("cam_publisher")
            {
                if (driver_.initialize()) {;
                
                if (driver_.start()) {;
                publisher_ = this -> create_publisher<sensor_msgs::msg::Image>("camera/image_raw", 10);
                timer_ = this->create_wall_timer(
                10ms, std::bind(&CameraPublisher::cam_callback, this));
                }
                else 
                    throw std::runtime_error("Camera start failed");

            
            }
        
           else 
                   throw std::runtime_error("Camera initialization failed");
                }

    private: 
        uint64_t last_sequence_number_published = 0;
        bool first_sequence_published = false;

        void cam_callback(){

            auto image_raw = sensor_msgs::msg::Image();
            CameraDriver::CameraData lib_image = driver_.getFrame();

                if (lib_image.image_data.empty())
                {return ;
                }

            image_raw.height = lib_image.image_data.rows;
            image_raw.width = lib_image.image_data.cols;
            image_raw.step = lib_image.image_data.elemSize() * lib_image.image_data.cols;
            image_raw.encoding = "bgr8";
            
            auto byte_size = lib_image.image_data.rows * lib_image.image_data.cols * lib_image.image_data.elemSize();
            auto end_adress = lib_image.image_data.data + byte_size;

            bool continuous = lib_image.image_data.isContinuous();

                if (continuous) {

                image_raw.data.assign(lib_image.image_data.data,  end_adress);


                    if (last_sequence_number_published == lib_image.sequence_number && first_sequence_published == true) {
                        return;
                    }
                    first_sequence_published = true;
           
                        
                
                    int64_t time = this -> now().nanoseconds();
                    clock_gettime(CLOCK_MONOTONIC, &start)
                    int64_t monotonictime = start.tv_sec * 1'000'000'000LL + start.tv_nsec;
                    int64_t offset = time - monotonictime;  

                    image_raw.header.stamp.sec = (offset + lib_image.time_stamp) / 1000000000  ;
                    image_raw.header.stamp.nanosec = (offset + lib_image.time_stamp) % 1000000000;


                    image_raw.header.frame_id = "camera_frame";
                    publisher_->publish(image_raw);
                    last_sequence_number_published = lib_image.sequence_number;
                }
            
        
            

        };

 rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr publisher_;
 rclcpp::TimerBase::SharedPtr timer_;
};



int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<CameraPublisher>());
  rclcpp::shutdown();
  return 0;
}

