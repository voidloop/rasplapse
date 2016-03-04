#ifndef __LCD_DRV_H__
#define __LCD_DRV_H__

void lcd_init(void);
void lcd_clear(void);
int lcd_putc(const char c);
int lcd_puts(const char *s);
void lcd_set_cursor(const unsigned char row, const unsigned char column); 

#endif
