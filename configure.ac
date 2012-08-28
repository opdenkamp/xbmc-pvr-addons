AC_INIT([xbmc-pvr-addons], 1:0:0)

AC_CANONICAL_HOST
AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION, [http://www.xbmc.org])

AC_PREFIX_DEFAULT(/usr/lib/xbmc/addons)

AC_PROG_CXX
AC_PROG_CPP
AC_PROG_LIBTOOL

AC_SEARCH_LIBS([pthread_create],[pthread],, AC_MSG_ERROR("required library 'pthread' is missing"))

AC_ARG_ENABLE([release],
  [AS_HELP_STRING([--enable-release],
  [build release binaries (default is no)])],
  [use_release=$enableval],
  [use_release=no])

BUILD_TYPE="debug"
if test "$use_release" = "yes"; then
  BUILD_TYPE="release"
fi

OS="unknown"
ARCHITECTURE="unknown"
HOST_INCLUDES=""
HOST_CXXFLAGS=""
ARCH_DEFINES="-DTARGET_POSIX -DTARGET_LINUX -D_LINUX"

case "${host}" in
  arm*-*-linux*)
    OS="linux"
    ARCHITECTURE="arm"
    ;;
  powerpc-*-linux*)
    OS="linux"
    ARCHITECTURE="powerpc"
    ;;
  powerpc64-*-linux*)
    OS="linux"
    ARCHITECTURE="powerpc64"
    ;;
  mipsel-*-linux*)
    OS="linux"
    ARCHITECTURE="mipsel"
    ;;
  x86_64-*-linux*)
    OS="linux"
    ARCHITECTURE="x86_64"
    ;;
  *-*-linux*)
    OS="linux"
    ARCHITECTURE="i486"
    ;;
  *-apple-darwin*)
    OS="darwin"
    HOST_CXXFLAGS="-dynamiclib -single_module -undefined dynamic_lookup"
    host_os_osx="yes"
    ARCH_DEFINES="-DTARGET_POSIX -DTARGET_DARWIN"
    ;;
  *-freebsd*)
    OS="freebsd"
    ARCH_DEFINES="-DTARGET_POSIX -DTARGET_FREEBSD"
    ;;
esac

use_libcurl="no"
PKG_CHECK_MODULES([CURL],[libcurl],use_libcurl="yes",AC_MSG_WARN("CURL was not found, N7 add-on will not be available"))

HOST_CXXFLAGS="-Wall -Wextra -Wno-missing-field-initializers -Woverloaded-virtual -Wno-parentheses -fPIC $HOST_CXXFLAGS"

AC_SUBST(ARCHITECTURE)
AC_SUBST(BUILD_TYPE)
AC_SUBST(HOST_CXXFLAGS)
AC_SUBST(HOST_INCLUDES)
AC_SUBST(OS)
AC_SUBST(ARCH_DEFINES)

if test "x$host_os_osx" = "xyes"; then
  AM_CONDITIONAL(HOST_IS_OSX, true)
else
  AM_CONDITIONAL(HOST_IS_OSX, false)
fi

if test "x$use_libcurl" = "xyes"; then
  INCLUDES="$INCLUDES $CURL_CFLAGS"
  LIBS="$LIBS $CURL_LIBS"
  AM_CONDITIONAL(USE_CURL, true)
else
  AM_CONDITIONAL(USE_CURL, false)
fi

AC_OUTPUT([Makefile
           lib/Makefile
           lib/libhts/Makefile
           lib/tinyxml/Makefile
           addons/Makefile
           addons/pvr.demo/Makefile
           addons/pvr.hts/Makefile
           addons/pvr.mediaportal.tvserver/Makefile
           addons/pvr.njoy/Makefile
           addons/pvr.vdr.vnsi/Makefile])

