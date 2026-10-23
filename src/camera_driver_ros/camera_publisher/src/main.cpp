
//Bibliotheken

#include <iomanip>
#include <iostream>
#include <memory>
#include <thread>

#include <libcamera/camera.h>
#include <libcamera/camera_manager.h>
#include <libcamera/framebuffer_allocator.h> 
#include <libcamera/stream.h> 
#include<libcamera/framebuffer.h>
#include <sys/mman.h>
#include <opencv2/core/mat.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>


#include "../include/camera.hpp"

using namespace std::chrono_literals;


int main() 
{
    CameraDriver driver;

    driver.initialize();
    driver.start();

    std::this_thread::sleep_for(std::chrono::seconds(10));

    driver.stop();

    return 0;
}