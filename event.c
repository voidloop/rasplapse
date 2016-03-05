#include <stdio.h>
#include <pigpio.h>
#include <pthread.h>
#include <errno.h>

#include "encoder.h"
#include "event.h"
#include "lcd.h"
#include "camera.h"

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
struct MenuEntry 
{
    char text[17]; // entry text
    int  next;     // next state 
};

const static struct MenuEntry main_menu[] = {
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
    lcd_puts(main_menu[0].text);       
}

//-----------------------------------------------------------------------------
// deals menu events: 
void menu_update(struct Event ev)
{
    const int len = sizeof(main_menu)/sizeof(struct MenuEntry);
    static int index = 0;

    switch (ev.type) 
    {
    case EV_PULSE:
        index += ev.value;
        if (index < 0) index = 0;
        else if (index >= len) index = len - 1;           
        break; 

    case EV_BUTTON:
        change_state(main_menu[index].next);
        return;
    }


    lcd_set_cursor(1, 0);
    lcd_puts(main_menu[index].text);
}


//-----------------------------------------------------------------------------
// an user interface to change target value:
void menu_number(long *target, struct Event ev)
{
    // two items: num(0), ok(1) 
    const int items = 2;
    static int selected = 1;
    
    static char buf[64];

    // if edit == 1 then number is in edit mode
    static int edit = 0;

    const int maxval = 99999;
    long nextval;   
    int dir = ev.value;

    switch (ev.type) 
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
void menu_timer(long *target, struct Event ev)
{
    static char buf[64];
    
    // four items: h(0), m(1), s(2), ok(3) 
    const int items = 4;
    static int selected = 3; 
    static int edit = 0; 
    static int mul = 1; 

    const int maxval = 359999; // 99*3600 + 59*60 + 59
    long nextval;   
    int dir = ev.value;
    

    switch (ev.type) 
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
    prog_state = state;

    struct Event ev;
    ev.type = EV_NONE;

    switch (state) 
    {
    case S_MENU: 
        lcd_puts("Timelapse!      ");
        menu_update(ev);
        break;

    case S_INTERVAL:
        lcd_puts("Interval        ");    
        menu_timer(&glob_interval, ev);
        break;    

    case S_DELAY: 
        lcd_puts("Delay           ");    
        menu_timer(&glob_delay, ev);
        break;

    case S_FRAMES: 
        lcd_puts("Frames          ");    
        menu_number(&glob_frames, ev);
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
            menu_update(ev);     
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
            menu_update(event);
            break; 

        case S_INTERVAL:
            menu_timer(&glob_interval, event);
            break;

        case S_DELAY:
            menu_timer(&glob_delay, event);
            break;
        
        case S_FRAMES:
            menu_number(&glob_frames, event);
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
    menu_init();    
    timelapse_init();

    process_events();
     
    // clean up and exit
    gpioTerminate();
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);

    return 0; 
}


