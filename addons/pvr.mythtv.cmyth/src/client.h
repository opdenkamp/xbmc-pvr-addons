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

extern "C" {
#include <cmyth/cmyth.h>
};

#ifdef __WINDOWS__
#define strdup _strdup // # strdup is POSIX, _strdup should be used instead
#endif

#define DEFAULT_HOST            "127.0.0.1"
#define DEFAULT_EXTRA_DEBUG     false
#define DEFAULT_LIVETV_PRIORITY false
#define DEFAULT_LIVETV          true
#define DEFAULT_PORT            6543
#define DEFAULT_DB_USER         "mythtv"
#define DEFAULT_DB_PASSWORD     "mythtv"
#define DEFAULT_DB_NAME         "mythconverg"

/*!
 * @brief PVR macros for string exchange
 */
#define PVR_STRCPY(dest, source) do { strncpy(dest, source, sizeof(dest)-1); dest[sizeof(dest)-1] = '\0'; } while(0)
#define PVR_STRCLR(dest) memset(dest, 0, sizeof(dest))

/** Delete macros that make the pointer NULL again */
#define SAFE_DELETE(p)       do { delete (p);     (p)=NULL; } while (0)
#define SAFE_DELETE_ARRAY(p) do { delete[] (p);   (p)=NULL; } while (0)

extern bool         g_bCreated;           ///< Shows that the Create function was successfully called
extern int          g_iClientID;          ///< The PVR client ID used by XBMC for this driver
extern CStdString   g_szUserPath;         ///< The Path to the user directory inside user profile
extern CStdString   g_szClientPath;       ///< The Path where this driver is located

/* Client Settings */
extern CStdString   g_szHostname;         ///< The Host name or IP of the mythtv server
extern int          g_iMythPort;          ///< The mythtv Port (default is 6543)
extern CStdString   g_szMythDBuser;       ///< The mythtv sql username (default is mythtv)
extern CStdString   g_szMythDBpassword;   ///< The mythtv sql password (default is mythtv)
extern CStdString   g_szMythDBname;       ///< The mythtv sql database name (default is mythconverg)
extern bool         g_bExtraDebug;        ///< Debug logging
extern bool         g_bLiveTV;            ///< LiveTV support (or recordings only)
extern bool         g_bLiveTVPriority;    ///< MythTV Backend setting to allow live TV to move scheduled shows

extern ADDON::CHelper_libXBMC_addon *XBMC;
extern CHelper_libXBMC_pvr   *PVR;
extern CHelper_libXBMC_gui   *GUI;

#endif /* CLIENT_H */
