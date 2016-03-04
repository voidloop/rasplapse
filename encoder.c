#include <stdio.h>
#include <pigpio.h>

#include "event.h"
#include "encoder.h"

#define ENCODER_A    5
#define ENCODER_B    6
#define ENC_BUTTON  13

//-----------------------------------------------------------------------------
void encoder_init() 
{
    gpioSetMode(ENCODER_A, PI_INPUT);
    gpioSetMode(ENCODER_B, PI_INPUT);
    gpioSetMode(ENC_BUTTON, PI_INPUT);

    /* pull up is needed as encoder common is grounded */
    gpioSetPullUpDown(ENCODER_A, PI_PUD_UP);
    gpioSetPullUpDown(ENCODER_B, PI_PUD_UP);
    gpioSetPullUpDown(ENC_BUTTON, PI_PUD_UP);

    /* monitor encoder level changes */
    gpioSetAlertFunc(ENCODER_A, encoder_pulse);
    gpioSetAlertFunc(ENCODER_B, encoder_pulse);
    gpioSetAlertFunc(ENC_BUTTON, encoder_button);
}

//-----------------------------------------------------------------------------
void encoder_pulse(int gpio, int level, uint32_t tick)
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

    if (gpio == ENCODER_A) lev_a = level; else lev_b = level;

    if (gpio == last_gpio) /* debounce */
        return;
    
    last_gpio = gpio;

    curr_state = lev_a << 1 | lev_b;
    dir = directions[ curr_state << 2 | prev_state ];
    prev_state = curr_state;
 
    if (!dir) return;
    
    wakeup_event_handler(EV_PULSE, dir);
}


//-----------------------------------------------------------------------------
void encoder_button(int gpio, int level, uint32_t tick) 
{
    static uint32_t last_tick = 0;
    
    if ((tick - last_tick) < 200000) /* debounce (200 ms) */
        return;

    last_tick = tick;

    // dont process button release
    if (level != 0) return;

    wakeup_event_handler(EV_BUTTON, 0);
}
