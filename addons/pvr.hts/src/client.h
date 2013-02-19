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

#include "platform/os.h"
#include "libXBMC_addon.h"
#include "libXBMC_pvr.h"
#include "libXBMC_gui.h"
#include "avcodec.h"

#define DEFAULT_HOST             "127.0.0.1"
#define DEFAULT_HTTP_PORT        9981
#define DEFAULT_HTSP_PORT        9982
#define DEFAULT_CONNECT_TIMEOUT  6
#define DEFAULT_RESPONSE_TIMEOUT 4
#define DEFAULT_VIDEO_CODEC      CODEC_ID_H264
#define DEFAULT_AUDIO_CODEC      CODEC_ID_NONE
#define DEFAULT_RESOLUTION       480
#define DEFAULT_TRANSCODE        false
#define HTSP_DEBUGGING           0

extern bool                      m_bCreated;
extern std::string               g_strHostname;
extern int                       g_iPortHTSP;
extern int                       g_iPortHTTP;
extern std::string               g_strUsername;
extern std::string               g_strPassword;
extern int                       g_iConnectTimeout;
extern int                       g_iResponseTimeout;
extern bool                      g_bShowTimerNotifications;
extern std::string               g_szUserPath;
extern std::string               g_szClientPath;
extern bool                      g_bTranscode;
extern CodecID                   g_iAudioCodec;
extern CodecID                   g_iVideoCodec;
extern int                       g_iResolution;

extern ADDON::CHelper_libXBMC_addon *   XBMC;
extern CHelper_libXBMC_pvr *     PVR;
extern CHelper_libXBMC_gui *     GUI;

uint32_t HTSPNextSequenceNumber(void);
