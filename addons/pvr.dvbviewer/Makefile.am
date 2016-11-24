#
# Makefile for the DVBViewer add-on for XBMC PVR
#
# See the README for copyright information and
# how to reach the author.
#

ADDONBINNAME    = Kodi_dvbviewer
ADDONNAME       = pvr.dvbviewer
LIBNAME         = libdvbviewer-addon
lib_LTLIBRARIES = libdvbviewer-addon.la

LIBS            = @abs_top_builddir@/lib/tinyxml/libtinyxml.la

include ../Makefile.include.am

libdvbviewer_addon_la_SOURCES = src/client.cpp \
                                src/DvbData.cpp \
                                src/TimeshiftBuffer.cpp
libdvbviewer_addon_la_LDFLAGS = @TARGET_LDFLAGS@

