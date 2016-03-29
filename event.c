#include <stdio.h>
#include <pigpio.h>
#include <pthread.h>
#include <errno.h>

#include "event.h"
#include "lcd.h"
#include "camera.h"
#include "ui.h"

// program state
static int prog_state = S_MENU;

// timelapse settings
long glob_interval = 0;
long glob_delay = 0;
long glob_frames = 0;

static struct Event    event;
static pthread_mutex_t mutex; 
static pthread_cond_t  cond;
 
//-----------------------------------------------------------------------------
// changes the program state:  
void change_state(int state) 
{
    lcd_clear();
    prog_state = state;

    struct Event ev;
    ev.type = EV_NONE;

    switch (state) 
    {
    case S_MENU: 
        lcd_puts("Timelapse!");
        user_menu(ev);
        break;

    case S_INTERVAL:
        lcd_puts("Interval  ");    
        user_timer(&glob_interval, ev);
        break;    

    case S_DELAY: 
        lcd_puts("Delay     ");    
        user_timer(&glob_delay, ev);
        break;

    case S_FRAMES: 
        lcd_puts("Frames    ");    
        user_number(&glob_frames, ev);
        break;

    case S_RUNNING: 

        if (glob_interval == 0)
        {
            change_state(S_INTERVAL);
            break;
        }

        if (timelapse_start() < 0) 
        {
            lcd_clear();           
            lcd_puts("CAMERA ERROR!"); 
            prog_state = S_MENU;
            user_menu(ev);     
        }

        break;
    } 
}


//-----------------------------------------------------------------------------
// centralized event handler:
void process_events()
{
    while (1)
    {
        // protect glob_encoder_event
        pthread_mutex_lock(&mutex);

        // if there isn't event to process then wait
        while (event.type == EV_NONE)
            pthread_cond_wait(&cond, &mutex);

        lcd_fadeout();

        switch (prog_state) 
        {
        case S_MENU:
            user_menu(event);
            break; 

        case S_INTERVAL:
            user_timer(&glob_interval, event);
            break;

        case S_DELAY:
            user_timer(&glob_delay, event);
            break;
        
        case S_FRAMES:
            user_number(&glob_frames, event);
            break;
        
        case S_RUNNING:
            if (event.type == EV_BUTTON) 
            {   
                lcd_clear();
                lcd_puts("Stopping");
                timelapse_stop();
                change_state(S_MENU);
            }

            break;
        }

        event.type = EV_NONE;

        // release event flag
        pthread_mutex_unlock(&mutex); 
    }
}

//-----------------------------------------------------------------------------
void generate_event(struct Event ev) 
{
    // if event handler is processing then return
    int ret;
    
    ret = pthread_mutex_trylock(&mutex);
    if (ret == 0)
    {
        event = ev;
        pthread_cond_signal(&cond);   // wake up event handler
        pthread_mutex_unlock(&mutex); // release ev_type
    }
    else if (ret != EBUSY) 
    {
        printf("pthread_mutex_trylock() failed, retval=%d\n", ret); 
    }
}

//-----------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    if (gpioInitialise()<0) return 1;

    // initialize mutex and condition variable object
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);

    encoder_init();
    lcd_init();
    timelapse_init();

    change_state(S_MENU);

    process_events();
     
    // clean up and exit
    gpioTerminate();
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);

    return 0; 
}


