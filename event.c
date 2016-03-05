#include <stdio.h>
#include <pigpio.h>
#include <pthread.h>
#include <errno.h>

#include "encoder.h"
#include "event.h"
#include "lcd.h"
#include "camera.h"

// program state
static int glo_progstate = S_MENU;

// timelapse settings
long glo_interval = 0;
long glo_delay = 0;
long glo_frames = 0;

// event variables
static int glo_evtype = EV_NONE; 
static int glo_evvalue = 0;
static pthread_mutex_t glo_evmutex; 
static pthread_cond_t  glo_evcond;

//-----------------------------------------------------------------------------
struct MenuEntry {
    char text[17];   /* menu entry text */   
    int nextstate; /* next state */
};

const static struct MenuEntry glo_mainmenu[] = {
    { "1. Interval     ", S_INTERVAL },
    { "2. Delay        ", S_DELAY },
    { "3. Frames       ", S_FRAMES },
    { "4. Start        ", S_RUNNING }
};

//-----------------------------------------------------------------------------
void menu_init() 
{
    lcd_puts("Timelapse!      ");

    lcd_set_cursor(1, 0);
    lcd_puts(glo_mainmenu[0].text);       
}

//-----------------------------------------------------------------------------
// deals menu events: 
void menu_update(int evtype, int dir)
{
    const int len = sizeof(glo_mainmenu)/sizeof(struct MenuEntry);
    static int index = 0;

    switch (evtype) 
    {
    case EV_PULSE:
        index += dir;
        if (index < 0) index = 0;
        else if (index >= len) index = len - 1;           
        break; 

    case EV_BUTTON:
        change_state(glo_mainmenu[index].nextstate);
        return;
    }


    lcd_set_cursor(1, 0);
    lcd_puts(glo_mainmenu[index].text);
}


//-----------------------------------------------------------------------------
// an user interface to change target value:
void menu_number(long *target, int evtype, int dir)
{
    // two items: num(0), ok(1) 
    const int items = 2;
    static int selected = 1;
    
    static char buf[64];

    // if edit == 1 then number is in edit mode
    static int edit = 0;

    const int maxval = 99999;
    long nextval;   

    switch (evtype) 
    {
    case EV_PULSE:
        if (edit) 
        {
            nextval = *target + dir; 
            if (nextval >= 0 && nextval <= maxval)
                *target = nextval;
        }
        else 
        { 
            if ( (selected + dir) >= 0 && (selected + dir) < items )   
                selected += dir;
        }
        break;

    case EV_BUTTON:
        if (!edit && selected == 1)
        {
            change_state(S_MENU);
            return;
        }
        
        edit = !edit;
        break;
    }
   
    if (edit) 
    {
        sprintf(buf, ">%05ld<      OK ", *target);
    } 
    else 
    {
        if (selected == 0) 
            sprintf(buf, "[%05ld]      OK ", *target);
        else 
            sprintf(buf, " %05ld      [OK]", *target);       
    }

    lcd_set_cursor(1, 0);
    lcd_puts(buf);
}

//-----------------------------------------------------------------------------
// an user interface to change target value with a timer HH:MM'SS'':
void menu_timer(long *target, int evtype, int dir)
{
    static char buf[64];
    
    // four items: h(0), m(1), s(2), ok(3) 
    const int items = 4;
    static int selected = 3; 
    static int edit = 0; 
    static int mul = 1; 

    const int maxval = 359999; // 99*3600 + 59*60 + 59
    long nextval;   

    switch (evtype) 
    {
    case EV_PULSE:
        if (edit) 
        {
            nextval = *target + (dir*mul); 
            if (nextval >= 0 && nextval <= maxval)
                *target = nextval;
        }
        else 
        { 
            if ( (selected + dir) >= 0 && (selected + dir) < items )   
                selected += dir;
            
            if (selected == 0) mul = 3600;
            else if (selected == 1) mul = 60; 
            else mul = 1;
        }
        break;

    case EV_BUTTON:
        if (!edit && selected == 3)
        {
            change_state(S_MENU);
            return;
        }
        
        edit = !edit;
        break;
    }

    int s = *target % 60; 
    int m = (*target / 60) % 60; 
    int h = *target / 3600; 
   
    if (edit) 
    {
        switch (selected) 
        {
        case 0: sprintf(buf, ">%02d<%02d'%02d''  OK ", h, m, s); break;
        case 1: sprintf(buf, " %02d>%02d<%02d''  OK ", h, m, s); break;
        case 2: sprintf(buf, " %02d:%02d>%02d<   OK ", h, m, s); break;
        }
    } 
    else 
    {
        switch (selected) 
        {
        case 0: sprintf(buf, "[%02d]%02d'%02d''  OK ", h, m, s); break;
        case 1: sprintf(buf, " %02d[%02d]%02d''  OK ", h, m, s); break;
        case 2: sprintf(buf, " %02d:%02d[%02d]   OK ", h, m, s); break;
        case 3: sprintf(buf, " %02d:%02d'%02d'' [OK]", h, m, s); break;
        }
    }

    lcd_set_cursor(1, 0);
    lcd_puts(buf);
}

