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

#ifndef CLIENT_H
#define CLIENT_H

#include "platform/util/StdString.h"
#include "libXBMC_addon.h"
#include "libXBMC_pvr.h"

enum eStreamingMethod
{
  TSReader = 0,
  ffmpeg = 1
};

#define DEFAULT_HOST                  "127.0.0.1"
#define DEFAULT_PORT                  8866
#define DEFAULT_PIN                   "0000"
#define DEFAULT_FTA_ONLY              false
#define DEFAULT_RADIO                 true
#define DEFAULT_TIMEOUT               10
#define DEFAULT_HANDLE_MSG            false
#define DEFAULT_RESOLVE_RTSP_HOSTNAME false
#define DEFAULT_READ_GENRE            false
#define DEFAULT_SLEEP_RTSP_URL        0
#define DEFAULT_USE_REC_DIR           false
#define DEFAULT_TVGROUP               ""
#define DEFAULT_RADIOGROUP            ""
#define DEFAULT_DIRECT_TS_FR          false
#define DEFAULT_SMBUSERNAME           "Guest"
#define DEFAULT_SMBPASSWORD           ""

extern std::string      g_szUserPath;         ///< The Path to the user directory inside user profile
extern std::string      g_szClientPath;       ///< The Path where this driver is located

/* Client Settings */
extern std::string      g_szHostname;
extern int              g_iPort;
extern std::string      g_szPin;
extern int              g_iConnectTimeout;
extern int              g_iSleepOnRTSPurl;
extern bool             g_bOnlyFTA;
extern bool             g_bRadioEnabled;
extern bool             g_bHandleMessages;
extern bool             g_bResolveRTSPHostname;
extern bool             g_bReadGenre;
extern bool             g_bFastChannelSwitch;
extern bool             g_bUseRTSP;           ///< Use RTSP streaming when using the tsreader
extern std::string      g_szTVGroup;
extern std::string      g_szRadioGroup;
extern std::string      g_szSMBusername;
extern std::string      g_szSMBpassword;
extern eStreamingMethod g_eStreamingMethod;

extern ADDON::CHelper_libXBMC_addon *XBMC;
extern CHelper_libXBMC_pvr          *PVR;

extern int              g_iTVServerXBMCBuild;

/*!
 * @brief PVR macros for string exchange
 */
#define PVR_STRCPY(dest, source) do { strncpy(dest, source, sizeof(dest)-1); dest[sizeof(dest)-1] = '\0'; } while(0)
#define PVR_STRCLR(dest) memset(dest, 0, sizeof(dest))

#endif /* CLIENT_H */
