CC = bfin-uclinux-gcc
LD = bfin-uclinux-ld

# we have to clobber a bunch of flags as they are normally geared towards
# running under linux, not a bare metal environment like core b
CFLAGS  ?=
CFLAGS  += -I$(PWD)/../include
PWD:= $(shell pwd)
LIB:=$(PWD)/../.libs/libmcapi.a

all: endpoints1 msg1

endpoints1: endpoints1.o $(LIB)
	$(CC) $(LDFLAGS) $< $(LIB) -o $@ > $@.map

msg1: msg1.o $(LIB)
	$(CC) $(LDFLAGS) $< $(LIB) -o $@ > $@.map

pkt1: pkt1.o $(LIB)
	$(CC) $(LDFLAGS) $< $(LIB) -o $@ > $@.map
clean:
	rm -rf *.o *.map *.gdb 

.PHONY: all clean

