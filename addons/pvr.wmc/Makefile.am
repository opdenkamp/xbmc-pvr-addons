#
# Makefile for the PVR Windows Media Center add-on for XBMC PVR
#
# See the README for copyright information and
# how to reach the author.
#

ADDONBINNAME    = XBMC_WMC
ADDONNAME       = pvr.wmc
LIBNAME         = libpvrwmc-addon
lib_LTLIBRARIES = libpvrwmc-addon.la

LIBS            = @abs_top_srcdir@/lib/tinyxml/libtinyxml.la

include ../Makefile.include.am

libpvrwmc_addon_la_SOURCES = src/DialogDeleteTimer.cpp \
                              src/DialogRecordPref.cpp \
                              src/client.cpp \
                              src/pvr2wmc.cpp \
                              src/Socket.cpp \
                              src/utilities.cpp
libpvrwmc_addon_la_LDFLAGS = @TARGET_LDFLAGS@

