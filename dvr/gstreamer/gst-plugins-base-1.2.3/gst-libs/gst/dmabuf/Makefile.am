lib_LTLIBRARIES = libgstdmabuf-@GST_API_VERSION@.la

libgstdmabuf_@GST_API_VERSION@_la_SOURCES = \
	dmabuf.c
libgstdmabuf_@GST_API_VERSION@includedir = $(includedir)/gstreamer-@GST_API_VERSION@/gst/dmabuf
libgstdmabuf_@GST_API_VERSION@include_HEADERS = \
	dmabuf.h

libgstdmabuf_@GST_API_VERSION@_la_CFLAGS = \
	$(GST_PLUGINS_BASE_CFLAGS)\
	$(GST_BASE_CFLAGS) \
	$(GST_CFLAGS)

libgstdmabuf_@GST_API_VERSION@_la_LIBADD = $(GST_BASE_LIBS) \
	$(GST_LIBS)

libgstdmabuf_@GST_API_VERSION@_la_LDFLAGS = \
	$(GST_LIB_LDFLAGS) \
	$(GST_ALL_LDFLAGS) \
	$(GST_LT_LDFLAGS)

