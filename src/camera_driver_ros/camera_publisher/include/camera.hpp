//Bibliotheken
#include <iomanip>
#include <iostream>
#include <memory>
#include <thread>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <cmath>
#include <string>
#include <tuple>
#include <iomanip>
#include <string>

extern "C" {

#include <libcamera/libcamera.h>

}

using namespace std;

//Funktionendeklaration

void start_camera_manager();
int camera_devices_detected(static camera, unique_ptr camera_manager);
void get_camera_by_Id(string camera_id);