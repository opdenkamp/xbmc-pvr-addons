#
# Makefile for the PVR demo add-on for XBMC PVR
#
# See the README for copyright information and
# how to reach the author.
#

ADDONBINNAME    = XBMC_Demo
ADDONNAME       = pvr.demo
LIBNAME         = libpvrdemo-addon
lib_LTLIBRARIES = libpvrdemo-addon.la

LIBS            = @abs_top_srcdir@/lib/tinyxml/libtinyxml.la

include ../Makefile.include.am

libpvrdemo_addon_la_SOURCES = src/client.cpp \
                              src/PVRDemoData.cpp
libpvrdemo_addon_la_LDFLAGS = @TARGET_LDFLAGS@

