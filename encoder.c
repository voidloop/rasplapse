#include <stdio.h>
#include <pigpio.h>

#include "event.h"

#define ENC_A    5
#define ENC_B    6
#define ENC_BTN 13

static void pulse(int gpio, int level, uint32_t tick);
static void button(int gpio, int level, uint32_t tick); 

//-----------------------------------------------------------------------------

static void pulse(int gpio, int level, uint32_t tick)
{
    const int8_t directions[] = { 
        0,  1, -1,  0,
        0,  0,  0,  0,
        0,  0,  0,  0,
        0, -1,  1,  0
    };

    static int8_t lev_a = 0, lev_b = 0, last_gpio = -1;
    static int8_t dir = 0;
    static uint8_t curr_state=0, prev_state=0;     

    if (gpio == ENC_A) lev_a = level; else lev_b = level;

    if (gpio == last_gpio) // debounce
        return;
    
    last_gpio = gpio;

    curr_state = lev_a << 1 | lev_b;
    dir = directions[ curr_state << 2 | prev_state ];
    prev_state = curr_state;
 
    if (!dir) return;
    
    struct Event ev;
    ev.type = EV_PULSE;
    ev.value = dir;

    generate_event(ev);
}

//-----------------------------------------------------------------------------

static void button(int gpio, int level, uint32_t tick) 
{
    static uint32_t last_tick = 0;
    
    if ((tick - last_tick) < 200000) // debounce (200 ms) 
        return;

    last_tick = tick;

    // skip button release
    if (level != 0) return;

    struct Event ev;
    ev.type = EV_BUTTON;

    generate_event(ev);
}

//-----------------------------------------------------------------------------

void encoder_init() 
{
    gpioSetMode(ENC_A, PI_INPUT);
    gpioSetMode(ENC_B, PI_INPUT);
    gpioSetMode(ENC_BTN, PI_INPUT);

    // pull up is needed as encoder common is grounded
    gpioSetPullUpDown(ENC_A, PI_PUD_UP);
    gpioSetPullUpDown(ENC_B, PI_PUD_UP);
    gpioSetPullUpDown(ENC_BTN, PI_PUD_UP);

    // monitor encoder level changes 
    gpioSetAlertFunc(ENC_A, pulse);
    gpioSetAlertFunc(ENC_B, pulse);
    gpioSetAlertFunc(ENC_BTN, button);
}
