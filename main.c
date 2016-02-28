#include <stdio.h> 
#include "timelapse.h"

#define TOTAL_FRAMES 5

int glo_total_frames = TOTAL_FRAMES; 

//-----------------------------------------------------------------------------
int main()    
{
    create_context();
    camera_init();
    camera_loop();
    camera_exit();
    return 0;
}
