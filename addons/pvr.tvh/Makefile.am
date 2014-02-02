#
# Makefile for the HTS Tvheadend add-on for XBMC PVR
#
# See the README for copyright information and
# how to reach the author.
#

ADDONBINNAME    = XBMC_pvr_tvh
ADDONNAME       = pvr.tvh
LIBNAME         = lib_pvr_tvh-addon
lib_LTLIBRARIES = lib_pvr_tvh-addon.la

LIBS            = @abs_top_srcdir@/lib/libhts/libhts.la -ldl

include ../Makefile.include.am

lib_pvr_tvh_addon_la_SOURCES = src/client.cpp \
                               src/Tvheadend.cpp \
                               src/HTSPConnection.cpp \
                               src/HTSPDemuxer.cpp \
                               src/HTSPVFS.cpp \
                               src/CircBuffer.cpp
lib_pvr_tvh_addon_la_LDFLAGS = @TARGET_LDFLAGS@

