#include <stdio.h>
#include <pigpio.h>
#include <pthread.h>
#include <errno.h>

#include "encoder.h"
#include "event.h"

// event variables
static int glo_evtype = EV_NONE; 
static int glo_evvalue = 0;
static pthread_mutex_t glo_evmutex; 
static pthread_cond_t  glo_evcond;

//-----------------------------------------------------------------------------
void *event_handler(void *arg)
{
    while (1)
    {
        // protect glo_encoder_event
        pthread_mutex_lock(&glo_evmutex);

        // if there isn't event to process then wait
        while (glo_evtype == EV_NONE)
            pthread_cond_wait(&glo_evcond, &glo_evmutex);

        if (glo_evtype == EV_PULSE) 
        {
            printf("direction: %d\n", glo_evvalue);
        }
        else if (glo_evtype == EV_BUTTON) 
        {
            printf("encoder pressed\n");
        }

        glo_evtype = EV_NONE;

        // release event flag
        pthread_mutex_unlock(&glo_evmutex); 
    }

    // exit
    pthread_exit(NULL);
}

//-----------------------------------------------------------------------------
void wakeup_event_handler(int evtype, int evvalue) 
{
    // protect glo_evtype, if event handler is processing then return
    int retval = pthread_mutex_trylock(&glo_evmutex);

    if (retval == 0)
    {
        glo_evtype = evtype;
        glo_evvalue = evvalue;
        pthread_cond_signal(&glo_evcond);   // wake up event handler
        pthread_mutex_unlock(&glo_evmutex); // release glo_evtype
    }
    else if (retval != EBUSY) 
    {
        printf("pthread_mutex_trylock() failed, retval=%d\n", retval); 
    }
}

//-----------------------------------------------------------------------------
int main(int argc, char * argv[])
{
    if (gpioInitialise()<0) return 1;

    pthread_t event_thread;
    pthread_attr_t attr; 

    // initialize mutex and condition variable object
    pthread_mutex_init(&glo_evmutex, NULL);
    pthread_cond_init(&glo_evcond, NULL);

    // create threads and join
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_create(&event_thread, &attr, event_handler, NULL);


    encoder_init();

    // wait all thread to complete
    pthread_join(event_thread, NULL);

    // clean up and exit
    gpioTerminate();
    pthread_attr_destroy(&attr);
    pthread_mutex_destroy(&glo_evmutex);
    pthread_cond_destroy(&glo_evcond);
    pthread_exit(NULL);
}


