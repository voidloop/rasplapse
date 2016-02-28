#ifndef __TIMELAPSE__H__
#define __TIMELAPSE__H__
#include <gphoto2/gphoto2-camera.h>

void create_context();
void camera_init();
void camera_exit();
void camera_loop();

#endif
