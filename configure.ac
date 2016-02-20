dnl Process this file with autoconf to produce a configure script.
dnl Created by Anjuta application wizard.

AC_INIT(mdtools, 0.2.0)

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.11])

AM_SILENT_RULES([yes])
AM_MAINTAINER_MODE

AC_PROG_CXX
AC_LANG([C++])
AX_CXX_COMPILE_STDCXX_14([],[mandatory])
AC_HEADER_STDC

CXXFLAGS="$CXXFLAGS -Wall -Wextra -pedantic -Wcast-qual -Wold-style-cast"
CXXFLAGS="$CXXFLAGS -Wwrite-strings -Wredundant-decls -fcheck-new"
CXXFLAGS="$CXXFLAGS -Wstrict-overflow -Wdisabled-optimization -Wctor-dtor-privacy"
CXXFLAGS="$CXXFLAGS -Wnon-virtual-dtor -Woverloaded-virtual"

AC_CHECK_HEADER(mdcomp/comper.h,break,[HAVE_COMPER="no"])
if test "$HAVE_COMPER" = "no"; then
    echo "Fatal error: Comper header not found!"
    exit 1
fi
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
AC_CHECK_HEADER(mdcomp/nemesis.h,break,[HAVE_NEMESIS="no"])
if test "$HAVE_NEMESIS" = "no"; then
    echo "Fatal error: Nemesis header not found!"
    exit 1
fi
AC_CHECK_HEADER(mdcomp/saxman.h,break,[HAVE_SAXMAN="no"])
if test "$HAVE_SAXMAN" = "no"; then
    echo "Fatal error: Saxman header not found!"
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
