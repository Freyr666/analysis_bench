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

        def __str__(self):
            if self._type == 0:
                return "CPU"
            else:
                return "GPU"

    _loop = None
    _pipe = None

    CPU = _BenchKind(0)
    GPU = _BenchKind(1)

    def __cpu_pipe(self, size):
        source = "videotestsrc is-live=true ! video/x-raw,height=720,width=1280,framerate=25/1 ! tee name=t"
        first = " ! queue ! cpuanalysis ! fakesink"
        analysis = " t. ! queue ! cpuanalysis ! fakesink" * (size - 1)
        return Gst.parse_launch(source + first + analysis)

    def __gpu_pipe(self, size):
        source = "videotestsrc is-live=true ! video/x-raw,height=720,width=1280,framerate=25/1 ! queue ! tee name=t"
        first = " ! queue ! glupload ! gpuanalysis ! fakesink"
        analysis = " t. ! queue ! glupload ! gpuanalysis ! fakesink" * (size - 1)
        #print(source + first + analysis)
        return Gst.parse_launch(source + first + analysis)

    def __stop(self, _none):
        self._pipe.set_state(Gst.State.NULL)
        self._loop.quit()
        self._pipe.unref()

    def __on_msg(self, _bus, msg, _data):
        if msg.type == Gst.MessageType.APPLICATION and msg.has_name("perf"):
            data = msg.get_structure().get_double("time")
            if data:
                print(data[1])
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
        else:
            raise ValueError("Unknown benchmark kind")

        self._loop = GLib.MainLoop()
        self._pipe.get_bus().add_watch(0, self.__on_msg, None)
        GLib.timeout_add_seconds(duration, self.__stop, None)

        self._pipe.set_state(Gst.State.PLAYING)
        self._loop.run()


def main(args):
    if args[1] == 'cpu':
        kind = Bench.CPU
    else:
        kind = Bench.GPU
    sz = int(args[2])
    Bench(kind=kind, size=sz, duration=30)


if __name__ == "__main__":
    Gst.init(sys.argv)
    main(sys.argv)
