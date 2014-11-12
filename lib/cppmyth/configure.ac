AC_PREREQ([2.60])
AC_INIT([cppmyth], [1.0], [jlbarriere68@gmail.com])

AM_INIT_AUTOMAKE([1.10 foreign])

AC_CONFIG_SRCDIR([cppmyth/src/mythtypes.cpp])
AC_CONFIG_HEADERS([config.h])

# Configure external packages.
AC_CONFIG_SUBDIRS([jansson])

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_CPP
AC_PROG_LIBTOOL
AM_CONDITIONAL([GCC], [test x$GCC = xyes])

# Checks for pthread.
AC_SEARCH_LIBS([pthread_create],[pthread],, AC_MSG_ERROR([required library 'pthread' is missing]))

# Checks for rt.
AC_SEARCH_LIBS(clock_gettime, [rt])

# Check for inttypes.
AC_CHECK_HEADERS([inttypes.h],, AC_MSG_ERROR([required header 'inttypes.h' is missing]))

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
case $ac_cv_c_inline in
    yes) CC_INLINE=inline;;
    no) CC_INLINE=;;
    *) CC_INLINE=$ac_cv_c_inline;;
esac
AC_SUBST([CC_INLINE])

# Checks for library functions.
AC_CHECK_FUNCS([timegm localtime_r gmtime_r])

case "$ac_cv_func_timegm" in
     yes) HAVE_TIMEGM=1;;
     *) HAVE_TIMEGM=0;;
esac
AC_SUBST([HAVE_TIMEGM])

case "$ac_cv_func_localtime_r" in
     yes) HAVE_LOCALTIME_R=1;;
     *) HAVE_LOCATIME_R=0;;
esac
AC_SUBST([HAVE_LOCALTIME_R])

case "$ac_cv_func_gmtime_r" in
     yes) HAVE_GMTIME_R=1;;
     *) HAVE_GMTIME_R=0;;
esac
AC_SUBST([HAVE_GMTIME_R])

AC_CONFIG_FILES([
        Makefile
        cppmyth/cppmyth.pc
        cppmyth/Makefile
        cppmyth/src/cppmyth_config.h
])

AC_OUTPUT
