#
# Makefile for the ForTheRecord Argus add-on for XBMC PVR
#
# See the README for copyright information and
# how to reach the author.
#

ADDONBINNAME    = XBMC_ForTheRecord
ADDONNAME       = pvr.fortherecord.argus
LIBNAME         = libfortherecord-addon
lib_LTLIBRARIES = libfortherecord-addon.la

LIBS            = @abs_top_srcdir@/lib/jsoncpp/libjsoncpp.la

include ../Makefile.include.am

INCLUDES+=-Isrc -Isrc/lib/filesystem -I@abs_top_srcdir@/lib/jsoncpp/include
DEFINES+=-DTSREADER

libfortherecord_addon_la_SOURCES = src/activerecording.cpp \
                                   src/channel.cpp \
                                   src/client.cpp \
                                   src/epg.cpp \
                                   src/fortherecordrpc.cpp \
                                   src/guideprogram.cpp \
                                   src/KeepAliveThread.cpp \
                                   src/pvrclient-fortherecord.cpp \
                                   src/recording.cpp \
                                   src/recordinggroup.cpp \
                                   src/recordingsummary.cpp \
                                   src/tools.cpp \
                                   src/upcomingrecording.cpp \
                                   src/uri.cpp \
                                   src/utils.cpp \
                                   src/lib/tsreader/FileReader.cpp \
                                   src/lib/tsreader/MultiFileReader.cpp \
                                   src/lib/tsreader/TSReader.cpp
libfortherecord_addon_la_LDFLAGS = @TARGET_LDFLAGS@

