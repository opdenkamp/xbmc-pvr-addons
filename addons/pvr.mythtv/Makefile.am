#
# Makefile for the MythTV (using libcppmyth) add-on for XBMC PVR
#
# See the README for copyright information and
# how to reach the author.
#

ADDONBINNAME    = Kodi_MythTV
ADDONNAME       = pvr.mythtv
LIBNAME         = libmythtv-addon
lib_LTLIBRARIES = libmythtv-addon.la

LIBS            = @abs_top_srcdir@/lib/cppmyth/cppmyth/libcppmyth.la

include ../Makefile.include.am

AM_CPPFLAGS    += -I@abs_top_srcdir@/lib/cppmyth/cppmyth/src

if HAVE_VERSION_SCRIPT
libmythtv_addon_la_LDFLAGS = -Wl,--version-script=src/addon.map \
                                  @TARGET_LDFLAGS@
else
libmythtv_addon_la_LDFLAGS = @TARGET_LDFLAGS@
endif

libmythtv_addon_la_SOURCES = src/client.cpp \
                                  src/fileOps.cpp \
                                  src/categories.cpp \
                                  src/pvrclient-mythtv.cpp \
                                  src/cppmyth/MythChannel.cpp \
                                  src/cppmyth/MythProgramInfo.cpp \
                                  src/cppmyth/MythRecordingRule.cpp \
                                  src/cppmyth/MythEPGInfo.cpp \
                                  src/cppmyth/MythScheduleManager.cpp \
                                  src/demux.cpp \
                                  src/demuxer/debug.cpp \
                                  src/demuxer/elementaryStream.cpp \
                                  src/demuxer/tsDemuxer.cpp \
                                  src/demuxer/bitstream.cpp \
                                  src/demuxer/ES_MPEGVideo.cpp \
                                  src/demuxer/ES_MPEGAudio.cpp \
                                  src/demuxer/ES_h264.cpp \
                                  src/demuxer/ES_AAC.cpp \
                                  src/demuxer/ES_AC3.cpp \
                                  src/demuxer/ES_Subtitle.cpp \
                                  src/demuxer/ES_Teletext.cpp \
                                  src/avinfo.cpp \
                                  src/guidialogbase.cpp \
                                  src/guidialogyesno.cpp
