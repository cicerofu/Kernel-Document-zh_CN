CC=gcc
CFLAGS=-g -O2 -Wall -fPIC
CPATH=
LIBPATH=
LIBS=

all: 01-nothing

01-nothing:
	$(CC) -o 01-nothing.o -c $(CFLAGS) $(CPATH) 01-nothing.c
	$(CC) -o 01-nothing 01-nothing.o $(LIBPATH) $(LIBS)

clean: 
	rm -rf *.o \
		01-nothing
