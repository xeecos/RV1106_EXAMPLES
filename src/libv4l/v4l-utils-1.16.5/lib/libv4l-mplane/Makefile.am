if WITH_V4L_BUILTIN_PLUGINS
noinst_LTLIBRARIES = libv4l-mplane.la
else
if WITH_V4L_PLUGINS
libv4l2plugin_LTLIBRARIES = libv4l-mplane.la
endif
endif

libv4l_mplane_la_SOURCES = libv4l-mplane.c
if WITH_V4L_BUILTIN_PLUGINS
libv4l_mplane_la_CPPFLAGS = -static
libv4l_mplane_la_LDFLAGS = -static
else
libv4l_mplane_la_CPPFLAGS = $(CFLAG_VISIBILITY)
libv4l_mplane_la_LDFLAGS = -avoid-version -module -shared -export-dynamic -lpthread
endif
