#include <stdio.h> 
#include "timelapse.h"

#define TOTAL_FRAMES 5

int glo_frames = TOTAL_FRAMES; 
int glo_interval = 10; //seconds

//-----------------------------------------------------------------------------
int main()    
{
    create_context();
    camera_init();
    camera_loop();
    camera_exit();
    return 0;
}
