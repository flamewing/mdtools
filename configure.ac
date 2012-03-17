dnl Process this file with autoconf to produce a configure script.
dnl Created by Anjuta application wizard.

AC_INIT(mdtools, 0.2.0)

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.11])

AM_SILENT_RULES([yes])
AM_MAINTAINER_MODE

AC_PROG_CXX

AC_CHECK_HEADER(mdcomp/enigma.h,break,[HAVE_ENIGMA="no"])
if test "$HAVE_ENIGMA" = "no"; then
    echo "Fatal error: Enigma header not found!"
    exit 1
fi
AC_CHECK_HEADER(mdcomp/kosinski.h,break,[HAVE_KOSINSKI="no"])
if test "$HAVE_KOSINSKI" = "no"; then
    echo "Fatal error: Kosinski header not found!"
    exit 1
fi

dnl ***************************************************************************
dnl Internatinalization
dnl ***************************************************************************
IT_PROG_INTLTOOL([0.35.0])

GETTEXT_PACKAGE=mdtools
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [GETTEXT package name])
AM_GLIB_GNU_GETTEXT

LT_INIT

AC_OUTPUT([
Makefile
src/Makefile
src/lib/Makefile
src/tools/Makefile
po/Makefile.in
])
