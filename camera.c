#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>

#include "camera.h"
#include "lcd.h"
#include "event.h"

// timelapse settings 
extern long glo_frames;
extern long glo_interval;
extern long glo_delay;

// control thread execution 
static volatile int glo_done;

// gphoto2 context and camera 
static GPContext *glo_context;
static Camera *glo_camera;

// pthread
static pthread_t glo_thread;
static pthread_attr_t glo_attr;

//-----------------------------------------------------------------------------
static void ctx_error_fn(GPContext *context, const char *str, void *data)
{
    fprintf(stderr, "\n*** Context error ***\n%s\n",str);
    fflush(stderr);
}

//-----------------------------------------------------------------------------
static void ctx_status_fn(GPContext *context, const char *str, void *data)
{
    fprintf(stderr, "%s\n", str);
    fflush(stderr);
}

//-----------------------------------------------------------------------------
// prepares variables to use gphoto2 and pthread:  
void gphoto2_pthread_init() 
{
    // gphoto2
	glo_context = gp_context_new();
    gp_context_set_error_func (glo_context, ctx_error_fn, NULL);
    gp_context_set_status_func (glo_context, ctx_status_fn, NULL);

    // pthread
    pthread_attr_init(&glo_attr);
    pthread_attr_setdetachstate(&glo_attr, PTHREAD_CREATE_DETACHED);
}

//-----------------------------------------------------------------------------
// releases resources:
void gphoto2_pthread_destroy() 
{
    pthread_attr_destroy(&glo_attr);
}

//-----------------------------------------------------------------------------
int camera_init()
{
    gp_camera_new(&glo_camera);
    printf("Camera init. Takes about 10 seconds.\n");

    int retval = gp_camera_init(glo_camera, glo_context);
    if (retval != GP_OK) 
    {
        printf("ERROR: gp_camera_init() failed, retval=%d\n", retval);
        return -1;
    }

    return 0;
}

//-----------------------------------------------------------------------------
static void *camera_loop(void *arg) 
{
    CameraFilePath path;
    int retval = 0;
    long nrcaptures = 1;
    struct timeval nexttime, now;
    static char buf[32]; 

    // waiting 
    if (glo_delay > 0) 
    {
        gettimeofday(&nexttime, NULL);
        now = nexttime; 
    
        nexttime.tv_sec += glo_delay;

        lcd_clear();
        lcd_puts("Waiting         ");
        lcd_set_cursor(1, 0);
    
        while (now.tv_sec < nexttime.tv_sec) 
        {
            if (glo_done) goto out; // exit immidiatly

            int seconds = nexttime.tv_sec - now.tv_sec;        
            int s = seconds % 60; 
            int m = (seconds / 60) % 60; 
            int h = seconds / 3600;

            sprintf(buf, "%02d:%02d'%02d''      ", h, m, s);
            lcd_puts(buf);

            gettimeofday(&now, NULL);

            usleep(20000);
        }
    }


    lcd_clear();
    if (glo_frames != 0)
    {
        sprintf(buf, "Capturing  %5ld", glo_frames);
        lcd_puts(buf);
    }
    else 
    {
        lcd_puts("Capturing       ");
    }

    lcd_set_cursor(1, 0); 
    
    // start time 
    gettimeofday(&nexttime, NULL);
    now = nexttime; 

    while ( !glo_done && (glo_frames == 0 || nrcaptures <= glo_frames) ) 
    {

        int seconds = nexttime.tv_sec - now.tv_sec;
        
        int s = seconds % 60; 
        int m = (seconds / 60) % 60; 
        int h = seconds / 3600; 
   
        sprintf(buf, "%02d:%02d'%02d'' %5ld", h, m, s, nrcaptures-1);
        lcd_puts(buf);
        
        gettimeofday(&now, NULL);

        if (now.tv_sec >= nexttime.tv_sec) {
            nexttime.tv_sec += glo_interval; 

            printf("Capturing\n");
            retval = gp_camera_capture(glo_camera, GP_CAPTURE_IMAGE, &path, glo_context);
            if (retval != GP_OK) {
                printf("ERROR: gp_camera_capture() failed, retval=%d\n", retval);
                break;
            }
        
            printf("Pathname on the camera: %s/%s\n", path.folder, path.name);
            nrcaptures++;
        }

        usleep(10000); 
    }

out: 

    change_state(S_MENU);
    gp_camera_exit(glo_camera, glo_context);
    pthread_exit(NULL);
}

//-----------------------------------------------------------------------------
void camera_start_capture() 
{
    glo_done = 0; 
    pthread_create(&glo_thread, &glo_attr, camera_loop, NULL);
}

//-----------------------------------------------------------------------------
void camera_stop_capture() 
{
    glo_done = 1;
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
