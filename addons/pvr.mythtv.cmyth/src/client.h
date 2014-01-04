#pragma once
/*
 *      Copyright (C) 2005-2012 Team XBMC
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

#include <platform/util/StdString.h>
#include <libXBMC_addon.h>
#include <libXBMC_pvr.h>
#include <libXBMC_gui.h>
#include <libXBMC_codec.h>

extern "C" {
#include <cmyth/cmyth.h>
};

#ifdef __WINDOWS__
#define strdup _strdup // # strdup is POSIX, _strdup should be used instead

static inline struct tm *localtime_r(const time_t * clock, struct tm *result)
{
	struct tm *data;
	if (!clock || !result)
		return NULL;
	data = localtime(clock);
	if (!data)
		return NULL;
	memcpy(result, data, sizeof(*result));
	return result;
}
#endif

#define TCP_RCV_BUF_CONTROL_SIZE           128000 // Inherited from MythTV's MythSocket class
#define RCV_BUF_CONTROL_SIZE               32000  // Buffer size to parse backend response from control connection
#define TCP_RCV_BUF_DATA_SIZE              65536  // TCP buffer for video stream (best performance)
#define RCV_BUF_DATA_SIZE                  64     // Buffer size to parse backend response from data control connection
#define RCV_BUF_IMAGE_SIZE                 32000  // Buffer size to download artworks

#define LIVETV_CONFLICT_STRATEGY_HASLATER  0
#define LIVETV_CONFLICT_STRATEGY_STOPTV    1
#define LIVETV_CONFLICT_STRATEGY_CANCELREC 2

#define DEFAULT_HOST                       "127.0.0.1"
#define DEFAULT_EXTRA_DEBUG                false
#define DEFAULT_LIVETV_PRIORITY            true
#define DEFAULT_LIVETV_CONFLICT_STRATEGY   LIVETV_CONFLICT_STRATEGY_HASLATER
#define DEFAULT_LIVETV                     true
#define DEFAULT_PORT                       6543
#define DEFAULT_DB_USER                    "mythtv"
#define DEFAULT_DB_PASSWORD                "mythtv"
#define DEFAULT_DB_NAME                    "mythconverg"
#define DEFAULT_DB_PORT                    3306
#define DEFAULT_RECORD_TEMPLATE            1

#define SUBTITLE_SEPARATOR " - "

#define MENUHOOK_REC_DELETE_AND_RERECORD   1
#define MENUHOOK_KEEP_LIVETV_RECORDING     2
#define MENUHOOK_SHOW_HIDE_NOT_RECORDING   3
#define MENUHOOK_EPG_REC_CHAN_ALL_SHOWINGS 4
#define MENUHOOK_EPG_REC_CHAN_WEEKLY       5
#define MENUHOOK_EPG_REC_CHAN_DAILY        6
#define MENUHOOK_EPG_REC_ONE_SHOWING       7
#define MENUHOOK_EPG_REC_NEW_EPISODES      8

#define DEFAULT_HANDLE_DEMUXING            false

/*!
 * @brief PVR macros for string exchange
 */
#define PVR_STRCPY(dest, source) do { strncpy(dest, source, sizeof(dest)-1); dest[sizeof(dest)-1] = '\0'; } while(0)
#define PVR_STRCLR(dest) memset(dest, 0, sizeof(dest))

/** Delete macros that make the pointer NULL again */
#define SAFE_DELETE(p)       do { delete (p);     (p)=NULL; } while (0)
#define SAFE_DELETE_ARRAY(p) do { delete[] (p);   (p)=NULL; } while (0)

extern bool         g_bCreated;                ///< Shows that the Create function was successfully called
extern int          g_iClientID;               ///< The PVR client ID used by XBMC for this driver
extern CStdString   g_szUserPath;              ///< The Path to the user directory inside user profile
extern CStdString   g_szClientPath;            ///< The Path where this driver is located

/* Client Settings */
extern CStdString   g_szMythHostname;          ///< The Host name or IP of the mythtv server
extern CStdString   g_szMythHostEther;         ///< The Host MAC address of the mythtv server
extern int          g_iMythPort;               ///< The mythtv Port (default is 6543)
extern CStdString   g_szDBUser;                ///< The mythtv sql username (default is mythtv)
extern CStdString   g_szDBPassword;            ///< The mythtv sql password (default is mythtv)
extern CStdString   g_szDBName;                ///< The mythtv sql database name (default is mythconverg)
extern CStdString   g_szDBHostname;            ///< The mythtv sql database host name or IP of the database server
extern int          g_iDBPort;                 ///< The mythtv sql database port (default is 3306)
extern bool         g_bExtraDebug;             ///< Debug logging
extern bool         g_bLiveTV;                 ///< LiveTV support (or recordings only)
extern bool         g_bLiveTVPriority;         ///< MythTV Backend setting to allow live TV to move scheduled shows
extern int          g_iLiveTVConflictStrategy; ///< Live TV conflict resolving strategy (0=Has later, 1=Stop TV, 2=Cancel recording)
extern int          g_iRecTemplateType;        ///< Template type for new record (0=Internal, 1=MythTV)
///* Internal Record template */
extern bool         g_bRecAutoMetadata;
extern bool         g_bRecAutoCommFlag;
extern bool         g_bRecAutoTranscode;
extern bool         g_bRecAutoRunJob1;
extern bool         g_bRecAutoRunJob2;
extern bool         g_bRecAutoRunJob3;
extern bool         g_bRecAutoRunJob4;
extern bool         g_bRecAutoExpire;
extern int          g_iRecTranscoder;
extern bool         g_bDemuxing;

extern ADDON::CHelper_libXBMC_addon *XBMC;
extern CHelper_libXBMC_pvr   *PVR;
extern CHelper_libXBMC_gui   *GUI;
extern CHelper_libXBMC_codec *CODEC;

#endif /* CLIENT_H */
