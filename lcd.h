#ifndef __LCD_H__
#define __LCD_H__

#define LCD_COLS 16
#define LCD_ROWS  2

void lcd_init(void);
void lcd_destroy(void);

void lcd_clear(void);
int  lcd_putc(const char c);
int  lcd_puts(const char *s);
void lcd_set_cursor(const int row, const int col); 

void lcd_fadeout();

#endif
