# GStreamer plugin for libopenaptx

### Superseded by https://gitlab.freedesktop.org/gstreamer/gst-plugins-bad/-/merge_requests/1871


## What works

* Encoding
* Decoding

## What needs work

* use error-recovering decoding
* Handling buffers with more than one memory segment
* non-stereo mode?
* inputs which do not exactly contain 4 samples
* error handling besides full assertion failures
