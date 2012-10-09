#
# Makefile for the VDR VNSI add-on for XBMC PVR
#
# See the README for copyright information and
# how to reach the author.
#

ADDONBINNAME    = XBMC_VDR_vnsi
ADDONNAME       = pvr.vdr.vnsi
LIBNAME         = libvdrvnsi-addon
lib_LTLIBRARIES = libvdrvnsi-addon.la

include ../Makefile.include.am

libvdrvnsi_addon_la_SOURCES = src/client.cpp \
                              src/VNSIChannelScan.cpp \
                              src/VNSIData.cpp \
                              src/VNSIDemux.cpp \
                              src/VNSIRecording.cpp \
                              src/VNSISession.cpp \
                              src/requestpacket.cpp \
                              src/responsepacket.cpp \
                              src/tools.cpp
libvdrvnsi_addon_la_LDFLAGS = @TARGET_LDFLAGS@
