CC=g++
CFLAGS=-g -O2 -Wall -fPIC
CPPPATH=-I/usr/include/vtk-5.10
LIBPATH=-L/usr/lib/vtk-5.10
LIBS=-lX11 -lXtst -lmtdev -lvtkCharts -lvtkFiltering -lvtkCommon

all: testgr read_mtdev vtkplot

testgr:
	$(CC) -o testgr.o -c $(CFLAGS) $(CPPPATH) testgr.cpp
	$(CC) -o gesture_recognition.o -c $(CFLAGS) $(CPPPATH) gesture_recognition.cpp
	$(CC) -o testgr testgr.o gesture_recognition.o $(LIBPATH) $(LIBS)

read_mtdev:
	$(CC) -o read_mtdev.o -c $(CFLAGS) $(CPPPATH) read_mtdev.cpp
	$(CC) -o gesture_recognition.o -c $(CFLAGS) $(CPPPATH) gesture_recognition.cpp
	$(CC) -o read_mtdev read_mtdev.o gesture_recognition.o $(LIBPATH) $(LIBS)

vtkplot:
	$(CC) -o vtkplot.o -c $(CFLAGS) $(CPPPATH) vtkplot.cpp
	$(CC) -o vtkplot vtkplot.o $(LIBPATH) $(LIBS)

clean: 
	rm -rf *.o testgr read_mtdev vtkplot
