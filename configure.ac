AC_INIT([xbmc-pvr-addons], 1:0:0)

AC_CANONICAL_HOST
AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION, [http://www.xbmc.org])

PREFIX_DEFAULT="/usr/local"
AC_PREFIX_DEFAULT($PREFIX_DEFAULT)

AC_PROG_CXX
AC_PROG_CPP
AC_PROG_LIBTOOL

AC_SEARCH_LIBS([pthread_create],[pthread],, AC_MSG_ERROR("required library 'pthread' is missing"))

AC_ARG_ENABLE([release],
  [AS_HELP_STRING([--enable-release],
  [build release binaries (default is no)])],
  [use_release=$enableval],
  [use_release=no])

### External libraries options
AC_ARG_ENABLE([external-libraries],
  [AS_HELP_STRING([--enable-external-libraries],
  [enable use of all supported external libraries (default is no) 'Linux only'])],
  [use_external_libraries=$enableval],
  [use_external_libraries=no])

AC_ARG_ENABLE([external-ffmpeg],
  [AS_HELP_STRING([--enable-external-ffmpeg],
  [enable use of external ffmpeg libraries (default is no) 'Linux only'])],
  [use_external_ffmpeg=$enableval],
  [use_external_ffmpeg=$use_external_libraries])

BUILD_TYPE="debug"
if test "$use_release" = "yes"; then
  BUILD_TYPE="release"
fi

OS="unknown"
ARCHITECTURE="unknown"
HOST_INCLUDES=""
HOST_CXXFLAGS=""
ARCH_DEFINES="-DTARGET_POSIX -DTARGET_LINUX -D_LINUX"
DYN_LIB_EXT="so"

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
    ARCH_DEFINES="-DTARGET_POSIX -DTARGET_DARWIN -D_LINUX"
    DYN_LIB_EXT="dylib"
    ;;
  *-freebsd*)
    OS="freebsd"
    ARCH_DEFINES="-DTARGET_POSIX -DTARGET_FREEBSD -D_LINUX"
    ;;
esac

### External libraries checks
# Curl
use_libcurl="no"
PKG_CHECK_MODULES([CURL],[libcurl],use_libcurl="yes",AC_MSG_WARN("CURL was not found, N7 add-on will not be available"))

### Check for Intree building
if test "x${cross_compiling}" = "xyes" || test "x${cross_compiling}" = "xmaybe"; then
  # don't call AC_CHECK_FILE when (maybe) cross-compiling
  AM_CONDITIONAL(IS_INTREE_BUILD, false)
  intree=false
else
  checkpath=".."
  AC_CHECK_FILE([$checkpath/xbmc/xbmc.h], [AM_CONDITIONAL(IS_INTREE_BUILD, true) intree=true], [AM_CONDITIONAL(IS_INTREE_BUILD, false) intree=false])
  echo "Intree build: $intree"
fi

# FFmpeg
if test "$use_external_ffmpeg" = "yes"; then
  FFMPEG_LIBNAMES="libavcodec" 
  PKG_CHECK_MODULES([FFMPEG], [$FFMPEG_LIBNAMES],
                    [FFMPEG_INCLUDES="$FFMPEG_CFLAGS"; LIBS="$LIBS $FFMPEG_LIBS"],
                    AC_MSG_ERROR(missing $FFMPEG_LIBNAMES))

 # Determine whether AVPacket and relevant functions are defined in libavformat
 # or libavcodec
  AC_CHECK_LIB([avcodec], [av_free_packet],
  [AC_MSG_NOTICE(== AVPacket and relevant functions defined in libavcodec. ==)],
  [AC_MSG_NOTICE(== AVPacket and relevant functions defined in libavformat. ==)
   AC_DEFINE([AVPACKET_IN_AVFORMAT], [1], [Whether AVPacket is in libavformat.])])

  # Possible places the ffmpeg headers may be
  #AC_CHECK_HEADERS([libavcodec/avcodec.h ffmpeg/avcodec.h],, AC_MSG_ERROR(avcodec.h not found),)
  AC_CHECK_HEADERS([libavcodec/avcodec.h],[FFMPEG_INCLUDES="-I$($PKG_CONFIG --variable=includedir libavcodec)/libavcodec"],
    [AC_CHECK_HEADERS([ffmpeg/avcodec.h],[FFMPEG_INCLUDES="-I$($PKG_CONFIG --variable=includedir libavcodec)/ffmpeg"],
      [AC_MSG_ERROR(avcodec.h not found)])
  ])

  echo "FFMPEG_INCLUDES: $FFMPEG_INCLUDES"
  AM_CONDITIONAL(USE_EXTERNAL_FFMPEG, true)
  AC_SUBST(FFMPEG_INCLUDES)
else
  AM_CONDITIONAL(USE_EXTERNAL_FFMPEG, false)
fi


HOST_CXXFLAGS="-Wall -Wextra -Wno-missing-field-initializers -Woverloaded-virtual -Wno-parentheses -fPIC $HOST_CXXFLAGS"

AC_SUBST(ARCHITECTURE)
AC_SUBST(BUILD_TYPE)
AC_SUBST(HOST_CXXFLAGS)
AC_SUBST(HOST_INCLUDES)
AC_SUBST(OS)
AC_SUBST(ARCH_DEFINES)
AC_SUBST(DYN_LIB_EXT)

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

if test "x${libdir}" != 'x${exec_prefix}/lib'; then
  LIBDIR=${libdir}
elif test "$prefix" = "NONE"; then
  LIBDIR=$PREFIX_DEFAULT/lib/xbmc/addons
else
  LIBDIR=$prefix/lib/xbmc/addons
fi
AC_SUBST(LIBDIR)

if test "x${datadir}" != 'x${datarootdir}' && test "x${datarootdir}" = 'x${prefix}/share'; then
  DATADIR=${datadir}
elif test "$prefix" = "NONE"; then
  DATADIR=$PREFIX_DEFAULT/share/xbmc/addons
else
  DATADIR=$prefix/share/xbmc/addons 
fi
AC_SUBST(DATADIR)


AC_OUTPUT([Makefile
           lib/Makefile
           lib/jsoncpp/Makefile
           lib/libhts/Makefile
           lib/tinyxml/Makefile
           addons/Makefile
           addons/pvr.demo/Makefile
           addons/pvr.fortherecord.argus/Makefile
           addons/pvr.hts/Makefile
           addons/pvr.mediaportal.tvserver/Makefile
           addons/pvr.njoy/Makefile
           addons/pvr.vdr.vnsi/Makefile])

