#
# Makefile for the Pctv add-on for XBMC PVR
#
# See the README for copyright information and
# how to reach the author.
#

ADDONBINNAME    = Kodi_pctv
ADDONNAME       = pvr.pctv
LIBNAME         = libpctv-addon
lib_LTLIBRARIES = libpctv-addon.la

LIBS            = @abs_top_srcdir@/lib/jsoncpp/libjsoncpp.la

include ../Makefile.include.am

INCLUDES+=-I@abs_top_srcdir@/lib/jsoncpp/include

libpctv_addon_la_SOURCES = src/client.cpp \
                              src/PctvData.cpp \
                              src/rest.cpp \
                              src/md5.cpp
libpctv_addon_la_LDFLAGS = @TARGET_LDFLAGS@

