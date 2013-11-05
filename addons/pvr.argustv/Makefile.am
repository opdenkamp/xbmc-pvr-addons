#
# Makefile for the ARGUS TV add-on for XBMC PVR
#
# See the README for copyright information and
# how to reach the author.
#

ADDONBINNAME    = XBMC_ArgusTV
ADDONNAME       = pvr.argustv
LIBNAME         = libargustv-addon
lib_LTLIBRARIES = libargustv-addon.la

LIBS            = @abs_top_srcdir@/lib/jsoncpp/libjsoncpp.la

include ../Makefile.include.am

INCLUDES+=-Isrc -Isrc/lib/filesystem -I@abs_top_srcdir@/lib/jsoncpp/include

libargustv_addon_la_SOURCES = src/activerecording.cpp \
                                   src/channel.cpp \
                                   src/client.cpp \
                                   src/epg.cpp \
                                   src/argustvrpc.cpp \
                                   src/guideprogram.cpp \
                                   src/KeepAliveThread.cpp \
                                   src/pvrclient-argustv.cpp \
                                   src/recording.cpp \
                                   src/recordinggroup.cpp \
                                   src/tools.cpp \
                                   src/upcomingrecording.cpp \
                                   src/uri.cpp \
                                   src/utils.cpp \
                                   src/lib/tsreader/FileReader.cpp \
                                   src/lib/tsreader/MultiFileReader.cpp \
                                   src/lib/tsreader/TSReader.cpp
libargustv_addon_la_LDFLAGS = @TARGET_LDFLAGS@

