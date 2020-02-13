CC = gcc

CFLAGS = -Wall -O3 -std=gnu11 -fPIC -Wall -c -g
CFLAGS += `pkg-config --cflags gstreamer-1.0 gstreamer-video-1.0 gstreamer-gl-1.0 gl`

LDFLAGS = -shared -Wall
LDFLAGS += `pkg-config --libs gstreamer-1.0 gstreamer-video-1.0 gstreamer-gl-1.0 glib-2.0 gl`

PY=python3

all: error.o gpuanalysis.o
	@$(CC) $(LDFLAGS) error.o gpuanalysis.o -o ../../build/libgpuanalysis.so

error.o:
	@$(CC) $(CFLAGS) error.c -o error.o

gpuanalysis.o: analysis.h
	@$(CC) $(CFLAGS) gstgpuanalysis.c -o gpuanalysis.o

clean:
	rm *.o
