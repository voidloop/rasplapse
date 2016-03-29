#include <pigpio.h>
#include <pthread.h>
#include <sys/time.h>
#include <stdio.h>
#include <errno.h>

#include "lcd.h"

#define	LCD_RS 25
#define	LCD_EN 24
#define	LCD_D4 23
#define	LCD_D5 22
#define	LCD_D6 27
#define	LCD_D7 17
#define LCD_BL 12 

// sec to wait before enter in pwm loop
#define WAIT_MAX 60 

// usec to wait before decrement pwm value
#define WAIT_NEXT 10000 

static uint8_t cursor_pos;
static volatile int thread_done = 1;

static pthread_mutex_t mutex; 
static pthread_cond_t condm; // master
static pthread_cond_t condw; // worker

// --------------------------------------------------------

static void* fadeout_thread(void *arg) 
{
    int pwm = 255;
    struct timespec ts;
    struct timeval now;

    // lock mutex
    pthread_mutex_lock(&mutex);

    // set gpio pwm to 100%
    gpioPWM(LCD_BL, pwm);        

    // convert 'now' in timespec and add 'wait_seconds'
    gettimeofday(&now, NULL);
    ts.tv_sec = now.tv_sec + WAIT_MAX; 
    ts.tv_nsec = now.tv_usec * 1000;

    // wait 'wait_seconds' or master signal
    pthread_cond_timedwait(&condw, &mutex, &ts);

    while (!thread_done && pwm > 0) 
    {   
        gettimeofday(&now, NULL);
        
        // convert timeval to timespec
        ts.tv_sec = now.tv_sec;
        ts.tv_nsec = (now.tv_usec + WAIT_NEXT) * 1000;

        // set gpio pwm
        gpioPWM(LCD_BL, --pwm);        

        // wait for 10 millis
        pthread_cond_timedwait(&condw, &mutex, &ts);
    }
    
    thread_done = 1;    

    // notify master thread that my work is finished
    pthread_cond_signal(&condm);
    
    // unlock mutex
    pthread_mutex_unlock(&mutex);

    return NULL;
}


// --------------------------------------------------------

static void fadeout_start( void ) 
{
    pthread_t thread;
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    thread_done = 0;
    pthread_create(&thread, &attr, fadeout_thread, NULL); 
    pthread_attr_destroy(&attr);
}

// --------------------------------------------------------

static void fadeout_stop( void ) 
{
    // lock mutex
    pthread_mutex_lock(&mutex);

    // if already exists a worker then stop it
    if (thread_done==0) 
    {
        // stop it 
        thread_done = 1;
        pthread_cond_signal(&condw);

        // wait its signal
        pthread_cond_wait(&condm, &mutex);
    }

    // unlock mutex 
    pthread_mutex_unlock(&mutex);
}


// --------------------------------------------------------

void lcd_fadeout( void ) 
{
    fadeout_stop();
    fadeout_start(); 
}

// ---------------------------------------------------------

static void lcd_write_byte( const uint8_t b )
{
    int rs = gpioRead(LCD_RS);
    
    // 1st nibble
    gpioWrite(LCD_D7, (b >> 7) & 1);    
    gpioWrite(LCD_D6, (b >> 6) & 1);
    gpioWrite(LCD_D5, (b >> 5) & 1);
    gpioWrite(LCD_D4, (b >> 4) & 1);

    // enable
    gpioWrite(LCD_EN, 1);
    gpioDelay(1);
    gpioWrite(LCD_EN, 0);
    (rs == 1) ? gpioDelay(200) : gpioDelay(5500); 

    // 2nd nibble
    gpioWrite(LCD_D7, (b >> 3) & 1);    
    gpioWrite(LCD_D6, (b >> 2) & 1);
    gpioWrite(LCD_D5, (b >> 1) & 1);
    gpioWrite(LCD_D4, (b >> 0) & 1);

    // enable
    gpioWrite(LCD_EN, 1);
    gpioDelay(1);
    gpioWrite(LCD_EN, 0);
    (rs == 1) ? gpioDelay(200) : gpioDelay(5500); 
}

// ---------------------------------------------------------

int lcd_putc( const char c )
{
    // remember last position command
    lcd_write_byte(cursor_pos);

	gpioWrite(LCD_RS, 1);
	lcd_write_byte(c);
	gpioWrite(LCD_RS, 0);
    return (int)c;
}

