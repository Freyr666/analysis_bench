#!/usr/bin/env python

import sys
import gi
gi.require_version('Gst', '1.0')
gi.require_version('GLib', '2.0')
gi.require_version('GObject', '2.0')
from gi.repository import GLib, GObject, Gst

class Bench:
    class _BenchKind:
        _type = 0

        def __init__(self, typ):
            self._type = typ

        def is_cpu(self):
            return self._type == 0

        def is_gpu(self):
            return self._type == 1

        def is_gpu_no_copy(self):
            return self._type == 2

        def __str__(self):
            if self._type == 0:
                return "CPU"
            elif self._type == 1:
                return "GPU"
            else:
                return "GPU (single texture upload)"

    FPS = 25

    CPU = _BenchKind(0)
    GPU = _BenchKind(1)
    GPUNC = _BenchKind(2)
            
    _loop = None
    _pipe = None
    _kind = None
    _size = None
    _duration = 0
    _data = []
    _avg_fps = 0

    def __cpu_pipe(self, size):
        source = "videotestsrc is-live=true ! video/x-raw,height=720,width=1280,framerate=25/1 ! tee name=t"
        first = " ! queue ! cpuanalysis ! fpsdisplaysink name=rate signal-fps-measurements=true"
        analysis = " t. ! queue ! cpuanalysis ! fakesink" * (size - 1)
        return Gst.parse_launch(source + first + analysis)

    def __gpu_pipe(self, size):
        source = "videotestsrc is-live=true ! video/x-raw,height=720,width=1280,framerate=25/1 ! queue ! tee name=t"
        first = " ! queue ! glupload ! gpuanalysis ! fpsdisplaysink name=rate video-sink=glimagesink signal-fps-measurements=true"
        analysis = " t. ! queue ! glupload ! gpuanalysis ! fakesink" * (size - 1)
        return Gst.parse_launch(source + first + analysis)

    def __gpu_no_copy_pipe(self, size):
        source = "videotestsrc is-live=true ! video/x-raw,height=720,width=1280,framerate=25/1 ! queue ! glupload ! tee name=t"
        first = " ! queue ! gpuanalysis ! fpsdisplaysink name=rate video-sink=glimagesink signal-fps-measurements=true"
        analysis = " t. ! queue ! gpuanalysis ! fakesink" * (size - 1)
        return Gst.parse_launch(source + first + analysis)

    def __stop(self, _none):
        self._pipe.set_state(Gst.State.PAUSED)
        self._pipe.set_state(Gst.State.NULL)
        self._loop.quit()
        #self._pipe.unref()

    def __on_msg(self, _bus, msg, _data):
        if msg.type == Gst.MessageType.APPLICATION and msg.has_name("perf"):
            data = msg.get_structure().get_double("time")
            if data:
                self._data.append(data[1])
        return True

    def __update_fps(self, _elem, _fps, _drop, avg_fps, _udata):
        self._avg_fps = avg_fps
        return True

    def __init__(self, kind=CPU, size=1, duration=20):
        ''' '''
        assert (type(kind) == type(self.CPU))
        assert (size > 0)
        assert (duration > 0)
        #print("Creating a benchmark for {} of size {}".format(kind, size))
        if kind.is_cpu():
            self._pipe = self.__cpu_pipe(size)
        elif kind.is_gpu():
            self._pipe = self.__gpu_pipe(size)
        elif kind.is_gpu_no_copy():
            self._pipe = self.__gpu_no_copy_pipe(size)
        else:
            raise ValueError("Unknown benchmark kind")

        self._kind = kind
        self._size = size
        self._duration = duration
        self._total_frame_number = duration * self.FPS * size

        rate = self._pipe.get_by_name("rate")
        rate.connect("fps-measurements", self.__update_fps, None)
        
        self._loop = GLib.MainLoop()
        self._pipe.get_bus().add_watch(0, self.__on_msg, None)
        GLib.timeout_add_seconds(duration, self.__stop, None)

        self._pipe.set_state(Gst.State.PLAYING)
        self._loop.run()

    def kind(self):
        return self._kind

    def size(self):
        return self._size

    def duration(self):
        return self._duration

    def data(self):
        return self._data

    def average_time(self):
        return sum(self._data)/len(self._data)

    def framerate(self):
        return self._avg_fps
 

def print_results(bench):
    print("{} benchmark ({} branches, {} seconds): {} s, {} FPS"
          .format(bench.kind(),
                  bench.size(),
                  bench.duration(),
                  bench.average_time(),
                  bench.framerate()))

def main(args):
#    if args[1] == 'cpu':
#        kind = Bench.CPU
#    else:
#        kind = Bench.GPU
#    sz = int(args[2])
#    Bench(kind=kind, size=sz, duration=30)
    #b1 = Bench(kind = Bench.CPU, size = 5, duration = 20)
    #print_results(b1)
    #b2 = Bench(kind = Bench.GPU, size = 5, duration = 20)
    #print_results(b2)
    #b3 = Bench(kind = Bench.GPU, size = 50, duration = 20)
    #print_results(b3)
    b3 = Bench(kind = Bench.GPU, size = 50, duration = 20)
    print_results(b3)
    #b4 = Bench(kind = Bench.GPU, size = 100, duration = 20)
    #print_results(b4)


if __name__ == "__main__":
    Gst.init(sys.argv)
    main(sys.argv)
