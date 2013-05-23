CC = bfin-uclinux-gcc
LD = bfin-uclinux-ld

test_PROGRAM = endpoints1 msg1 pkt1 scl1

# we have to clobber a bunch of flags as they are normally geared towards
# running under linux, not a bare metal environment like core b
CFLAGS  ?=
CFLAGS  += -I$(PWD)/../include
PWD:= $(shell pwd)
LIB:=$(PWD)/../../../../staging/usr/lib/libmcapi.a

all: $(test_PROGRAM)

endpoints1: endpoints1.o $(LIB)
	$(CC) $(LDFLAGS) $< $(LIB) -o $@ > $@.map

msg1: msg1.o $(LIB)
	$(CC) $(LDFLAGS) $< $(LIB) -o $@ > $@.map

pkt1: pkt1.o $(LIB)
	$(CC) $(LDFLAGS) $< $(LIB) -o $@ > $@.map

scl1: scl1.o $(LIB)
	$(CC) $(LDFLAGS) $< $(LIB) -o $@ > $@.map

clean:
	rm -rf *.o *.map *.gdb $(test_PROGRAM)

.PHONY: all clean

