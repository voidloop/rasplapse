#ifndef __ENCODER_H__
#define __ENCODER_H__

void encoder_init(void);
void encoder_pulse(int gpio, int lev, uint32_t tick);
void encoder_button(int gpio, int lev, uint32_t tick);

#endif
