AC_INIT([xbmc-pvr-addons], 1:0:0)

AC_CANONICAL_HOST
AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION, [http://www.xbmc.org])

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([no])])

PREFIX_DEFAULT="/usr/local"
AC_PREFIX_DEFAULT($PREFIX_DEFAULT)

AC_PROG_CXX
AC_PROG_CPP
AC_PROG_LIBTOOL

AC_SEARCH_LIBS([pthread_create],[pthread],, AC_MSG_ERROR([required library 'pthread' is missing]))

AC_ARG_ENABLE([release],
  [AS_HELP_STRING([--enable-release],
  [build release binaries (default is no)])],
  [use_release=$enableval],
  [use_release=no])

### External dependencies option
AC_ARG_ENABLE([addons-with-dependencies],
  [AS_HELP_STRING([--enable-addons-with-dependencies],
  [enable build of addons that depend on external libraries (default is no)])],
  [build_addons_with_dependencies=$enableval],
  [build_addons_with_dependencies=no])

AC_ARG_ENABLE([mysql],
  [AS_HELP_STRING([--disable-mysql],
  [disable mysql])],
  [use_mysql=$enableval],
  [use_mysql=yes])

BUILD_TYPE="debug"
if test "$use_release" = "yes"; then
  BUILD_TYPE="release"
fi

AC_CHECK_PROG(HAVE_PKG_CONFIG, pkg-config, yes)

OS="unknown"
ARCHITECTURE="unknown"
HOST_INCLUDES=""
HOST_CXXFLAGS=""
ARCH_DEFINES="-DTARGET_POSIX -DTARGET_LINUX -D_LINUX"
DYN_LIB_EXT="so"
TARGET_LDFLAGS=""
BIN_EXT=".pvr"
BINPREFIX=""
case "${host}" in
  arm-*-linux-androideabi)
    TARGET_LDFLAGS="-avoid-version -no-undefined"
    OS="android"
    ARCHITECTURE="arm"
    BIN_EXT=".so"
    BINPREFIX="lib"
    ;;
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
  arm-apple-darwin*)
    OS="ios"
    HOST_CXXFLAGS="-dynamiclib -single_module -undefined dynamic_lookup"
    host_os_osx="yes"
    ARCH_DEFINES="-DTARGET_POSIX -DTARGET_DARWIN -D_LINUX"
    DYN_LIB_EXT="dylib"
    ;;
  x86_64-apple-darwin*)
    OS="osx64"
    HOST_CXXFLAGS="-dynamiclib -single_module -undefined dynamic_lookup"
    host_os_osx="yes"
    ARCH_DEFINES="-DTARGET_POSIX -DTARGET_DARWIN -D_LINUX"
    DYN_LIB_EXT="dylib"
    ;;
  *-apple-darwin*)
    OS="osx32"
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
AC_CHECK_LIB([GL],[main],have_gl=yes, have_gl=no)  
if test "$have_gl" = "yes"; then
  AC_DEFINE([HAVE_GL],[1],["Define to 1 if we have gl"])
fi
echo "GL: $have_gl"

AC_CHECK_LIB([GLESv2],[main],have_gles2=yes, have_gles2=no)
if test "$have_gles2" = "yes"; then
  AC_DEFINE([HAVE_GLES2],[1],["Define to 1 if we have gles2"])
  AM_CONDITIONAL(HAVE_GLES2, true)
else
  AM_CONDITIONAL(HAVE_GLES2, false)
fi
echo "GLES2: $have_gles2"

### End external Libraries 

### Build addons with dependencies
if test "$build_addons_with_dependencies" = "yes"; then
  echo "Building addons with dependencies"

  ### Libs that addons depend on
  if test "$use_mysql" = "yes"; then
    AC_PATH_PROG(MYSQL_CONFIG, mysql_config, "no")
    if test "x$MYSQL_CONFIG" != "xno"; then
      AC_DEFINE([HAVE_MYSQL],[1],["Define to 1 if you have the `mysql' library (-lmysqlclient)."])
      MYSQL_INCLUDES=`$MYSQL_CONFIG --include`
      MYSQL_LIBS=`$MYSQL_CONFIG --libs`
      AC_SUBST(MYSQL_INCLUDES)
      AC_SUBST(MYSQL_LIBS)
    else
      use_mysql=no
    fi
  fi
  echo "MySQL: $use_mysql"

  # Checks for boost headers
  AC_LANG_PUSH([C++])
  AC_CHECK_HEADER([boost/shared_ptr.hpp], use_boost=yes, use_boost=no,)
  AC_LANG_POP([C++])
  if test "$use_boost" = "yes"; then
    AC_DEFINE([HAVE_BOOST],[1],["Define to 1 if we have the `boost' headers"])
  fi
  echo "Boost: $use_boost"

  AC_CHECK_LIB([z],[inflate], use_zlib=yes, use_zlib=no)
  if test "$use_zlib" = "yes"; then
    AC_DEFINE([HAVE_ZLIB],[1],["Define to 1 if we have the zlib library"])
    ZLIB_LIBS='-lz'
    AC_SUBST(ZLIB_LIBS)
  fi
  echo "Zlib: $use_zlib"
fi

