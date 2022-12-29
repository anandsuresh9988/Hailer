######################################################################
# Process this file with autoconf to produce a configure script.
######################################################################
AC_PREREQ([2.69])
AC_INIT([hailer], [0.1], [anandsuresh9988@gmail.com], [hailer], [https://github.com/anandsuresh9988/Hailer])
AC_CONFIG_SRCDIR([src/hailer.c])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([config/m4])
AC_CONFIG_HEADERS([src/include/config.h])

AM_INIT_AUTOMAKE([foreign subdir-objects -Wall])
AM_SILENT_RULES([yes])

######################################################################
# Versioning
# Each release increases HAILER_MICRO_VERSION, HAILER_INTERFACE_AGE and HAILER_BINARY_AGE by 1.
# if any functions have been added, set HAILER_INTERFACE_AGE to 0.
# If backwards compatibility has been broken, set HAILER_INTERFACE_AGE and HAILER_BINARY_AGE to 0.
######################################################################
HAILER_MAJOR_VERSION=0
HAILER_MINOR_VERSION=1
HAILER_MICRO_VERSION=0
HAILER_INTERFACE_AGE=0
HAILER_BINARY_AGE=0
HAILER_VERSION=$HAILER_MAJOR_VERSION.$HAILER_MINOR_VERSION.$HAILER_MICRO_VERSION

AC_SUBST(HAILER_MAJOR_VERSION)
AC_SUBST(HAILER_MINOR_VERSION)
AC_SUBST(HAILER_MICRO_VERSION)
AC_SUBST(HAILER_INTERFACE_AGE)
AC_SUBST(HAILER_BINARY_AGE)
AC_SUBST(HAILER_VERSION)

# libtool versioning
LT_RELEASE=$HAILER_MAJOR_VERSION.$HAILER_MINOR_VERSION
LT_CURRENT=`expr $HAILER_MICRO_VERSION - $HAILER_INTERFACE_AGE`
LT_BINARY=`expr $HAILER_MICRO_VERSION - $HAILER_BINARY_AGE`
LT_REVISION=$HAILER_INTERFACE_AGE
LT_AGE=`expr $HAILER_BINARY_AGE - $HAILER_INTERFACE_AGE`
AC_SUBST(LT_RELEASE)
AC_SUBST(LT_CURRENT)
AC_SUBST(LT_BINARY)
AC_SUBST(LT_REVISION)
AC_SUBST(LT_AGE)

######################################################################
# Dependencies
######################################################################
AC_LANG([C])
AC_PROG_INSTALL
AM_PROG_AR
LT_INIT
PKG_PROG_PKG_CONFIG

AC_SEARCH_LIBS([pthread_create], [pthread],[],[AC_MSG_ERROR([You need to install pthreads library.])])

AC_CHECK_HEADERS([stdlib.h stdarg.h stdio.h signal.h unistd.h])

PKG_CHECK_MODULES([JSONC], [json-c >= 0.12])

######################################################################
AC_ARG_WITH([tests], [AS_HELP_STRING([--with-tests], [compile tests @<:@default=no@:>@])], [], [with_tests=no])
AM_CONDITIONAL([WITH_TESTS], [test x$with_tests = xyes])

AC_CONFIG_FILES([Makefile src/Makefile test-app/Makefile hailer.pc:hailer.pc.in])
AC_OUTPUT