//-----------------------------------------------------------------------------
// changes the program state:  
void change_state(int state) 
{
    lcd_clear();

    glo_progstate = state;
    printf("state=%d\n", state);

    switch (state) 
    {
    case S_MENU: 
        lcd_puts("Timelapse!      ");
        menu_update(EV_NONE, 0);
        break;

    case S_INTERVAL:
        lcd_puts("Interval        ");    
        menu_timer(&glo_interval, EV_NONE, 0);
        break;    

    case S_DELAY: 
        lcd_puts("Delay           ");    
        menu_timer(&glo_delay, EV_NONE, 0);
        break;

    case S_FRAMES: 
        lcd_puts("Frames          ");    
        menu_number(&glo_frames, EV_NONE, 0);
        break;

    case S_RUNNING: 
        if (glo_interval == 0)
        {
            change_state(S_INTERVAL);
            break;
        }
        
        lcd_clear();

        if (camera_init() != 0) 
        {
            lcd_puts("NO CAMERA!"); 
            glo_progstate = S_MENU;
            menu_update(EV_NONE, 0);     
            break;
        }
        else 
        {
            camera_start_capture();
        }

    } 
}


//-----------------------------------------------------------------------------
// centralized event handler:
void *event_handler(void *arg)
{
    while (1)
    {
        // protect glo_encoder_event
        pthread_mutex_lock(&glo_evmutex);

        // if there isn't event to process then wait
        while (glo_evtype == EV_NONE)
            pthread_cond_wait(&glo_evcond, &glo_evmutex);

        switch (glo_progstate) 
        {
        case S_MENU:
            menu_update(glo_evtype, glo_evvalue);
            break; 

        case S_INTERVAL:
            menu_timer(&glo_interval, glo_evtype, glo_evvalue);
            break;

        case S_DELAY:
            menu_timer(&glo_delay, glo_evtype, glo_evvalue);
            break;
        
        case S_FRAMES:
            menu_number(&glo_frames, glo_evtype, glo_evvalue);
            break;
        
        case S_RUNNING:
            if (glo_evtype == EV_BUTTON)
                camera_stop_capture();
            break;
        }

        glo_evtype = EV_NONE;

        // release event flag
        pthread_mutex_unlock(&glo_evmutex); 
    }

    // exit
    pthread_exit(NULL);
}

//-----------------------------------------------------------------------------
void wakeup_event_handler(int evtype, int evvalue) 
{
    // protect glo_evtype, if event handler is processing then return
    int retval = pthread_mutex_trylock(&glo_evmutex);

    if (retval == 0)
    {
        glo_evtype = evtype;
        glo_evvalue = evvalue;
        pthread_cond_signal(&glo_evcond);   // wake up event handler
        pthread_mutex_unlock(&glo_evmutex); // release glo_evtype
    }
    else if (retval != EBUSY) 
    {
        printf("pthread_mutex_trylock() failed, retval=%d\n", retval); 
    }
}

//-----------------------------------------------------------------------------
int main(int argc, char * argv[])
{
    // event thread 
    pthread_t thread;
    pthread_attr_t attr;

    if (gpioInitialise()<0) return 1;

    // initialize mutex and condition variable object
    pthread_mutex_init(&glo_evmutex, NULL);
    pthread_cond_init(&glo_evcond, NULL);

    // create threads and join
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_create(&thread, &attr, event_handler, NULL);

    encoder_init();
    lcd_init();
    menu_init();    
    gphoto2_pthread_init();

    // wait all thread to complete
    pthread_join(thread, NULL);

    
    // clean up and exit
    gpioTerminate();

    pthread_attr_destroy(&attr);
    pthread_mutex_destroy(&glo_evmutex);
    pthread_cond_destroy(&glo_evcond);
    gphoto2_pthread_destroy();

    pthread_exit(NULL);
}