// --------------------------------------------------------

void lcd_set_cursor( const int row, const int column )
{
    uint8_t command = 0x80;
	gpioWrite(LCD_RS, 0);
	switch(row) {
		case 0:
			command += column;
			break;
		case 1:
			command += 0x40 + (column);
			break;
	}

    cursor_pos = command;
	lcd_write_byte(command);
}

// --------------------------------------------------------

int lcd_puts( const char *s )
{
    // remember last position command
    gpioWrite(LCD_RS, 0);
    lcd_write_byte(cursor_pos);

    if(s != NULL) {
		gpioWrite(LCD_RS, 1);
		while(*s != '\0') {
			lcd_write_byte(*(s++));
		}
		gpioWrite(LCD_RS, 0);
		return 1;
	}
	else {
		return 0;
	}
}

// --------------------------------------------------------

void lcd_clear( void ) 
{
    gpioWrite(LCD_RS, 0);
	lcd_write_byte(0x01);
    
    // first row and first column
    cursor_pos = 0x80;
}

//---------------------------------------------------------

void lcd_init( void )
{
    // set pin mode
    gpioSetMode(LCD_RS, PI_OUTPUT);
    gpioSetMode(LCD_EN, PI_OUTPUT);
    gpioSetMode(LCD_D7, PI_OUTPUT);
    gpioSetMode(LCD_D6, PI_OUTPUT);
    gpioSetMode(LCD_D5, PI_OUTPUT);
    gpioSetMode(LCD_D4, PI_OUTPUT);

    // -- power on -- 
    // wait for more than 15 ms
    // after Vcc rises to 4.5V
    gpioDelay(15000);

    gpioWrite(LCD_RS, 0);
    gpioWrite(LCD_EN, 0);

    // -- function set --
    // RS RW D7 D6 D5 D4
    // 0  0  0  0  1  1
    gpioWrite(LCD_D7, 0);
    gpioWrite(LCD_D6, 0);
    gpioWrite(LCD_D5, 1);
    gpioWrite(LCD_D4, 1);

    gpioWrite(LCD_EN, 1);
    gpioDelay(1);
    gpioWrite(LCD_EN, 0);

    // wait for more than 4.1 ms 
    gpioDelay(5000);

    // -- function set --
    gpioWrite(LCD_EN, 1);
    gpioDelay(1);
    gpioWrite(LCD_EN, 0);

    // wait for more than 100 us
    gpioDelay(200);

    // -- function set --
    gpioWrite(LCD_EN, 1);
    gpioDelay(1);
    gpioWrite(LCD_EN, 0);
    gpioDelay(200);

    // -- 4-bit mode --
    // RS RW D7 D6 D5 D4
    // 0  0  0  0  1  0
    gpioWrite(LCD_D7, 0);
    gpioWrite(LCD_D6, 0);
    gpioWrite(LCD_D5, 1);
    gpioWrite(LCD_D4, 0);

	gpioWrite(LCD_EN, 1);
	gpioDelay(1);
	gpioWrite(LCD_EN, 0);
    gpioDelay(5000);
       
    // -- interface length --
    // RS RW D7 D6 D5 D4
    // 0  0  0  0  1  0
    // 0  0  N  F  -  -
    lcd_write_byte(0x28);

    // -- display off --
    // RS RW D7 D6 D5 D4
    // 0  0  0  0  0  0
    // 0  0  1  0  0  0
    lcd_write_byte(0x08);
 
 
    // -- clear screen --
    // RS RW D7 D6 D5 D4
    // 0  0  0  0  0  0
    // 0  0  0  0  0  1
    lcd_write_byte(0x01);

    // -- set entry mode --
    // RS RW D7 D6 D5 D4
    // 0  0  0  0  0  0
    // 0  0  0  1  D  S
    lcd_write_byte(0x06);

    // -- set entry mode --
    // RS RW D7 D6 D5 D4
    // 0  0  0  0  0  0
    // 0  0  1  D  C  B
    lcd_write_byte(0x0C);

    // first row and first column
    cursor_pos = 0x80;

    // initialize mutex and condition variables object
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&condw, NULL);
    pthread_cond_init(&condm, NULL);

    lcd_fadeout();
}

// --------------------------------------------------------

void lcd_destroy( void )
{
    fadeout_stop();
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&condw);
    pthread_cond_destroy(&condm);
}

