//Source for functions: https://docs.libcamera.org/master/guides/application-developer.html



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

//Funktionsdefinitionen 

bool CameraDriver::initialize()

{       if (initialized_)
                return true;

        if(!acquireCamera()|| !configureCamera() || !allocateBuffers() || !createRequests())
        {
                cleanup();
                return false;
        }

        initialized_ = true;

        return true;
  
    
};

bool CameraDriver::acquireCamera(){
         //Kamera Manager Initialisieren
    cm_ = std::make_unique<libcamera::CameraManager>();
    int response = cm_->start();

    //Check Response
    if (response < 0) {
                cm_.reset();
                return false;
            }

    //Kamera Suchen
    auto cameras = cm_->cameras();
    if (cameras.empty()) {
            std::cout << "No cameras were identified on the system."
                    << std::endl;
            cm_->stop();
            cm_.reset();
            return false;
            
                }

    std::string cameraId = cameras[0]->id();
    camera_ = cm_->get(cameraId);
    if (!camera_) {    
            std::cerr << "Camera not found" << std::endl;

            // Ressourcen aufräumen
                cm_->stop();
                cm_.reset();
            return false;
            }

    int acquire = camera_->acquire();

    if (acquire < 0) {
                return false;
            }
        acquired_ = true;
  return true;

};


bool CameraDriver::configureCamera(){
     //Kamera Konfiguration - In this case the cameras configuration is beeing set automatically to the best fitting
    config_ = camera_->generateConfiguration( { libcamera::StreamRole::Viewfinder } );
        if (!config_ || config_->size() == 0) {
                        std::cout << "No configuration list" << std::endl;
                        return false;}

    libcamera::StreamConfiguration &streamConfig = config_->at(0);

    std::cout << "Default viewfinder configuration is: " << streamConfig.toString() << std::endl;
    
    auto status = config_->validate();
                if (status == libcamera::CameraConfiguration::Invalid) {
                        std::cerr << "Invalid camera configuration" << std::endl;
                        // Ressourcen aufräumen
                        return false;}

                        std::cout << "Validated viewfinder configuration is: " << streamConfig.toString() << std::endl;
    
    auto configure = camera_->configure(config_.get());
        if (configure < 0) {
                std::cerr << "Failed camera configuration" << std::endl;
                // Ressourcen aufräumen
                return false;}

                std::cout << "Stride: " << streamConfig.stride << '\n';
    
    
    return true;

}

bool CameraDriver::allocateBuffers(){
    //Speicher/Buffer Allokation 
    allocator_ = std::make_unique<libcamera::FrameBufferAllocator>(camera_);

        for (libcamera::StreamConfiguration &cfg : *config_) {
                int ret = allocator_->allocate(cfg.stream());
                        if (ret < 0) {
                                std::cerr << "Can't allocate buffers" << std::endl;
                                camera_.reset();
                                cm_->stop();
                                cm_.reset();
                                return false;
                        }

                size_t allocated = allocator_->buffers(cfg.stream()).size();
                std::cout << "Allocated " << allocated << " buffers for stream \n\n" << std::endl;
                                
                };      
                return true;

}

bool CameraDriver::createRequests(){
        libcamera::StreamConfiguration &streamConfig = config_->at(0);
        stream_ = streamConfig.stream();
        if (!stream_){

                return false;
        }
        const std::vector<std::unique_ptr<libcamera::FrameBuffer>> &buffers = allocator_->buffers(stream_);


                for (unsigned int i = 0; i < buffers.size(); ++i) {
                std::unique_ptr<libcamera::Request> request = camera_->createRequest();
                if (!request)
                {
                        std::cerr << "Can't create request" << std::endl;
                        return false;
                }

                const std::unique_ptr<libcamera::FrameBuffer> &buffer = buffers[i];
                int ret = request->addBuffer(stream_, buffer.get());
                if (ret < 0)
                {
                        std::cerr << "Can't set buffer for request"<< std::endl;
                        return false;
                }

                requests_.push_back(std::move(request));
                
                }
        return true;


};

void CameraDriver::cleanup()
{
    requests_.clear();

    allocator_.reset();
    stream_ = nullptr;
    config_.reset();

    if (camera_ && acquired_) {
        camera_->release();
    }

    acquired_ = false;
    camera_.reset();

    cm_.reset();

    initialized_ = false;
    running_ = false;
};


bool CameraDriver::start() {
        if (!initialized_){return false;};
        if(!running_){return false;};
        camera_->requestCompleted.connect(this, &CameraDriver::requestComplete); 

        camera_->start();

        for (std::unique_ptr<libcamera::Request> &request : requests_){
        
        camera_->queueRequest(request.get());
        
        
        };


        std::this_thread::sleep_for(10s);

        running_ = true;
        return true;


};

void CameraDriver::requestComplete(libcamera::Request *request) {
                if (request->status() == libcamera::Request::RequestCancelled)
                return;
 
    const std::map<const libcamera::Stream *, libcamera::FrameBuffer *> &buffers = request->libcamera::Request::buffers();

    for (auto bufferPair : buffers) {   
        libcamera::FrameBuffer *buffer = bufferPair.second;
        const libcamera::FrameMetadata &metadata = buffer->metadata();

        std::cout << "seq: " << std::setw(6) << std::setfill('0') << metadata.sequence << " bytesused: ";
        
        unsigned int nplane = 0;
        for (const libcamera::FrameMetadata::Plane &plane : metadata.planes())
        {
                std::cout << plane.bytesused;
                        if (++nplane < metadata.planes().size()) std::cout << "/";
        }

        std::cout << std::endl;

            
    
        for (const libcamera::FrameBuffer::Plane &plane : buffer->planes()) {
                std::cout << "FD: " << plane.fd.get() << '\n';
                std::cout << "Offset: " << plane.offset << '\n';
                std::cout << "Length: " << plane.length << '\n';
                void *ptr = mmap(nullptr, plane.length, PROT_READ, MAP_SHARED, plane.fd.get(), plane.offset);
                        if (ptr == MAP_FAILED) {
                        std::cerr << "mmap fehlgeschlagen\n";
                        continue;}

    
                        std::cout << "Adress: " << ptr << "\n\n\n";
                        cv::Mat image(600, 800, CV_8UC4, ptr, 3200);
                        cv::Mat bgr;
                        cv::cvtColor(image, bgr, cv::COLOR_BGRA2BGR);
                        cv::imwrite("frame.png", bgr);

                        munmap(ptr, plane.length);
                        
                        }

    }
    
    request->reuse(libcamera::Request::ReuseBuffers);
    camera_->queueRequest(request);

   

};