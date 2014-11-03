CC=gcc
CFLAGS=-g -O2 -Wall -fPIC
CPATH=
LIBPATH=
LIBS=-lseccomp

all: 01-nothing 02-bpf-only-dup-sudo 03-bpf-only-dup

01-nothing:
	$(CC) -o 01-nothing.o -c $(CFLAGS) $(CPATH) 01-nothing.c
	$(CC) -o 01-nothing 01-nothing.o $(LIBPATH) $(LIBS)

02-bpf-only-dup-sudo:
	$(CC) -o 02-bpf-only-dup-sudo.o -c $(CFLAGS) $(CPATH) 02-bpf-only-dup-sudo.c
	$(CC) -o 02-bpf-only-dup-sudo 02-bpf-only-dup-sudo.o $(LIBPATH) $(LIBS)

03-bpf-only-dup:
	$(CC) -o 03-bpf-only-dup.o -c $(CFLAGS) $(CPATH) 03-bpf-only-dup.c 
	$(CC) -o 03-bpf-only-dup 03-bpf-only-dup.o $(LIBPATH) $(LIBS)

clean: 
	rm -rf *.o 01-nothing 02-bpf-only-dup-sudo 03-bpf-only-dup
