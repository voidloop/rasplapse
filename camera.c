#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include "timelapse.h"


extern GPContext *glo_context;
Camera *glo_camera;

extern int glo_frames;
extern int glo_interval;

//-----------------------------------------------------------------------------
void camera_init()
{
    gp_camera_new(&glo_camera);
    printf("Camera init. Takes about 10 seconds.\n");

    int retval = gp_camera_init(glo_camera, glo_context);
    if (retval != GP_OK) {
        printf("ERROR: gp_camera_init() failed, retval=%d\n", retval);
        exit(1);
    }
}

//-----------------------------------------------------------------------------
void camera_exit() 
{
    gp_camera_exit(glo_camera, glo_context);
}

//-----------------------------------------------------------------------------
void camera_loop() 
{
    CameraFilePath path;
    int retval = 0;
    int nrcaptures = 1;
    struct timeval next_time, now;

    // start time 
    gettimeofday(&next_time, NULL);

    while (glo_frames == 0 || nrcaptures <= glo_frames) {

        gettimeofday(&now, NULL);

        if (now.tv_sec > next_time.tv_sec) {
            next_time.tv_sec += glo_interval; 

            printf("Capturing\n");
            retval = gp_camera_capture(glo_camera, GP_CAPTURE_IMAGE, &path, glo_context);
            if (retval != GP_OK) {
                printf("ERROR: gp_camera_capture() failed, retval=%d\n", retval);
                return;
            }
        
            printf("Pathname on the camera: %s/%s\n", path.folder, path.name);
            nrcaptures++;
        }

        usleep(500); 
    } // while
}






//-----------------------------------------------------------------------------
/*static int wait_for_event(long waittime) 
{
	CameraEventType	evtype;
	CameraFilePath	*path;
    int retval = 0;
    void *data = NULL; 

	evtype = GP_EVENT_UNKNOWN;
	retval = gp_camera_wait_for_event(glo_camera, waittime, &evtype, &data, glo_context);
	if (retval != GP_OK) {
		return retval;
    }

	path = data;
	switch (evtype) {
    case GP_EVENT_TIMEOUT:
        break;
    case GP_EVENT_CAPTURE_COMPLETE:
        break;
    case GP_EVENT_FOLDER_ADDED:
        printf("FOLDER_ADDED %s/%s during wait, ignoring.\n", path->folder, path->name);
        break;
    case GP_EVENT_FILE_ADDED:
        printf("Pathname on the camera: %s/%s\n", path->folder, path->name);
        break;
    case GP_EVENT_UNKNOWN:
        break;
    default:
        printf("Unknown event type %d during wait, ignoring.\n", evtype);
        break;
	}

    free(data);

    return GP_OK;
}*/
