#!/usr/bin/env python

import sys

import gi
gi.require_version('Gst', '1.0')
gi.require_version('GLib', '2.0')
gi.require_version('GObject', '2.0')
from gi.repository import GLib, GObject, Gst

frame_time = 0.0
frames = 0

def on_msg(_bus, msg, _data):
    global frame_time
    global frames
    if msg.has_name("perf"):
        data = msg.get_structure().get_double("time")
        frame_time += data[1]
        frames += 1
    return True

def stop(pipe):
    print("Stopping the pipeline...")
    pipe.set_state(Gst.State.NULL)
    print("Stopped, avg frame time is: " + str(frame_time / frames))
    sys.exit(0)

def main(args):
    Gst.init(None)

    pipe = Gst.parse_launch("videotestsrc ! video/x-raw,height=720,width=1280 ! cpuanalysis ! autovideosink")

    #pipe = Gst.parse_launch("gltestsrc is-live=true ! glcolorconvert ! video/x-raw(ANY),height=720,width=1280 ! gpuanalysis ! glimagesink")

    bus = pipe.get_bus()
    bus.add_watch(0, on_msg, None)
    
    GLib.timeout_add_seconds(20, stop, pipe)
    
    pipe.set_state(Gst.State.PLAYING)

    GLib.MainLoop().run()

if __name__ == "__main__":
    main(sys.argv)
