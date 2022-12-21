pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = hailer.pc
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I config/m4

SUBDIRS = src

if WITH_TESTS
SUBDIRS	+=	test-app
endif

DIST_SUBDIRS = src test-app
EXTRA_DIST = LICENSE README.md
