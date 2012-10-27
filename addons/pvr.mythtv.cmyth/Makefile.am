#
# Makefile for the MythTV (using libcmyth) add-on for XBMC PVR
#
# See the README for copyright information and
# how to reach the author.
#

ADDONBINNAME    = XBMC_MythTV_cmyth
ADDONNAME       = pvr.mythtv.cmyth
LIBNAME         = libmythtvcmyth-addon
lib_LTLIBRARIES = libmythtvcmyth-addon.la

LIBS            = @abs_top_srcdir@/lib/cmyth/libcmyth.la

AM_CPPFLAGS = -I$(abs_top_srcdir)/lib/cmyth/include

include ../Makefile.include.am

libmythtvcmyth_addon_la_SOURCES = src/client.cpp \
                                  src/fileOps.cpp \
                                  src/categories.cpp \
                                  src/pvrclient-mythtv.cpp \
                                  src/cppmyth/MythChannel.cpp \
                                  src/cppmyth/MythConnection.cpp \
                                  src/cppmyth/MythDatabase.cpp \
                                  src/cppmyth/MythEventHandler.cpp \
                                  src/cppmyth/MythFile.cpp \
                                  src/cppmyth/MythProgramInfo.cpp \
                                  src/cppmyth/MythRecorder.cpp \
                                  src/cppmyth/MythStorageGroupFile.cpp \
                                  src/cppmyth/MythSignal.cpp \
                                  src/cppmyth/MythTimer.cpp \
                                  src/cppmyth/MythTimestamp.cpp

