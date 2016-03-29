#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <gphoto2/gphoto2-camera.h>

#include "camera.h"
#include "lcd.h"

// timelapse settings 
extern long glob_frames;
extern long glob_interval;
extern long glob_delay;

// gphoto2 context
static GPContext *main_context;

// variables to control camera thread 
static volatile int thread_done = 1;
static pthread_mutex_t mutex;
static pthread_cond_t condm; // master
static pthread_cond_t condw; // worker

//-----------------------------------------------------------------------------

static int
_lookup_widget(CameraWidget*widget, const char *key, CameraWidget **child) 
{
	int ret;
	ret = gp_widget_get_child_by_name (widget, key, child);
	if (ret < GP_OK)
		ret = gp_widget_get_child_by_label (widget, key, child);
	return ret;
}

//-----------------------------------------------------------------------------

static int
get_config_value_string(Camera *camera, const char *key, char **str, GPContext *context) 
{
	CameraWidget		*widget = NULL, *child = NULL;
	CameraWidgetType	type;
	int			ret;
	char			*val;

	ret = gp_camera_get_config (camera, &widget, context);
	if (ret < GP_OK) {
		fprintf (stderr, "camera_get_config failed: %d\n", ret);
		return ret;
	}
	ret = _lookup_widget (widget, key, &child);
	if (ret < GP_OK) {
		fprintf (stderr, "lookup widget failed: %d\n", ret);
		goto out;
	}

	/* This type check is optional, if you know what type the label
	 * has already. If you are not sure, better check. */
	ret = gp_widget_get_type (child, &type);
	if (ret < GP_OK) {
		fprintf (stderr, "widget get type failed: %d\n", ret);
		goto out;
	}
	switch (type) {
        case GP_WIDGET_MENU:
        case GP_WIDGET_RADIO:
        case GP_WIDGET_TEXT:
		break;
	default:
		fprintf (stderr, "widget has bad type %d\n", type);
		ret = GP_ERROR_BAD_PARAMETERS;
		goto out;
	}

	/* This is the actual query call. Note that we just
	 * a pointer reference to the string, not a copy... */
	ret = gp_widget_get_value (child, &val);
	if (ret < GP_OK) {
		fprintf (stderr, "could not query widget value: %d\n", ret);
		goto out;
	}
	/* Create a new copy for our caller. */
	*str = strdup (val);
out:
	gp_widget_free (widget);
	return ret;
}

//-----------------------------------------------------------------------------

static int
set_config_value_string(Camera *camera, const char *key, const char *val, GPContext *context)
{
	CameraWidget		*widget = NULL, *child = NULL;
	CameraWidgetType	type;
	int			ret;

	ret = gp_camera_get_config (camera, &widget, context);
	if (ret < GP_OK) {
		fprintf (stderr, "camera_get_config failed: %d\n", ret);
		return ret;
	}
	ret = _lookup_widget (widget, key, &child);
	if (ret < GP_OK) {
		fprintf (stderr, "lookup widget failed: %d\n", ret);
		goto out;
	}

	/* This type check is optional, if you know what type the label
	 * has already. If you are not sure, better check. */
	ret = gp_widget_get_type (child, &type);
	if (ret < GP_OK) {
		fprintf (stderr, "widget get type failed: %d\n", ret);
		goto out;
	}
	switch (type) {
        case GP_WIDGET_MENU:
        case GP_WIDGET_RADIO:
        case GP_WIDGET_TEXT:
		/* This is the actual set call. Note that we keep
		 * ownership of the string and have to free it if necessary.
		 */
		ret = gp_widget_set_value (child, val);
		if (ret < GP_OK) {
			fprintf (stderr, "could not set widget value: %d\n", ret);
			goto out;
		}
		break;
        case GP_WIDGET_TOGGLE: {
		int ival;

		sscanf(val,"%d",&ival);
		ret = gp_widget_set_value (child, &ival);
		if (ret < GP_OK) {
			fprintf (stderr, "could not set widget value: %d\n", ret);
			goto out;
		}
		break;
	}
	default:
		fprintf (stderr, "widget has bad type %d\n", type);
		ret = GP_ERROR_BAD_PARAMETERS;
		goto out;
	}

	/* This stores it on the camera again */
	ret = gp_camera_set_config (camera, widget, context);
	if (ret < GP_OK) {
		fprintf (stderr, "camera_set_config failed: %d\n", ret);
		return ret;
	}
out:
	gp_widget_free (widget);
	return ret;
}


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

void timelapse_init() 
{
    // gphoto2
	main_context = gp_context_new();
    gp_context_set_error_func(main_context, ctx_error_fn, NULL);
    gp_context_set_status_func(main_context, ctx_status_fn, NULL);

    // initialize mutex and condition variables object
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&condw, NULL);
    pthread_cond_init(&condm, NULL);
}


//-----------------------------------------------------------------------------

