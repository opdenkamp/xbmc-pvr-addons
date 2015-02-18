#
# Makefile for the PVR Filmon add-on for XBMC PVR
#
# See the README for copyright information and
# how to reach the author.
#

ADDONBINNAME    = Kodi_Filmon
ADDONNAME       = pvr.filmon
LIBNAME         = libpvrfilmon-addon
lib_LTLIBRARIES = libpvrfilmon-addon.la

LIBS            = @abs_top_srcdir@/lib/jsoncpp/libjsoncpp.la

include ../Makefile.include.am

AM_CPPFLAGS+=-I@abs_top_srcdir@/lib/jsoncpp/include

libpvrfilmon_addon_la_SOURCES = src/client.cpp \
                              src/PVRFilmonData.cpp \
                              src/FilmonAPI.cpp \
                              src/md5.cpp
libpvrfilmon_addon_la_LDFLAGS = @TARGET_LDFLAGS@
