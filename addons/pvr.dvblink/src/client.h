/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://xbmc.org
 
 *      Copyright (C) 2012 Palle Ehmsen(Barcode Madness)
 *      http://www.barcodemadness.com
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
 
#pragma once
#include "libXBMC_addon.h"
#include "libXBMC_pvr.h"
#include "libXBMC_gui.h"

extern bool                          m_bCreated;

enum DVBLINK_STREAMTYPE {HTTP=0,RTP=1,HLS =2, ASF=3};

/*  Client Settings default values */
#define DEFAULT_HOST                "127.0.0.1"
#define DEFAULT_PORT                8080
#define DEFAULT_TIMEOUT             10
#define DEFAULT_STREAMTYPE          HTTP
#define DEFAULT_CLIENTNAME          "xbmc"
#define DEFAULT_USERNAME            ""
#define DEFAULT_PASSWORD            ""
#define DEFAULT_USECHLHANDLE        true
#define DEFAULT_SHOWINFOMSG         false
#define DEFAULT_HEIGHT              720
#define DEFAULT_WIDTH               576
#define DEFAULT_BITRATE             512
#define DEFAULT_AUDIOTRACK          "eng"
#define DEFAULT_TIMESHIFTBUFFERPATH "special://userdata/addon_data/pvr.dvblink/"
#define DEFAULT_USETIMESHIFT        false

/* Client Settings */
extern std::string  g_szClientname;
extern std::string  g_szHostname;
extern long         g_lPort;
extern int          g_iConnectTimeout;
extern DVBLINK_STREAMTYPE  g_szStreamType;
extern std::string  g_szUsername;
extern std::string  g_szPassword;
extern int   g_iHeight;
extern int   g_iWidth;
extern int   g_iBitrate;
extern std::string  g_szAudiotrack;
extern bool g_bUseChlHandle;
extern bool g_bShowInfoMSG;
extern bool g_bUseTimeshift;
extern std::string  g_szTimeShiftBufferPath;



extern ADDON::CHelper_libXBMC_addon *XBMC;
extern CHelper_libXBMC_pvr          *PVR;

