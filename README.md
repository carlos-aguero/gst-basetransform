# gst-basetransform
Gstreamer 1.0 element using GstBaseTransform class

BaseTransform element created using: gst-element-maker my_element basetransform

Modified later, based on the documentation available at: http://gstreamer.freedesktop.org/data/doc/gstreamer/head/gstreamer-libs/html/GstBaseTransform.html

Compile: 
make

Run:
GST_DEBUG=myelement:7 GST_PLUGIN_PATH=src/.libs/ gst-launch-1.0 videotestsrc num-buffers=300 ! myelement ! fakesink  silent=true -v

