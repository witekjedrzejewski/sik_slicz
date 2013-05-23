CC = gcc
CFLAGS = -Wall -std=gnu99

all: slicz

err.o: err.c
	$(CC) $(CFLAGS) -c err.c -o err.o

error_codes.o: error_codes.c
	$(CC) $(CFLAGS) -c error_codes.c -o error_codes.o
	
utils.o: utils.c
	$(CC) $(CFLAGS) -c utils.c -o utils.o

ports.o: ports.c
	$(CC) $(CFLAGS) -c ports.c -o ports.o

macs_map.o: macs_map.c
	$(CC) $(CFLAGS) -c macs_map.c -o macs_map.o

slicz: slicz.c err.o utils.o ports.o error_codes.o macs_map.o
	$(CC) $(CFLAGS) slicz.c err.o utils.o ports.o error_codes.o macs_map.o -o slicz -levent

clean:
	rm -f slicz *.o
