CC=gcc
CFLAGS=-g -O2 -Wall -fPIC
CPATH=
LIBPATH=
LIBS=

all: main-0-template main-1-uts main-3-pid main-4-ns main-5-net main-6-user

main-0-template:
	$(CC) -o main-0-template.o -c $(CFLAGS) $(CPATH) main-0-template.c
	$(CC) -o main-0-template main-0-template.o $(LIBPATH) $(LIBS)

main-1-uts:
	$(CC) -o main-1-uts.o -c $(CFLAGS) $(CPATH) main-1-uts.c
	$(CC) -o main-1-uts main-1-uts.o $(LIBPATH) $(LIBS)

main-3-pid:
	$(CC) -o main-3-pid.o -c $(CFLAGS) $(CPATH) main-3-pid.c
	$(CC) -o main-3-pid main-3-pid.o $(LIBPATH) $(LIBS)

main-4-ns:
	$(CC) -o main-4-ns.o -c $(CFLAGS) $(CPATH) main-4-ns.c
	$(CC) -o main-4-ns main-4-ns.o $(LIBPATH) $(LIBS)

main-5-net:
	$(CC) -o main-5-net.o -c $(CFLAGS) $(CPATH) main-5-net.c
	$(CC) -o main-5-net main-5-net.o $(LIBPATH) $(LIBS)

main-6-user:
	$(CC) -o main-6-user.o -c $(CFLAGS) $(CPATH) main-6-user.c
	$(CC) -o main-6-user main-6-user.o $(LIBPATH) $(LIBS)

clean: 
	rm -rf *.o \
		main-0-template \
		main-1-uts \
		main-3-pid \
		main-4-ns \
		main-5-net \
		main-6-user
