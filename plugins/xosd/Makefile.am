EXTRA_DIST = vars-pl.txt commands-pl.txt 

lib_LTLIBRARIES = xosd.la
xosd_la_SOURCES = xosd.c
xosd_la_LDFLAGS = -module -avoid-version
xosd_la_LIBADD = $(LIBXOSD_LIBS) -lxosd
xosd_la_CFLAGS = $(LIBXOSD_CFLAGS) $(AM_CFLAGS)
# TODO: add $(C_FVISIBILITY) to *_CFLAGS or comment why this plugin should not be using it.

override libdir = $(plugindir)

install-data-am:
	mkdir -p $(DESTDIR)$(pkgdatadir)/plugins/xosd/
	$(INSTALL) -m 644 $(EXTRA_DIST) $(DESTDIR)$(pkgdatadir)/plugins/xosd/

