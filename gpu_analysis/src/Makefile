CC = gcc

CFLAGS = -Wall -O3 -std=gnu11 -fPIC -Wall -c -g
CFLAGS += `pkg-config --cflags gstreamer-1.0 gstreamer-video-1.0 gstreamer-gl-1.0 gl`

LDFLAGS = -shared -Wall
LDFLAGS += `pkg-config --libs gstreamer-1.0 gstreamer-video-1.0 gstreamer-gl-1.0 glib-2.0 gl`

PY=python3

all: gstatsanalysis.o gpuanalysis.o
	@$(CC) $(LDFLAGS) gstatsanalysis.o gpuanalysis.o -o ../../build/libgpuanalysis.so

gstatsanalysis.o:
	@$(CC) $(CFLAGS) gstatsanalysis.c -o gstatsanalysis.o

gpuanalysis.o: common.h
	@$(CC) $(CFLAGS) gstvideoanalysis.c -o gpuanalysis.o

clean:
	rm *.o
