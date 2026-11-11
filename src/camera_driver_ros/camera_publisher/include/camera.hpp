
#include <iomanip>
#include <iostream>
#include <memory>
#include <thread>
#include <mutex>
#include <vector>
#include <cstdint>
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
#include <atomic>


class CameraDriver
{
public:
    bool initialize();
    bool start();
    void stop();
    
    ~CameraDriver();
    struct CameraData {
        cv::Mat image_data;
        uint64_t sequence_number;
        uint64_t time_stamp;
    };
    struct CameraData getFrame();


private:
    bool initialized_ = false; //Sind alle Ressourcen für die Aufnahme vorbereitet ?
    std::atomic<bool> running_{false};  // Läuft die Aufnahme gerade ? 
    bool acquired_ = false; //Ist die Kamera reserviert ?
    bool acquireCamera();
    bool configureCamera();
    bool allocateBuffers();
    bool createRequests();
    void cleanup();
    std::mutex mtx;
    CameraData Data;

    void requestComplete(libcamera::Request *request);

    std::unique_ptr<libcamera::CameraManager> cm_;
    std::shared_ptr<libcamera::Camera> camera_;
    std::unique_ptr<libcamera::CameraConfiguration> config_;
    std::unique_ptr<libcamera::FrameBufferAllocator> allocator_;

    libcamera::Stream *stream_ = nullptr;

    std::vector<std::unique_ptr<libcamera::Request>> requests_;

    
};