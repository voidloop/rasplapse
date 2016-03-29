#include <stdio.h>

#include "lcd.h"
#include "ui.h"

//-----------------------------------------------------------------------------

void user_menu(struct Event ev) 
{
    const static int states[] = { 
        S_INTERVAL, S_DELAY, S_FRAMES, S_RUNNING };

    const static char *choises[] = { 
        "Interval", "Delay   ", "Frames  ", "Start   " };

    const static int n_choises = sizeof(states)/sizeof(int);
    static int index = 0;

    char buf[LCD_COLS+1];

    switch (ev.type) 
    {
    case EV_PULSE:
        index += ev.value;

        if (index < 0) 
            index = 0;
        else if (index >= n_choises) 
            index = n_choises - 1;

        break; 

    case EV_BUTTON:
        change_state(states[index]);
        return;
    }
    
    // print menu choise
    snprintf(buf, LCD_COLS, "%d. %s", index+1, choises[index]);
    lcd_set_cursor(1, 0); 
    lcd_puts(buf);
}

//-----------------------------------------------------------------------------

void user_number(long *target, struct Event ev)
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

void user_timer(long *target, struct Event ev)
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

