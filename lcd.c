#include <pigpio.h>
#include "lcd.h"

#define	LCD_RS 25
#define	LCD_EN 24
#define	LCD_D4 23
#define	LCD_D5 22
#define	LCD_D6 27
#define	LCD_D7 17

//-----------------------------------------------------------------------------
static void lcd_write_byte(const unsigned char b)
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

//-----------------------------------------------------------------------------
int lcd_putc(const char c)
{
	gpioWrite(LCD_RS, 1);
	lcd_write_byte(c);
	gpioWrite(LCD_RS, 0);
    return (int)c;
}

//-----------------------------------------------------------------------------
void lcd_set_cursor(const unsigned char row, 
                    const unsigned char column)
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

	lcd_write_byte(command);
}

//-----------------------------------------------------------------------------
int lcd_puts(const char *s)
{
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

//-----------------------------------------------------------------------------
void lcd_clear() 
{
    gpioWrite(LCD_RS, 0);
	lcd_write_byte(0x01);
}


//-----------------------------------------------------------------------------
void lcd_init()
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
}





