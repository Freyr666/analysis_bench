CC = gcc

CFLAGS = -Wall -O3 -march=native -fPIC -Wall -c 
CFLAGS += `pkg-config --cflags gstreamer-1.0 gstreamer-video-1.0`

LDFLAGS = -shared -Wall
LDFLAGS += `pkg-config --libs gstreamer-1.0 gstreamer-video-1.0 glib-2.0`

PY=python3

all: error.o videodata.o cpuanalysis.o
	@$(CC) $(LDFLAGS) videodata.o error.o cpuanalysis.o -o ../../build/libcpuanalysis.so

error.o:
	@$(CC) $(CFLAGS) error.c -o error.o

videodata.o: analysis.h
	@$(CC) $(CFLAGS) videodata.c -o videodata.o

cpuanalysis.o: analysis.h
	@$(CC) $(CFLAGS) gstcpuanalysis.c -o cpuanalysis.o

analysis.h:
	@$(PY) generate_array.py analysis.h.template analysis.h

clean:
	rm *.o analysis.h
