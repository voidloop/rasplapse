#ifndef __EVENT_H__
#define __EVENT_H__

#define EV_NONE   0  // there is no event to process 
#define EV_PULSE  1  // encoder knob was rotated 
#define EV_BUTTON 2  // encoder button was pressed 

void wakeup_event_handler(int evtype, int evvalue); 

#endif
