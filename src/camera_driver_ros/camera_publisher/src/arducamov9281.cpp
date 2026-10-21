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


//Funktionsdefinitionen 

bool CameraDriver::initialize()
{
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
            return false;
            }

    int acquire = camera_->acquire();

    if (acquire < 0) {
                camera_.reset();
                cameras.clear();


                cm_->stop();
                cm_.reset();
                return false;
            }

    //Kamera Konfiguration - In this case the cameras configuration is beeing set automatically to the best fitting
    config_ = camera_->generateConfiguration( { libcamera::StreamRole::Viewfinder } );
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
    
    config_->at(0);
    if (!config_ || config_->size() == 0) {
            std::cout << "No configuration list" << std::endl;
            return false;}


    return true;

  
    
};
