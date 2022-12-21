AM_CFLAGS = -I$(top_srcdir)/src/include @JSONC_CFLAGS@

lib_LTLIBRARIES = libhailer.la
bin_PROGRAMS = hailer_server hailer_cli

libhailer_la_SOURCES = hailer.c
libhailer_la_LIBADD = @JSONC_LIBS@
libhailer_la_LDFLAGS = -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) -release $(LT_RELEASE)
include_HEADERS = include/*.h

hailer_server_SOURCES = hailer_server.c hailer_peer_discovery.c
hailer_server_CFLAGS = $(AM_CFLAGS)
hailer_server_LDADD = @JSONC_LIBS@ libhailer.la

hailer_cli_SOURCES = hailer_cli.c
hailer_cli_CFLAGS = $(AM_CFLAGS)
hailer_cli_LDADD = libhailer.la