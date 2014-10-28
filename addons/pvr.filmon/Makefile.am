#
# Makefile for the PVR Filmon add-on for XBMC PVR
#
# See the README for copyright information and
# how to reach the author.
#

ADDONBINNAME    = XBMC_Filmon
ADDONNAME       = pvr.filmon
LIBNAME         = libpvrfilmon-addon
lib_LTLIBRARIES = libpvrfilmon-addon.la

include ../Makefile.include.am

libpvrfilmon_addon_la_SOURCES = src/client.cpp \
                              src/PVRFilmonData.cpp \
                              src/FilmonAPI.cpp
libpvrfilmon_addon_la_LDFLAGS = $(CURL_LIBS) $(JSONCPP_LIBS) $(CRYPTOPP_LIBS) @TARGET_LDFLAGS@
