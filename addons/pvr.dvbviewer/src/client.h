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
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
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

#define DEFAULT_HOST             "127.0.0.1"
#define DEFAULT_CONNECT_TIMEOUT  30
#define DEFAULT_WEB_PORT         8089
#define DEFAULT_TSBUFFERPATH     "special://userdata/addon_data/pvr.dvbviewer"

extern CStdString    g_strHostname;
extern int           g_iPortWeb;
extern CStdString    g_strUsername;
extern CStdString    g_strPassword;
extern bool          g_useFavourites;
extern bool          g_useFavouritesFile;
extern CStdString    g_favouritesFile;
extern bool          g_bUseTimeshift;
extern CStdString    g_strTimeshiftBufferPath;
extern bool          g_useRTSP;
extern ADDON::CHelper_libXBMC_addon *XBMC;
extern CHelper_libXBMC_pvr *PVR;

#endif
