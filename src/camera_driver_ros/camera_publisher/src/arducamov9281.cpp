//Source for functions: https://docs.libcamera.org/master/guides/application-developer.html


//Bibliotheken
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <cmath>
#include <string>
#include <tuple>
#include <iomanip>

//#include "camera_driver_ros/camera.hpp"


extern "C" {

#include <libcamera/libcamera.h>

}



//Funktionsdefinitionen

//Kamera Manager starten 
void start_camera_manager(){
std::unique_ptr<Camera_Manager> cm = std::make_unique<CameraManager>();
cm -> start();

return cm; 
}


int camera_devices_detected_id(static camera, unique_ptr camera_manager){
for (const auto &camera : cm->cameras())
    std::cout << camera->id() << std::endl;
return 0;
}


void available_camera();