CC=gcc
CFLAGS=-c -Wall
LIBS=-lgphoto2

all: timelapse

timelapse: main.o context.o camera.o
	$(CC) $(LIBS) main.o context.o camera.o -o timelapse

main.o: main.c
	$(CC) $(CFLAGS) main.c 

context.o: context.c
	$(CC) $(CFLAGS) context.c

camera.o: camera.c
	$(CC) $(CFLAGS) camera.c

clean:
	rm *.o timelapse
