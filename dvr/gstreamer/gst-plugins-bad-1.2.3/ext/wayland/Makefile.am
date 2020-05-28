plugin_LTLIBRARIES = libgstwaylandsink.la

libgstwaylandsink_la_SOURCES =  gstwaylandsink.c waylandpool.c gstwlbufferpriv.c wayland-drm-protocol.c
libgstwaylandsink_la_CFLAGS = $(GST_CFLAGS) $(GST_PLUGINS_BASE_CFLAGS) \
                                $(GST_PLUGINS_BAD_CFLAGS) \
                               $(WAYLAND_CFLAGS) $(DRM_CFLAGS)
libgstwaylandsink_la_LIBADD = $(GST_PLUGINS_BASE_LIBS) \
                               -lgstvideo-$(GST_API_VERSION) \
                               $(WAYLAND_LIBS) \
                               -lgstdmabuf-$(GST_API_VERSION) \
                               $(top_builddir)/gst-libs/gst/drm/libgstdrm-$(GST_API_VERSION).la
libgstwaylandsink_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstwaylandsink_la_LIBTOOLFLAGS = $(GST_PLUGIN_LIBTOOLFLAGS)

noinst_HEADERS = gstwaylandsink.h waylandpool.h gstwlbufferpriv.h  wayland-drm-client-protocol.h