### Addons with dependencies
AM_CONDITIONAL([ADDON_MYTHTV], [test "$build_addons_with_dependencies" = "yes" -a "$use_mysql" = "yes" -a "$use_boost" = "yes"])
AM_CONDITIONAL([ADDON_IPTVSIMPLE], [test "$build_addons_with_dependencies" = "yes" -a "$use_zlib" = "yes"])

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

# Check for intrinsics
AC_MSG_CHECKING([for __sync_add_and_fetch(temp, 1)])
AC_TRY_COMPILE([],[long* temp=0; __sync_add_and_fetch(temp, 1);],
                [have_builtin_sync_add_and_fetch=yes],
                [have_builtin_sync_add_and_fetch=no])
AC_MSG_RESULT($have_builtin_sync_add_and_fetch)
if test "x$have_builtin_sync_add_and_fetch" = "xyes"; then
    AC_DEFINE(HAS_BUILTIN_SYNC_ADD_AND_FETCH, 1,
        [Define to 1 if your compiler supports the __sync_add_and_fetch() intrinsic.])
fi

AC_MSG_CHECKING([for __sync_sub_and_fetch(temp, 1)])
AC_TRY_COMPILE([],[long* temp=0; __sync_sub_and_fetch(temp, 1);],
                [have_builtin_sync_sub_and_fetch=yes],
                [have_builtin_sync_sub_and_fetch=no])
AC_MSG_RESULT($have_builtin_sync_sub_and_fetch)
if test "x$have_builtin_sync_sub_and_fetch" = "xyes"; then
    AC_DEFINE(HAS_BUILTIN_SYNC_SUB_AND_FETCH, 1,
        [Define to 1 if your compiler supports the __sync_sub_and_fetch() intrinsic.])
fi

AC_MSG_CHECKING([for __sync_val_compare_and_swap(temp, 1, 1)])
AC_TRY_COMPILE([],[long *temp = 0; __sync_val_compare_and_swap(temp, 1, 1);],
                [have_builtin_sync_val_compare_and_swap=yes],
                [have_builtin_sync_val_compare_and_swap=no])
AC_MSG_RESULT($have_builtin_sync_val_compare_and_swap)
if test "x$have_builtin_sync_val_compare_and_swap" = "xyes"; then
    AC_DEFINE(HAS_BUILTIN_SYNC_VAL_COMPARE_AND_SWAP, 1,
        [Define to 1 if your compiler supports the __sync_val_compare_and_swap() intrinsic.])
fi



HOST_CXXFLAGS="-Wall -Wextra -Wno-missing-field-initializers -Woverloaded-virtual -Wno-parentheses -fPIC $HOST_CXXFLAGS"

AC_SUBST(ARCHITECTURE)
AC_SUBST(BUILD_TYPE)
AC_SUBST(HOST_CXXFLAGS)
AC_SUBST(HOST_INCLUDES)
AC_SUBST(OS)
AC_SUBST(ARCH_DEFINES)
AC_SUBST(DYN_LIB_EXT)
AC_SUBST(TARGET_LDFLAGS)
AC_SUBST(BINPREFIX)
AC_SUBST(BIN_EXT)

if test "x$host_os_osx" = "xyes"; then
  AM_CONDITIONAL(HOST_IS_OSX, true)
else
  AM_CONDITIONAL(HOST_IS_OSX, false)
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

AC_CONFIG_FILES([Makefile \
                 lib/Makefile \
                 lib/jsoncpp/Makefile \
                 lib/libhts/Makefile \
                 lib/tinyxml/Makefile \
                 lib/cmyth/Makefile \
                 lib/cmyth/libcmyth/Makefile \
                 lib/cmyth/librefmem/Makefile \
                 lib/tinyxml2/Makefile \
                 lib/libdvblinkremote/Makefile \
                 addons/Makefile \
                 addons/pvr.demo/Makefile \
                 addons/pvr.dvbviewer/Makefile \
                 addons/pvr.argustv/Makefile \
                 addons/pvr.hts/Makefile \
                 addons/pvr.mediaportal.tvserver/Makefile \
                 addons/pvr.nextpvr/Makefile \
                 addons/pvr.njoy/Makefile \
                 addons/pvr.vuplus/Makefile \
                 addons/pvr.vdr.vnsi/Makefile \
                 addons/pvr.mythtv.cmyth/Makefile \
                 addons/pvr.dvblink/Makefile \
                 addons/pvr.wmc/Makefile \
                 addons/pvr.iptvsimple/Makefile \
                 addons/pvr.argustv/addon/addon.xml
                 addons/pvr.demo/addon/addon.xml
                 addons/pvr.dvbviewer/addon/addon.xml
                 addons/pvr.hts/addon/addon.xml
                 addons/pvr.mediaportal.tvserver/addon/addon.xml
                 addons/pvr.mythtv.cmyth/addon/addon.xml
                 addons/pvr.nextpvr/addon/addon.xml
                 addons/pvr.njoy/addon/addon.xml
                 addons/pvr.vdr.vnsi/addon/addon.xml
                 addons/pvr.vuplus/addon/addon.xml
                 addons/pvr.dvblink/addon/addon.xml
                 addons/pvr.wmc/addon/addon.xml
                 addons/pvr.iptvsimple/addon/addon.xml])

AC_OUTPUT
