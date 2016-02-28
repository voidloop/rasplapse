#include <stdlib.h>
#include <stdio.h>
#include "timelapse.h"

extern int glo_total_frames;
extern GPContext *glo_context;
Camera *glo_camera;



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
static int wait_event_and_download(int *nrdownloads) 
{
	CameraEventType	evtype;
	CameraFilePath *path;
    void *data = NULL;
    int retval = 0;
    int waittime = 100;
	
    retval = gp_camera_wait_for_event(glo_camera, waittime, &evtype, &data, glo_context);
    if (retval != GP_OK) {
        return retval;
    } 

    switch (evtype) {
        case GP_EVENT_CAPTURE_COMPLETE:
            printf("wait for event CAPTURE_COMPLETE\n");
            break;
        case GP_EVENT_UNKNOWN:
        case GP_EVENT_TIMEOUT:
            break;
        case GP_EVENT_FOLDER_ADDED:
            printf("wait for event FOLDER_ADDED\n");
            break;
        case GP_EVENT_FILE_ADDED:
            path = data;
            printf("File %s / %s added to queue.\n", path->folder, path->name);
            (*nrdownloads)++;
            break;
    } // switch 
    
	return GP_OK;
}

//-----------------------------------------------------------------------------
void camera_loop() 
{
    int retval = 0;
    int nrtriggers = 1;
    int nrdownloads = 1; 

    
    while (glo_total_frames == 0 || nrdownloads <= glo_total_frames) {

       
    } // while
}


