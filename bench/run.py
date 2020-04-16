#!/usr/bin/env python

import sys
import gi
gi.require_version('Gst', '1.0')
gi.require_version('GLib', '2.0')
gi.require_version('GObject', '2.0')
from gi.repository import GLib, GObject, Gst

import numpy as np
import matplotlib.pyplot as plt

class Bench:
    class _BenchKind:
        _type = 0

        def __init__(self, typ):
            self._type = typ
            
        def is_gpu(self):
            return self._type == 0

        def is_gpu_decode(self):
            return self._type == 1

        def is_gpu_no_copy(self):
            return self._type == 2

        def __str__(self):
            if self._type == 0:
                return "Upload"
            elif self._type == 1:
                return "Upload + decode"
            else:
                return "No texture upload"

    FPS = 25

    Upload = _BenchKind(0)
    UploadDec = _BenchKind(1)
    NoUpload = _BenchKind(2)
            
    _loop = None
    _pipe = None
    _kind = None
    _size = None
    _duration = 0
    _fps = []

    def __gpu_pipe_decode(self, size):
        source = "videotestsrc is-live=true ! video/x-raw,height=720,width=1280,framerate=25/1 ! x264enc ! queue ! tee name=t"
        first = " ! queue ! avdec_h264 ! glupload ! glcolorconvert ! gleffects_fisheye ! fpsdisplaysink name=rate video-sink=fakesink signal-fps-measurements=true"
        analysis = " t. ! queue ! avdec_h264 ! glupload ! glcolorconvert ! gleffects_fisheye ! fakesink" * (size - 1)
        return Gst.parse_launch(source + first + analysis)

    def __gpu_pipe(self, size):
        source = "videotestsrc is-live=true ! video/x-raw,height=720,width=1280,framerate=25/1 ! queue ! tee name=t"
        first = " ! queue ! glupload ! gleffects_fisheye ! fpsdisplaysink name=rate video-sink=fakesink signal-fps-measurements=true"
        analysis = " t. ! queue ! glupload ! gleffects_fisheye ! fakesink" * (size - 1)
        return Gst.parse_launch(source + first + analysis)

    def __gpu_no_copy_pipe(self, size):
        source = "videotestsrc is-live=true ! video/x-raw,height=720,width=1280,framerate=25/1 ! queue ! glupload ! tee name=t"
        first = " ! queue ! gleffects_fisheye ! fpsdisplaysink name=rate video-sink=fakesink signal-fps-measurements=true"
        analysis = " t. ! queue ! gleffects_fisheye ! fakesink" * (size - 1)
        return Gst.parse_launch(source + first + analysis)

    def __stop(self, _none):
        #self._pipe.set_state(Gst.State.PAUSED)
        self._pipe.set_state(Gst.State.NULL)
        self._loop.quit()
        #self._pipe.unref()

    def __update_fps(self, _elem, fps, _drop, _avg_fps, _udata):
        self._fps.append(fps)
        return True

    def __init__(self, kind=Upload, size=1, duration=20):
        ''' '''
        assert (type(kind) == type(self.Upload))
        assert (size > 0)
        assert (duration > 0)
        print("Creating a benchmark for {} of size {}".format(kind, size))
        if kind.is_gpu():
            self._pipe = self.__gpu_pipe(size)
        elif kind.is_gpu_decode():
            self._pipe = self.__gpu_pipe_decode(size)
        elif kind.is_gpu_no_copy():
            self._pipe = self.__gpu_no_copy_pipe(size)
        else:
            raise ValueError("Unknown benchmark kind")

        self._kind = kind
        self._size = size
        self._duration = duration
        self._fps = []

        rate = self._pipe.get_by_name("rate")
        rate.connect("fps-measurements", self.__update_fps, None)
        
        self._loop = GLib.MainLoop()
        GLib.timeout_add_seconds(duration, self.__stop, None)

        self._pipe.set_state(Gst.State.PLAYING)
        self._loop.run()

    def kind(self):
        return self._kind

    def size(self):
        return self._size

    def duration(self):
        return self._duration

    def framerate(self):
        array = np.array(self._fps)
        return array.mean(), array.std()
 

def print_results(bench):
    print("{} benchmark ({} branches, {} seconds): {} FPS"
          .format(bench.kind(),
                  bench.size(),
                  bench.duration(),
                  bench.framerate()))
    
def main(args):
    interval = [x for x in range(1,40) if x % 5 == 0 or x == 1]
    copy = []
    copy_dec = []
    no_copy = []
    
    for size in interval:
        c = Bench(kind=Bench.Upload, size=size)
        cd = Bench(kind=Bench.UploadDec, size=size)
        nc = Bench(kind=Bench.NoUpload, size=size)
        copy.append(c.framerate())
        copy_dec.append(cd.framerate())
        no_copy.append(nc.framerate())
        print_results(c)
        print_results(cd)
        print_results(nc)

    x = np.array(interval)
    copy_mean = np.array(list(map(lambda x: x[0], copy)))
    copy_std = np.array(list(map(lambda x: x[1], copy)))
    copy_dec_mean = np.array(list(map(lambda x: x[0], copy_dec)))
    copy_dec_std = np.array(list(map(lambda x: x[1], copy_dec)))
    no_copy_mean = np.array(list(map(lambda x: x[0], no_copy)))
    no_copy_std = np.array(list(map(lambda x: x[1], no_copy)))

    fig = plt.figure()
    plt.xlabel('Количество программ')
    plt.ylabel('Средняя частота кадров, кадров в секунду')
    plt.title('Деградация производительности')
    plt.errorbar(x, copy_mean, yerr=copy_std, fmt='-o', label='Copy', color='blue', ecolor='blue')
    plt.errorbar(x, copy_dec_mean, yerr=copy_dec_std, fmt='-o', label='Copy + decode', color='blue', ecolor='green')
    plt.errorbar(x, no_copy_mean, yerr=no_copy_std, fmt='-o', label='No Copy', color='red', ecolor='red')
    plt.legend(loc='lower right')
    plt.show()
    #plt.savefig("plots/degrad_{}.svg".format(prefix))

if __name__ == "__main__":
    Gst.init(sys.argv)
    main(sys.argv)
