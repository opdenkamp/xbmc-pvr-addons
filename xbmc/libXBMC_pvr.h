#pragma once
/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include <string>
#include <vector>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "xbmc_pvr_types.h"
#include "libXBMC_addon.h"

#ifdef _WIN32
#define PVR_HELPER_DLL "\\library.xbmc.pvr\\libXBMC_pvr" ADDON_HELPER_EXT
#else
#define PVR_HELPER_DLL "/library.xbmc.pvr/libXBMC_pvr-" ADDON_HELPER_ARCH "-" ADDON_HELPER_PLATFORM ADDON_HELPER_EXT
#endif

#define DVD_TIME_BASE 1000000
#define DVD_NOPTS_VALUE    (-1LL<<52) // should be possible to represent in both double and __int64

class CHelper_libXBMC_pvr
{
public:
  CHelper_libXBMC_pvr()
  {
    m_libXBMC_pvr = NULL;
    m_Handle      = NULL;
  }

  ~CHelper_libXBMC_pvr()
  {
    if (m_libXBMC_pvr)
    {
      PVR_unregister_me();
      dlclose(m_libXBMC_pvr);
    }
  }

  bool RegisterMe(void *Handle)
  {
    m_Handle = Handle;

    std::string libBasePath;
    libBasePath  = ((cb_array*)m_Handle)->libPath;
    libBasePath += PVR_HELPER_DLL;

    m_libXBMC_pvr = dlopen(libBasePath.c_str(), RTLD_LAZY);
    if (!m_libXBMC_pvr) { fprintf(stderr, "Unable to load %s\n", dlerror()); return false; }

    PVR_register_me         = (int (*)(void *HANDLE))
        dlsym(m_libXBMC_pvr, "PVR_register_me");
    if (!PVR_register_me)            { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    PVR_unregister_me       = (void (*)())
        dlsym(m_libXBMC_pvr, "PVR_unregister_me");
    if (!PVR_unregister_me)          { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

#ifdef USE_DEMUX
    AllocateDemuxPacket         = (DemuxPacket* (*)(int iDataSize))
        dlsym(m_libXBMC_pvr, "PVR_allocate_demux_packet");
    if (!AllocateDemuxPacket)        { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    FreeDemuxPacket             = (void (*)(DemuxPacket* pPacket))
        dlsym(m_libXBMC_pvr, "PVR_free_demux_packet");
    if (!FreeDemuxPacket)            { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }
#endif

    TransferChannelEntry        = (void (*)(const ADDON_HANDLE handle, const PVR_UPDATE_TYPE updateType, const PVR_CHANNEL *chan))
        dlsym(m_libXBMC_pvr, "PVR_transfer_channel_entry");
    if (!TransferChannelEntry)       { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    TransferChannelGroup        = (void (*)(const ADDON_HANDLE handle, const PVR_UPDATE_TYPE updateType, const PVR_CHANNEL_GROUP *group))
        dlsym(m_libXBMC_pvr, "PVR_transfer_channel_group");
    if (!TransferChannelGroup)       { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    TransferChannelGroupMember  = (void (*)(const ADDON_HANDLE handle, const PVR_UPDATE_TYPE updateType, const PVR_CHANNEL_GROUP_MEMBER *member))
        dlsym(m_libXBMC_pvr, "PVR_transfer_channel_group_member");
    if (!TransferChannelGroupMember) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    TransferEpgEntry            = (void (*)(const ADDON_HANDLE handle, const PVR_UPDATE_TYPE updateType, const EPG_TAG *epgentry))
        dlsym(m_libXBMC_pvr, "PVR_transfer_epg_entry");
    if (!TransferEpgEntry)           { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    TransferMenuHook            = (void (*)(const ADDON_HANDLE handle, const PVR_UPDATE_TYPE updateType, const PVR_MENUHOOK *hook))
        dlsym(m_libXBMC_pvr, "PVR_transfer_menu_hook");
    if (!TransferMenuHook)           { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    TransferRecordingEntry      = (void (*)(const ADDON_HANDLE handle, const PVR_UPDATE_TYPE updateType, const PVR_RECORDING *recording))
        dlsym(m_libXBMC_pvr, "PVR_transfer_recording_entry");
    if (!TransferRecordingEntry)     { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    TransferTimerEntry          = (void (*)(const ADDON_HANDLE handle, const PVR_UPDATE_TYPE updateType, const PVR_TIMER *timer))
        dlsym(m_libXBMC_pvr, "PVR_transfer_timer_entry");
    if (!TransferTimerEntry)         { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    Recording                   = (void (*)(const char *Name, const char *FileName, bool On))
        dlsym(m_libXBMC_pvr, "PVR_recording");
    if (!Recording)                  { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    return PVR_register_me(m_Handle) > 0;
  }

#ifdef USE_DEMUX
  DemuxPacket* (*AllocateDemuxPacket)        (int iDataSize);
  void         (*FreeDemuxPacket)            (DemuxPacket* pPacket);
#endif

  void         (*TransferChannelEntry)       (const ADDON_HANDLE handle, const PVR_UPDATE_TYPE updateType, const PVR_CHANNEL *chan);
  void         (*TransferChannelGroup)       (const ADDON_HANDLE handle, const PVR_UPDATE_TYPE updateType, const PVR_CHANNEL_GROUP *group);
  void         (*TransferChannelGroupMember) (const ADDON_HANDLE handle, const PVR_UPDATE_TYPE updateType, const PVR_CHANNEL_GROUP_MEMBER *member);
  void         (*TransferEpgEntry)           (const ADDON_HANDLE handle, const PVR_UPDATE_TYPE updateType, const EPG_TAG *epgentry);
  void         (*TransferMenuHook)           (const ADDON_HANDLE handle, const PVR_UPDATE_TYPE updateType, const PVR_MENUHOOK *hook);
  void         (*TransferRecordingEntry)     (const ADDON_HANDLE handle, const PVR_UPDATE_TYPE updateType, const PVR_RECORDING *recording);
  void         (*TransferTimerEntry)         (const ADDON_HANDLE handle, const PVR_UPDATE_TYPE updateType, const PVR_TIMER *timer);
  void         (*Recording)                  (const char *Name, const char *FileName, bool On);

protected:
  int          (*PVR_register_me)            (void *HANDLE);
  void         (*PVR_unregister_me)          (void);

private:
  void *m_libXBMC_pvr;
  void *m_Handle;
  struct cb_array
  {
    const char* libPath;
  };
};
