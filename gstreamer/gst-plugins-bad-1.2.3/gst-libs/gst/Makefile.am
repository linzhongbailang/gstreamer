if HAVE_EGL
EGL_DIR = egl
endif

if USE_KMS
DRM_DIR = drm
endif

SUBDIRS = interfaces basecamerabinsrc codecparsers $(DRM_DIR) \
	 insertbin uridownloader mpegts $(EGL_DIR)

noinst_HEADERS = gst-i18n-plugin.h gettext.h glib-compat-private.h
DIST_SUBDIRS = interfaces egl basecamerabinsrc codecparsers drm \
	insertbin uridownloader mpegts
