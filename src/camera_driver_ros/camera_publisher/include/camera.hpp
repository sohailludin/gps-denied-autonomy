
#include <iomanip>
#include <iostream>
#include <memory>
#include <thread>
//#include "camera_driver_ros/camera.hpp"

#include <libcamera/camera.h>
#include <libcamera/camera_manager.h>
#include <libcamera/framebuffer_allocator.h> 
#include <libcamera/stream.h> 
#include<libcamera/framebuffer.h>
#include <sys/mman.h>
#include <opencv2/core/mat.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>


class CameraDriver
{
public:
    bool initialize();
    bool start();
    void stop();
    bool status() const;

private:
    bool initialized_ = false;
    bool running_ = false;

    std::unique_ptr<libcamera::CameraManager> cm_;
    std::shared_ptr<libcamera::Camera> camera_;
    std::unique_ptr<libcamera::CameraConfiguration> config_;
    std::unique_ptr<libcamera::FrameBufferAllocator> allocator_;

    libcamera::Stream *stream_ = nullptr;

    std::vector<std::unique_ptr<libcamera::Request>> requests_;

    void requestComplete(libcamera::Request *request);
};