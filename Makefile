CC = gcc
CFLAGS = -Wall -std=gnu99

ALL = slicz
SOURCES = err error_codes utils ports macs_map frame_queue
OBJECTS = $(addsuffix .o, $(SOURCES))

all: slicz

$(OBJECTS): %.o : %.c %.h
	$(CC) $(CFLAGS) -c $<
	
slicz: slicz.c $(OBJECTS)
	$(CC) $(CFLAGS) slicz.c $(OBJECTS) -o slicz -levent

clean:
	rm -f slicz *.o
