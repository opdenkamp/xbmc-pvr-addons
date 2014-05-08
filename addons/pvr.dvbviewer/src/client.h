#pragma once
/*
 *      Copyright (C) 2005-2011 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301  USA
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifndef PVR_DVBVIEWER_CLIENT_H
#define PVR_DVBVIEWER_CLIENT_H

#include "libXBMC_addon.h"
#include "libXBMC_pvr.h"
#include "libXBMC_gui.h"
#include "platform/util/StdString.h"

#ifndef _UNUSED
#if defined(__GNUC__)
# define _UNUSED(x) UNUSED_ ## x __attribute__((unused))
#elif defined(__LCLINT__)
# define _UNUSED(x) /*@unused@*/ x
#else
# define _UNUSED(x) x
#endif
#endif

#define STR(x)  #x
#define XSTR(x) STR(x)

/*!
 * @brief PVR macros for string exchange
 */
#define PVR_STRCPY(dest, source) do { strncpy(dest, source, sizeof(dest)-1); dest[sizeof(dest)-1] = '\0'; } while(0)
#define PVR_STRCLR(dest) memset(dest, 0, sizeof(dest))

/* indicate that caller can handle truncated reads, where function returns before entire buffer has been filled */
#define READ_TRUNCATED 0x01
/* indicate that that caller support read in the minimum defined chunk size, this disables internal cache then */
#define READ_CHUNKED 0x02
/* use cache to access this file */
#define READ_CACHED 0x04
/* open without caching. regardless to file type. */
#define READ_NO_CACHE 0x08
/* calcuate bitrate for file while reading */
#define READ_BITRATE 0x10

#define DEFAULT_HOST             "127.0.0.1"
#define DEFAULT_WEB_PORT         8089
#define DEFAULT_TSBUFFERPATH     "special://userdata/addon_data/pvr.dvbviewer"

extern CStdString    g_hostname;
extern int           g_webPort;
extern CStdString    g_username;
extern CStdString    g_password;
extern bool          g_useFavourites;
extern bool          g_useFavouritesFile;
extern CStdString    g_favouritesFile;
extern int           g_groupRecordings;
extern bool          g_useTimeshift;
extern CStdString    g_timeshiftBufferPath;
extern bool          g_useRTSP;
extern bool          g_lowPerformance;

extern ADDON::CHelper_libXBMC_addon *XBMC;
extern CHelper_libXBMC_pvr *PVR;

#endif
