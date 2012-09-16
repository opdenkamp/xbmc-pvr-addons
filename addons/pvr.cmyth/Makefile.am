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

LIBS            = -lboost_system -lboost_filesystem -lboost_regex -ldl -lmysqlclient
AM_CPPFLAGS     = -I$(abs_srcdir)/src/cmyth/libcmyth -I$(abs_srcdir)/src/cmyth/librefmem -I$(abs_srcdir)/src/cmyth/include

include ../Makefile.include.am

libcmyth_addon_la_SOURCES = \
src/cmyth/libcmyth/bookmark.c \
src/cmyth/libcmyth/commbreak.c \
src/cmyth/libcmyth/connection.c \
src/cmyth/libcmyth/debug.c \
src/cmyth/libcmyth/event.c \
src/cmyth/libcmyth/file.c \
src/cmyth/libcmyth/freespace.c \
src/cmyth/libcmyth/input.c \
src/cmyth/libcmyth/keyframe.c \
src/cmyth/libcmyth/livetv.c \
src/cmyth/libcmyth/mysql_query.c \
src/cmyth/libcmyth/mythtv_mysql.c \
src/cmyth/libcmyth/posmap.c \
src/cmyth/libcmyth/proginfo.c \
src/cmyth/libcmyth/proglist.c \
src/cmyth/libcmyth/rec_num.c \
src/cmyth/libcmyth/recorder.c \
src/cmyth/libcmyth/ringbuf.c \
src/cmyth/libcmyth/socket.c \
src/cmyth/libcmyth/timestamp.c \
src/cmyth/libcmyth/utf8tolatin1.c \
src/cmyth/librefmem/alloc.c \
src/cmyth/librefmem/debug_refmem.c \
                                  src/client.cpp \
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


