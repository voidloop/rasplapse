#ifndef __EVENT_H__
#define __EVENT_H__

#define EV_NONE    0  // there is no event to process 
#define EV_PULSE   1  // encoder knob was rotated 
#define EV_BUTTON  2  // encoder button was pressed 

#define S_MENU     0  // display menu
#define S_INTERVAL 1  // set up interval
#define S_RUNNING  2  // capture images 
#define S_DELAY    3  // set up delay before capturing state
#define S_FRAMES   4  // set up frames value

struct Event 
{
    int type;
    int value;
};

void change_state(int state);
void generate_event(struct Event ev); 

#endif
