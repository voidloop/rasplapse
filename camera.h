#ifndef __CAMERA__H__
#define __CAMERA__H__
#include <gphoto2/gphoto2-camera.h>

void gphoto2_pthread_init();
void gphoto2_pthread_destroy();
void camera_start_capture();
void camera_stop_capture();
int  camera_init();

#endif
