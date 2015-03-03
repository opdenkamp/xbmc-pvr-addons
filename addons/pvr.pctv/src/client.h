#pragma once
/*
*      Copyright (C) 2005-2015 Team XBMC
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

#include "libXBMC_addon.h"
#include "libXBMC_pvr.h"
#include "libXBMC_gui.h"

#define DEFAULT_HOST              "192.168.1.20"
#define DEFAULT_WEB_PORT          80
#define DEFAULT_PIN               "0000"
#define DEFAULT_AUTH              ""
#define DEFAULT_TRANSCODE         false 
#define DEFAULT_USEPIN		      false 
#define DEFAULT_BITRATE           1200

extern bool                          m_bCreated;
extern std::string                   g_strHostname;
extern int                           g_iPortWeb;
extern std::string                   g_strPin;
extern std::string                   g_strAuth;
//extern std::string                   g_strBaseUrl;
extern int                           g_iStartNumber;
extern std::string                   g_strUserPath;
extern std::string                   g_strClientPath;
extern bool                          g_bTranscode;
extern bool                          g_bUsePIN;
extern int                           g_iBitrate;

extern ADDON::CHelper_libXBMC_addon *XBMC;
extern CHelper_libXBMC_pvr          *PVR;

extern std::string PathCombine(const std::string &strPath, const std::string &strFileName);
extern std::string GetClientFilePath(const std::string &strFileName);
extern std::string GetUserFilePath(const std::string &strFileName);