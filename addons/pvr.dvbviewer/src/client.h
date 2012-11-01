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

#include "libXBMC_addon.h"
#include "libXBMC_pvr.h"

#define DEFAULT_HOST             "127.0.0.1"
#define DEFAULT_CONNECT_TIMEOUT  30
#define DEFAULT_WEB_PORT         8089
#define DEFAULT_STREAM_PORT      7522
#define DEFAULT_RECORDING_PORT   8090

extern bool                      m_bCreated;
extern std::string               g_strHostname;
extern int                       g_iPortStream;
extern int                       g_iPortWeb;
extern int                       g_iPortRecording;
extern std::string               g_strUsername;
extern std::string               g_strPassword;
extern bool                      g_bUseFavourites;
extern std::string               g_strFavouritesPath;
//extern int                       g_iClientId;
extern ADDON::CHelper_libXBMC_addon *   XBMC;
extern CHelper_libXBMC_pvr *     PVR;
