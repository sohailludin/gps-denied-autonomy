
#include <iomanip>
#include <iostream>
#include <memory>
#include <thread>
//#include "camera_driver_ros/camera.hpp"

#include <libcamera/camera.h>
#include <libcamera/camera_manager.h>


using namespace libcamera;
using namespace std::chrono_literals;
static std::shared_ptr<Camera> camera;

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


}
