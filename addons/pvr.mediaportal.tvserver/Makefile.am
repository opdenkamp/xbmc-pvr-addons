#
# Makefile for the Mediaportal add-on for XBMC PVR
#
# See the README for copyright information and
# how to reach the author.
#

ADDONBINNAME    = XBMC_Mediaportal
ADDONNAME       = pvr.mediaportal.tvserver
LIBNAME         = libmediaportal-addon
lib_LTLIBRARIES = libmediaportal-addon.la

LIBS            = @abs_top_srcdir@/lib/tinyxml/libtinyxml.la

include ../Makefile.include.am

INCLUDES+=-Isrc

libmediaportal_addon_la_SOURCES = src/channels.cpp \
                                  src/client.cpp \
                                  src/epg.cpp \
                                  src/GenreTable.cpp \
                                  src/pvrclient-mediaportal.cpp \
                                  src/recordings.cpp \
                                  src/Socket.cpp \
                                  src/timers.cpp \
                                  src/uri.cpp \
                                  ../shared/misc/utils.cpp \
                                  ../shared/tsreader/Cards.cpp \
                                  ../shared/tsreader/FileReader.cpp \
                                  ../shared/tsreader/MultiFileReader.cpp \
                                  ../shared/tsreader/TSReader.cpp

