
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
#include <opencv4/core/mat.hpp>



using namespace libcamera;
using namespace std::chrono_literals;
static std::shared_ptr<Camera> camera;
static void requestComplete(Request *request);


int main()
{

std::unique_ptr<CameraManager> cm = std::make_unique<CameraManager>();

cm->start();

for (const auto &camera : cm->cameras())
    std::cout << camera->id() << std::endl;

auto cameras = cm->cameras();

if (cameras.empty()) {
    std::cout << "No cameras were identified on the system."
              << std::endl;
    cm->stop();
    return EXIT_FAILURE;
        }

std::string cameraId = cameras[0]->id();

camera = cm->get(cameraId);

camera->acquire();

//Kamera Konfiguration - In this case the cameras configuration is beeing set automatically to the best fitting

std::unique_ptr<CameraConfiguration> config = camera->generateConfiguration( { StreamRole::Viewfinder } );
StreamConfiguration &streamConfig = config->at(0);
std::cout << "Default viewfinder configuration is: " << streamConfig.toString() << std::endl;
config->validate();
std::cout << "Validated viewfinder configuration is: " << streamConfig.toString() << std::endl;
camera->configure(config.get());
std::cout << "Stride: " << streamConfig.stride << '\n';

//Memory Allocation
auto allocator = std::make_unique<FrameBufferAllocator>(camera);

for (StreamConfiguration &cfg : *config) {
    int ret = allocator->allocate(cfg.stream());
    if (ret < 0) {
        std::cerr << "Can't allocate buffers" << std::endl;
        return -ENOMEM;
    }

    size_t allocated = allocator->buffers(cfg.stream()).size();
    std::cout << "Allocated " << allocated << " buffers for stream \n\n" << std::endl;
}


Stream *stream = streamConfig.stream();
const std::vector<std::unique_ptr<FrameBuffer>> &buffers = allocator->buffers(stream);

std::vector<std::unique_ptr<Request>> requests;

for (unsigned int i = 0; i < buffers.size(); ++i) {
    std::unique_ptr<Request> request = camera->createRequest();
    if (!request)
    {
        std::cerr << "Can't create request" << std::endl;
        return -ENOMEM;
    }

    const std::unique_ptr<FrameBuffer> &buffer = buffers[i];
    int ret = request->addBuffer(stream, buffer.get());
    if (ret < 0)
    {
        std::cerr << "Can't set buffer for request"
              << std::endl;
        return ret;
    }

    requests.push_back(std::move(request));
  
}

camera->requestCompleted.connect(requestComplete); 

camera->start();



for (std::unique_ptr<Request> &request : requests){
   camera->queueRequest(request.get());
   
}


std::this_thread::sleep_for(2s);






camera -> stop();



requests.clear();

allocator->free(stream);
allocator.reset();

camera->release();
camera.reset();

cameras.clear();


cm->stop();
return 0;

}



static void requestComplete(Request *request)
{
   if (request->status() == Request::RequestCancelled)
    return;
 
    const std::map<const Stream *, FrameBuffer *> &buffers = request->Request::buffers();

    for (auto bufferPair : buffers) {
    FrameBuffer *buffer = bufferPair.second;
    const FrameMetadata &metadata = buffer->metadata();

    std::cout << "seq: " << std::setw(6) << std::setfill('0') << metadata.sequence << " bytesused: ";
    
    unsigned int nplane = 0;
    for (const FrameMetadata::Plane &plane : metadata.planes())
    {
        std::cout << plane.bytesused;
        if (++nplane < metadata.planes().size()) std::cout << "/";
    }

    std::cout << std::endl;

            
    
    for (const FrameBuffer::Plane &plane : buffer->planes()) {
    std::cout << "FD: " << plane.fd.get() << '\n';
    std::cout << "Offset: " << plane.offset << '\n';
    std::cout << "Length: " << plane.length << '\n';
    void *ptr = mmap(nullptr, plane.length, PROT_READ, MAP_SHARED, plane.fd.get(), plane.offset);
    std::cout << "Adress: " << ptr << "\n\n\n";
    return ptr;
    }
    cv::

}       
}
 
