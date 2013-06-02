CC = gcc
CFLAGS = -Wall -std=gnu99

SOURCES = err error_codes common ports macs_map frame_queue frame vlan_list
OBJECTS = $(addsuffix .o, $(SOURCES))

all: slicz slijent

$(OBJECTS): %.o : %.c %.h
	$(CC) $(CFLAGS) -c $<
	
slicz: slicz.c $(OBJECTS)
	$(CC) $(CFLAGS) slicz.c $(OBJECTS) -o slicz -levent

slijent: slijent.c err.o
	$(CC) $(CFLAGS) slijent.c err.o -o slijent -levent
	
clean:
	rm -f slicz slijent *.o
