lib_LTLIBRARIES = ruby.la
ruby_la_SOURCES = ruby_ekg.c ruby_ekg.h ruby_theme.c
ruby_la_LDFLAGS = -module -avoid-version
ruby_la_LIBADD = $(RUBY_LIBS)
ruby_la_CFLAGS = $(C_FVISIBILITY) $(RUBY_CFLAGS) $(AM_CFLAGS)

INCLUDES = -I$(top_srcdir)
override libdir = $(plugindir)
