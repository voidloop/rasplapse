#ifndef __UI_H__
#define __UI_H__

#include "event.h"

void encoder_init(void);
void user_menu(struct Event ev);
void user_number(long *target, struct Event ev);
void user_timer(long *target, struct Event ev);

#endif
