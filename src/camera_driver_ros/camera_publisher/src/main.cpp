
#include <iomanip>
#include <iostream>
#include <memory>
#include <thread>
//#include "camera_driver_ros/camera.hpp"

#include <libcamera/camera.h>
#include <libcamera/camera_manager.h>
#include <libcamera/framebuffer_allocator.h> 
#include <libcamera/stream.h> 


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


//Memory Allocation
auto allocator = std::make_unique<FrameBufferAllocator>(camera);

for (StreamConfiguration &cfg : *config) {
    int ret = allocator->allocate(cfg.stream());
    if (ret < 0) {
        std::cerr << "Can't allocate buffers" << std::endl;
        return -ENOMEM;
    }

    size_t allocated = allocator->buffers(cfg.stream()).size();
    std::cout << "Allocated " << allocated << " buffers for stream" << std::endl;
}

camera->start();
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



for (std::unique_ptr<Request> &request : requests)
   {camera->queueRequest(request.get());}


camera->requestCompleted.connect(requestComplete);

for (std::unique_ptr<Request> &request : requests)
   camera->queueRequest(request.get());




camera->stop();
allocator->free(stream);
allocator.reset();
camera->release();
camera.reset();
cm->stop();

return 0;

}



static void requestComplete(Request *request)
{
   if (request->status() == Request::RequestCancelled)
   return;
   const std::map<const Stream *, FrameBuffer *> &buffers = request->buffers();

}