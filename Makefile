CC=gcc
CFLAGS=-c -Wall
LIBS=-lgphoto2

all: timelapse

timelapse: event.o lcd.c context.o camera.o
	$(CC) $(LIBS) main.o context.o camera.o -o timelapse

context.o: context.c
	$(CC) $(CFLAGS) context.c

camera.o: camera.c
	$(CC) $(CFLAGS) camera.c

event.o: event.c
	$(CC) $(CFLAGS) event.c

encoder.o: encoder.c
	$(CC) $(CFLAGS) encoder.c

lcd.o: lcd.c
	$(CC) $(CFLAGS) lcd.c

clean:
	rm *.o timelapse
