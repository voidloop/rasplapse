CC=gcc
CFLAGS=-c -Wall 
LIBS=-lgphoto2 -lpthread -lrt -lpigpio

all: timelapse

timelapse: event.o lcd.o camera.o encoder.o ui.o
	$(CC) $(LIBS) event.o camera.o encoder.o lcd.o ui.o -o timelapse 

camera.o: camera.c
	$(CC) $(CFLAGS) camera.c

event.o: event.c
	$(CC) $(CFLAGS) event.c

encoder.o: encoder.c
	$(CC) $(CFLAGS) encoder.c

lcd.o: lcd.c
	$(CC) $(CFLAGS) lcd.c

ui.o: ui.c
	$(CC) $(CFLAGS) ui.c

clean:
	rm -f *.o timelapse
