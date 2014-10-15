CC=gcc
CFLAGS=-g -O2 -Wall -fPIC
CPPPATH=
LIBPATH=
LIBS=

all: main-0-template main-1-uts main-3-pid

main-0-template:
	$(CC) -o main-0-template.o -c $(CFLAGS) $(CPPPATH) main-0-template.c
	$(CC) -o main-0-template main-0-template.o $(LIBPATH) $(LIBS)

main-1-uts:
	$(CC) -o main-1-uts.o -c $(CFLAGS) $(CPPPATH) main-1-uts.c
	$(CC) -o main-1-uts main-1-uts.o $(LIBPATH) $(LIBS)

main-3-pid:
	$(CC) -o main-3-pid.o -c $(CFLAGS) $(CPPPATH) main-3-pid.c
	$(CC) -o main-3-pid main-3-pid.o $(LIBPATH) $(LIBS)

clean: 
	rm -rf *.o main-0-template main-1-uts main-3-pid
