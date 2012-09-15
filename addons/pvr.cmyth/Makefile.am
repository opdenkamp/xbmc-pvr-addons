#
# Makefile for the cmyth add-on for XBMC PVR
#
# See the README for copyright information and
# how to reach the author.
#

ADDONBINNAME    = XBMC_cmyth
ADDONNAME       = pvr.cmyth
LIBNAME         = libcmyth-addon
lib_LTLIBRARIES = libcmyth-addon.la

LIBS            = -lboost_system -lboost_filesystem -lboost_regex -ldl

include ../Makefile.include.am

libcmyth_addon_la_SOURCES = src/client.cpp \
                                  src/cppmyth/MythChannel.cpp \
                                  src/cppmyth/MythConnection.cpp \
                                  src/cppmyth/MythDatabase.cpp \
                                  src/cppmyth/MythEventHandler.cpp \
                                  src/cppmyth/MythFile.cpp \
                                  src/cppmyth/MythProgramInfo.cpp \
                                  src/cppmyth/MythRecorder.cpp \
                                  src/cppmyth/MythSignal.cpp \
                                  src/cppmyth/MythTimer.cpp \
                                  src/cppmyth/MythTimestamp.cpp \
                                  src/cppmyth/MythSGFile.cpp \
                                  src/fileOps.cpp \
                                  src/PVRcmyth.cpp


