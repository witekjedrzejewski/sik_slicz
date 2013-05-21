CC = gcc
CFLAGS = -Wall -std=gnu99

all: slicz

err.o: err.c
	$(CC) $(CFLAGS) -c err.c -o err.o

slicz: slicz.c err.o
	$(CC) $(CFLAGS) slicz.c err.o -o slicz -levent

clean:
	rm -f slicz *.o
