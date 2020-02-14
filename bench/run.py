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
        def __init__ (self, typ):
            self._type = typ
        def is_cpu (self):
            return self._type == 0
        def is_gpu (self):
            return self._type == 1
        def __str__(self):
            if self._type == 0:
                return "CPU"
            else:
                return "GPU"

    _frame_time = 0.0
    _frames = 0
    _loop = None
    _pipe = None

    CPU = _BenchKind(0)
    GPU = _BenchKind(1)

    def __cpu_pipe (self, size):
        source = "videotestsrc is-live=true ! video/x-raw,height=720,width=1280 ! tee name=t"
        first = " ! queue ! cpuanalysis ! autovideosink"
        analysis = " t. ! queue ! cpuanalysis ! fakesink" * (size - 1)
        return Gst.parse_launch(source + first + analysis)

    def __gpu_pipe (self, size):
        source = "videotestsrc is-live=true ! video/x-raw,height=720,width=1280 ! queue ! tee name=t"
        first = " ! queue ! glupload ! gpuanalysis ! glimagesink"
        analysis = " t. ! queue ! glupload ! gpuanalysis ! fakesink" * (size - 1)
        print (source + first + analysis)
        return Gst.parse_launch(source + first + analysis)

    def __stop (self, _none):
        self._pipe.set_state(Gst.State.NULL)
        self._loop.quit()
        self._pipe.unref()

    def __on_msg (self, _bus, msg, _data):
        if msg.has_name("perf"):
            data = msg.get_structure().get_double("time")
            if data:
                self._frame_time += data[1]
                self._frames += 1
        return True
    
    def __init__ (self, kind = CPU, size = 1, duration = 20):
        ''' '''
        assert (type(kind) == type(self.CPU))
        assert (size > 0)
        assert (duration > 0)
        print ("Creating a benchmark for {} of size {}".format(kind, size))
       
        if kind.is_cpu():
            self._pipe = self.__cpu_pipe (size)
        elif kind.is_gpu():
            self._pipe = self.__gpu_pipe (size)
        else:
            raise ValueError("Unknown benchmark kind")
        
        self._loop = GLib.MainLoop()

        self._pipe.get_bus().add_watch(0, self.__on_msg, None)
        GLib.timeout_add_seconds(duration, self.__stop, None)

        self._pipe.set_state(Gst.State.PLAYING)
        self._loop.run()

    def result (self):
        return self._frame_time / self._frames
        
def main(args):
    b1 = Bench(kind = Bench.CPU, size = 25, duration = 10)
    print ("Result is: {}".format(b1.result()))
    #b2 = Bench(kind = Bench.GPU, size = 20, duration = 10)
    #print ("Result is: {}".format(b2.result()))
    # b3 = Bench(kind = Bench.GPU, size = 5, duration = 3)
    # print ("Result is: {}".format(b3.result()))
    # b4 = Bench(kind = Bench.GPU, size = 10, duration = 3)
    # print ("Result is: {}".format(b4.result()))

if __name__ == "__main__":
    Gst.init(sys.argv)
    main(sys.argv)
