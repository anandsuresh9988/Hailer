bin_PROGRAMS = hailer_test_a hailer_test_b

hailer_test_a_SOURCES = test_app_a.c
hailer_test_a_CFLAGS = $(AM_CFLAGS) -I$(top_srcdir)/src/include
hailer_test_a_LDADD = @JSONC_LIBS@ $(top_srcdir)/src/libhailer.la

hailer_test_b_SOURCES = test_app_b.c
hailer_test_b_CFLAGS = $(AM_CFLAGS) -I$(top_srcdir)/src/include
hailer_test_b_LDADD = @JSONC_LIBS@ $(top_srcdir)/src/libhailer.la
