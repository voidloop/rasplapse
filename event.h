#ifndef __EVENT_H__
#define __EVENT_H__

#define EV_NONE   0  // there is no event to process 
#define EV_PULSE  1  // encoder knob was rotated 
#define EV_BUTTON 2  // encoder button was pressed 

#define S_MENU     0
#define S_INTERVAL 1
#define S_RUNNING  2
#define S_DELAY    3
#define S_FRAMES   4

void change_state(int state);
void wakeup_event_handler(int evtype, int evvalue); 

#endif
