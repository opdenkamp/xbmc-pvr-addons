#
# Makefile for the Njoy N7 add-on for XBMC PVR
#
# See the README for copyright information and
# how to reach the author.
#

ADDONBINNAME    = XBMC_Njoy
ADDONNAME       = pvr.njoy
LIBNAME         = libpvrnjoy-addon
lib_LTLIBRARIES = libpvrnjoy-addon.la

LIBS            = @abs_top_srcdir@/lib/tinyxml/libtinyxml.la

include ../Makefile.include.am

libpvrnjoy_addon_la_SOURCES = src/client.cpp \
                              src/N7Xml.cpp
libpvrnjoy_addon_la_LDFLAGS = @TARGET_LDFLAGS@