static void *timelapse_thread(void *arg) 
{
    Camera *camera = (Camera *) arg;
    CameraFilePath path;
    int ret, sec;

    long nrcaptures = 1;
    struct timeval next, now;
    struct timespec ts; 
    static char buf[32]; 

    if (glob_delay != 0) 
    {
        gettimeofday(&next, NULL);
        now = next;
        next.tv_sec += glob_delay;

        // lock 'thread_done' 
        pthread_mutex_lock(&mutex); 

        lcd_clear();
        lcd_puts("Waiting");
        lcd_set_cursor(1, 0);
       
        while (!thread_done && now.tv_sec < next.tv_sec) 
        {
            // print remaining time to lcd 
            sec = next.tv_sec - now.tv_sec;            
            sprintf(buf, "%02d:%02d'%02d''", sec/3600, (sec/60)%60, sec%60);
            lcd_puts(buf);

            // wait 100 millis
            ts.tv_sec = now.tv_sec;
            ts.tv_nsec = (now.tv_usec + 100000) * 1000;
            pthread_cond_timedwait(&condw, &mutex, &ts);

            gettimeofday(&now, NULL);
        }

        // notify master
        pthread_cond_signal(&condm);

        // unlock 'thread_done'
        pthread_mutex_unlock(&mutex);
    }

    // lock 'thread_done'
    pthread_mutex_lock(&mutex);

    // clear lcd and print a title
    lcd_clear();
    if (glob_frames != 0)
        sprintf(buf, "Capturing  %5ld", glob_frames);
    else 
        sprintf(buf, "Capturing");
    lcd_puts(buf);
    lcd_set_cursor(1, 0); 
    
    // start time 
    gettimeofday(&next, NULL);
    now = next; 

    while (!thread_done && (glob_frames == 0 || nrcaptures <= glob_frames)) 
    {
        sec = next.tv_sec - now.tv_sec;
        if (sec < 0) sec = 0; 
        sprintf(buf, "%02d:%02d'%02d'' %5ld", sec/3600, (sec/60)%60, sec%60, nrcaptures-1);
        lcd_puts(buf);
        
        if (now.tv_sec >= next.tv_sec) {
            next.tv_sec += glob_interval; 

            printf("Capturing\n");
            ret = gp_camera_capture(camera, GP_CAPTURE_IMAGE, &path, main_context);
            if (ret != GP_OK) {
                printf("gp_camera_capture() failed: %d\n", ret);
                break;
            }
        
            printf("Pathname on the camera: %s/%s\n", path.folder, path.name);
            nrcaptures++;
        }

        // wait 100 millis
        ts.tv_sec = now.tv_sec;
        ts.tv_nsec = (now.tv_usec + 100000) * 1000;
        pthread_cond_timedwait(&condw, &mutex, &ts);
        gettimeofday(&now, NULL);
    }

    gp_camera_exit(camera, main_context);

    thread_done = 1;

    // notify master
    pthread_cond_signal(&condm);

    // unlock 'thread_done'
    pthread_mutex_unlock(&mutex);

    //pthread_exit(NULL);
    return NULL;
}


//-----------------------------------------------------------------------------

int timelapse_start()
{        
    Camera *camera;
    pthread_t thread;
    pthread_attr_t attr;
    int ret;   

    printf("Camera init. Takes about 10 seconds.\n");
    gp_camera_new(&camera); 

    ret = gp_camera_init(camera, main_context);
    if (ret < GP_OK) 
    {
        fprintf(stderr, "gp_camera_init() failed: %d\n", ret);
        return -1;
    }

    char target[] = "Memory card";
    ret = set_config_value_string(camera, "capturetarget", target, main_context);
    if (ret < GP_OK)
    {
        fprintf(stderr, "gp_camera_init() failed: %d\n", ret);
        return -1;
    }

    // start a new thread to capture images
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    
    thread_done = 0;
    pthread_create(&thread, &attr, timelapse_thread, (void *) camera);
    pthread_attr_destroy(&attr);

    return 0;
}


//-----------------------------------------------------------------------------

void timelapse_stop() 
{
    // lock 'thread_done'
    pthread_mutex_lock(&mutex);

    // if there is a worker then stop it
    if (thread_done==0) 
    {        
        thread_done = 1;
        pthread_cond_signal(&condw);

        // wait worker signal
        pthread_cond_wait(&condm, &mutex);
    }

    // unlock 'thread_done'
    pthread_mutex_unlock(&mutex);
}

//-----------------------------------------------------------------------------

void timelapse_destroy( void )
{
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&condw);
    pthread_cond_destroy(&condm);
}

//-----------------------------------------------------------------------------
/*static int wait_for_event(long waittime) 
{&mutex
	CameraEventType	evtype;
	CameraFilePath	*path;
    int ret = 0;
    void *data = NULL; 

	evtype = GP_EVENT_UNKNOWN;
	ret = gp_camera_wait_for_event(glob_camera, waittime, &evtype, &data, main_context);
	if (ret != GP_OK) {
		return ret;
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
