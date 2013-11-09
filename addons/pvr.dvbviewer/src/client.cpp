/*
 *      Copyright (C) 2005-2011 Team XBMC
 *      http://xbmc.org
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

#include "client.h"
#include "DvbData.h"
#include "xbmc_pvr_dll.h"
#include "platform/util/util.h"
#include <stdlib.h>

using namespace ADDON;

ADDON_STATUS m_CurStatus = ADDON_STATUS_UNKNOWN;

/* User adjustable settings are saved here.
 * Default values are defined inside client.h
 * and exported to the other source files.
 */
CStdString g_strHostname             = DEFAULT_HOST;
int        g_iConnectTimeout         = DEFAULT_CONNECT_TIMEOUT;
int        g_iPortWeb                = DEFAULT_WEB_PORT;
CStdString g_strUsername             = "";
CStdString g_strPassword             = "";
bool       g_useFavourites           = false;
bool       g_useFavouritesFile       = false;
CStdString g_favouritesFile          = "";
bool       g_bUseTimeshift           = false;
CStdString g_strTimeshiftBufferPath  = DEFAULT_TSBUFFERPATH;
bool       g_useRTSP                 = false;

CHelper_libXBMC_addon *XBMC = NULL;
CHelper_libXBMC_pvr   *PVR  = NULL;
Dvb *DvbData                = NULL;

extern "C"
{
void ADDON_ReadSettings(void)
{
  char buffer[1024];

  if (XBMC->GetSetting("host", buffer))
    g_strHostname = buffer;

  if (XBMC->GetSetting("user", buffer))
    g_strUsername = buffer;

  if (XBMC->GetSetting("pass", buffer))
    g_strPassword = buffer;

  if (!XBMC->GetSetting("webport", &g_iPortWeb))
    g_iPortWeb = DEFAULT_WEB_PORT;

  if (!XBMC->GetSetting("usefavourites", &g_useFavourites))
    g_useFavourites = false;

  if (XBMC->GetSetting("usefavouritesfile", &g_useFavouritesFile))
    g_useFavouritesFile = false;

  if (g_useFavouritesFile && XBMC->GetSetting("favouritesfile", buffer))
    g_favouritesFile = buffer;

  if (!XBMC->GetSetting("usetimeshift", &g_bUseTimeshift))
    g_bUseTimeshift = false;

  if (XBMC->GetSetting("timeshiftpath", buffer))
    g_strTimeshiftBufferPath = buffer;

  if (!XBMC->GetSetting("usertsp", &g_useRTSP) || g_bUseTimeshift)
    g_useRTSP = false;

  /* Log the current settings for debugging purposes */
  XBMC->Log(LOG_DEBUG, "DVBViewer Addon Configuration options");
  XBMC->Log(LOG_DEBUG, "Hostname:   %s", g_strHostname.c_str());
  if (!g_strUsername.empty() && !g_strPassword.empty())
  {
    XBMC->Log(LOG_DEBUG, "Username:   %s", g_strUsername.c_str());
    XBMC->Log(LOG_DEBUG, "Password:   %s", g_strPassword.c_str());
  }
  XBMC->Log(LOG_DEBUG, "WebPort:    %d", g_iPortWeb);
  XBMC->Log(LOG_DEBUG, "Use favourites: %s", (g_useFavourites) ? "yes" : "no");
  if (g_useFavouritesFile)
    XBMC->Log(LOG_DEBUG, "Favourites File: %s", g_favouritesFile.c_str());
  XBMC->Log(LOG_DEBUG, "Timeshift: %s", (g_bUseTimeshift) ? "enabled" : "disabled");
  if (g_bUseTimeshift)
    XBMC->Log(LOG_DEBUG, "Timeshift Buffer Path: %s", g_strTimeshiftBufferPath.c_str());
  XBMC->Log(LOG_DEBUG, "Use RTSP: %s", (g_useRTSP) ? "yes" : "no");
}

ADDON_STATUS ADDON_Create(void* hdl, void* props)
{
  if (!hdl || !props)
    return ADDON_STATUS_UNKNOWN;

  XBMC = new CHelper_libXBMC_addon();
  if (!XBMC->RegisterMe(hdl))
  {
    SAFE_DELETE(XBMC);
    return ADDON_STATUS_PERMANENT_FAILURE;
  }

  PVR = new CHelper_libXBMC_pvr();
  if (!PVR->RegisterMe(hdl))
  {
    SAFE_DELETE(PVR);
    SAFE_DELETE(XBMC);
    return ADDON_STATUS_PERMANENT_FAILURE;
  }

  XBMC->Log(LOG_DEBUG, "%s - Creating DVBViewer PVR-Client", __FUNCTION__);
  m_CurStatus = ADDON_STATUS_UNKNOWN;

  ADDON_ReadSettings();

  DvbData = new Dvb();
  if (!DvbData->Open())
  {
    SAFE_DELETE(DvbData);
    SAFE_DELETE(PVR);
    SAFE_DELETE(XBMC);
    m_CurStatus = ADDON_STATUS_LOST_CONNECTION;
    return m_CurStatus;
  }

  m_CurStatus = ADDON_STATUS_OK;
  return m_CurStatus;
}

ADDON_STATUS ADDON_GetStatus()
{
  /* check whether we're still connected */
  if (m_CurStatus == ADDON_STATUS_OK && !DvbData->IsConnected())
    m_CurStatus = ADDON_STATUS_LOST_CONNECTION;

  return m_CurStatus;
}

void ADDON_Destroy()
{
  SAFE_DELETE(DvbData);
  SAFE_DELETE(PVR);
  SAFE_DELETE(XBMC);

  m_CurStatus = ADDON_STATUS_UNKNOWN;
}

bool ADDON_HasSettings()
{
  return true;
}

unsigned int ADDON_GetSettings(ADDON_StructSetting ***_UNUSED(sSet))
{
  return 0;
}

ADDON_STATUS ADDON_SetSetting(const char *settingName, const void *settingValue)
{
  // SetSetting can occur when the addon is enabled, but TV support still
  // disabled. In that case the addon is not loaded, so we should not try
  // to change its settings.
  if (!XBMC)
    return ADDON_STATUS_OK;

  CStdString sname(settingName);
  if (sname == "host")
  {
    if (g_strHostname.compare((const char *)settingValue) != 0)
      return ADDON_STATUS_NEED_RESTART;
  }
  else if (sname == "user")
  {
    if (g_strUsername.compare((const char *)settingValue) != 0)
      return ADDON_STATUS_NEED_RESTART;
  }
  else if (sname == "pass")
  {
    if (g_strPassword.compare((const char *)settingValue) != 0)
      return ADDON_STATUS_NEED_RESTART;
  }
  else if (sname == "webport")
  {
    if (g_iPortWeb != *(int *)settingValue)
      return ADDON_STATUS_NEED_RESTART;
  }
  else if (sname == "usefavourites")
  {
    if (g_useFavourites != *(bool *)settingValue)
      return ADDON_STATUS_NEED_RESTART;
  }
  else if (sname == "usefavouritesfile")
  {
    if (g_useFavouritesFile != *(bool *)settingValue)
      return ADDON_STATUS_NEED_RESTART;
  }
  else if (sname == "favouritesfile")
  {
    if (g_favouritesFile.compare((const char *)settingValue) != 0)
      return ADDON_STATUS_NEED_RESTART;
  }
  else if (sname == "usetimeshift")
  {
    if (g_bUseTimeshift != *(bool *)settingValue)
      return ADDON_STATUS_NEED_RESTART;
  }
  else if (sname == "timeshiftpath")
  {
    CStdString newValue = (const char *)settingValue;
    if (g_strTimeshiftBufferPath != newValue)
    {
      XBMC->Log(LOG_DEBUG, "%s - Changed Setting '%s' from '%s' to '%s'", __FUNCTION__,
          settingName, g_strTimeshiftBufferPath.c_str(), newValue.c_str());
      g_strTimeshiftBufferPath = newValue;
    }
  }
  else if (sname == "usertsp")
  {
    if (g_useRTSP != *(bool *)settingValue)
      return ADDON_STATUS_NEED_RESTART;
  }
  return ADDON_STATUS_OK;
}

void ADDON_Stop()
{
}

void ADDON_FreeSettings()
{
}

void ADDON_Announce(const char *_UNUSED(flag), const char *_UNUSED(sender),
    const char *_UNUSED(message), const void *_UNUSED(data))
{
}

/***********************************************************
 * PVR Client AddOn specific public library functions
 ***********************************************************/

const char* GetPVRAPIVersion(void)
{
  static const char *strApiVersion = XBMC_PVR_API_VERSION;
  return strApiVersion;
}

const char* GetMininumPVRAPIVersion(void)
{
  static const char *strMinApiVersion = XBMC_PVR_MIN_API_VERSION;
  return strMinApiVersion;
}

const char* GetGUIAPIVersion(void)
{
  static const char *strGuiApiVersion = XBMC_GUI_API_VERSION;
  return strGuiApiVersion;
}

const char* GetMininumGUIAPIVersion(void)
{
  static const char *strMinGuiApiVersion = XBMC_GUI_MIN_API_VERSION;
  return strMinGuiApiVersion;
}

PVR_ERROR GetAddonCapabilities(PVR_ADDON_CAPABILITIES* pCapabilities)
{
  pCapabilities->bSupportsEPG             = true;
  pCapabilities->bSupportsTV              = true;
  pCapabilities->bSupportsRadio           = true;
  pCapabilities->bSupportsRecordings      = true;
  pCapabilities->bSupportsTimers          = true;
  pCapabilities->bSupportsChannelGroups   = true;
  pCapabilities->bSupportsChannelScan     = false;
  pCapabilities->bHandlesInputStream      = false;
  pCapabilities->bHandlesDemuxing         = false;
  pCapabilities->bSupportsLastPlayedPosition = false;

  return PVR_ERROR_NO_ERROR;
}

const char *GetBackendName(void)
{
  static const CStdString name = DvbData ? DvbData->GetBackendName()
    : "unknown";
  return name.c_str();
}

const char *GetBackendVersion(void)
{
  static const CStdString version = DvbData ? DvbData->GetBackendVersion()
    : "UNKNOWN";
  return version.c_str();
}

const char *GetConnectionString(void)
{
  static CStdString conn;
  if (DvbData)
    conn.Format("%s%s", g_strHostname, DvbData->IsConnected() ? "" : " (Not connected!)");
  else
    conn.Format("%s (addon error!)", g_strHostname);
  return conn.c_str();
}

PVR_ERROR GetDriveSpace(long long *iTotal, long long *iUsed)
{
  if (!DvbData || !DvbData->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return DvbData->GetDriveSpace(iTotal, iUsed);
}

PVR_ERROR GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd)
{
  if (!DvbData || !DvbData->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return DvbData->GetEPGForChannel(handle, channel, iStart, iEnd);
}

int GetChannelsAmount(void)
{
  if (!DvbData || !DvbData->IsConnected())
    return 0;

  return DvbData->GetChannelsAmount();
}

PVR_ERROR GetChannels(ADDON_HANDLE handle, bool bRadio)
{
  if (!DvbData || !DvbData->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return DvbData->GetChannels(handle, bRadio);
}

int GetRecordingsAmount(void)
{
  if (!DvbData || !DvbData->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return DvbData->GetRecordingsAmount();
}

PVR_ERROR GetRecordings(ADDON_HANDLE handle)
{
  if (!DvbData || !DvbData->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return DvbData->GetRecordings(handle);
}

PVR_ERROR DeleteRecording(const PVR_RECORDING &recording)
{
  if (!DvbData || !DvbData->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return DvbData->DeleteRecording(recording);
}

PVR_ERROR RenameRecording(const PVR_RECORDING &_UNUSED(recording))
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}

int GetTimersAmount(void)
{
  if (!DvbData || !DvbData->IsConnected())
    return 0;

  return DvbData->GetTimersAmount();
}

PVR_ERROR GetTimers(ADDON_HANDLE handle)
{
  if (!DvbData || !DvbData->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return DvbData->GetTimers(handle);
}

PVR_ERROR AddTimer(const PVR_TIMER &timer)
{
  if (!DvbData || !DvbData->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return DvbData->AddTimer(timer);
}

PVR_ERROR DeleteTimer(const PVR_TIMER &timer, bool _UNUSED(bForceDelete))
{
  if (!DvbData || !DvbData->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return DvbData->DeleteTimer(timer);
}

PVR_ERROR UpdateTimer(const PVR_TIMER &timer)
{
  if (!DvbData || !DvbData->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return DvbData->UpdateTimer(timer);
}

int GetCurrentClientChannel(void)
{
  if (!DvbData || !DvbData->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return DvbData->GetCurrentClientChannel();
}

bool SwitchChannel(const PVR_CHANNEL &channel)
{
  if (!DvbData || !DvbData->IsConnected())
    return false;

  return DvbData->SwitchChannel(channel);
}

int GetChannelGroupsAmount(void)
{
  if (!DvbData || !DvbData->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return DvbData->GetChannelGroupsAmount();
}

PVR_ERROR GetChannelGroups(ADDON_HANDLE handle, bool bRadio)
{
  if (!DvbData || !DvbData->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return DvbData->GetChannelGroups(handle, bRadio);
}

PVR_ERROR GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group)
{
  if (!DvbData || !DvbData->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return DvbData->GetChannelGroupMembers(handle, group);
}

void CloseLiveStream(void)
{
  DvbData->CloseLiveStream();
}

bool OpenLiveStream(const PVR_CHANNEL &channel)
{
  if (!DvbData || !DvbData->IsConnected())
    return false;

  return DvbData->OpenLiveStream(channel);
}

const char *GetLiveStreamURL(const PVR_CHANNEL &channel)
{
  if (!DvbData || !DvbData->IsConnected())
    return "";

  return DvbData->GetLiveStreamURL(channel).c_str();
}

bool CanPauseStream(void)
{
  if (!DvbData || !DvbData->IsConnected())
    return false;

  return g_bUseTimeshift;
}

bool CanSeekStream(void)
{
  if (!DvbData || !DvbData->IsConnected())
    return false;

  return g_bUseTimeshift;
}

int ReadLiveStream(unsigned char *pBuffer, unsigned int iBufferSize)
{
  if (!DvbData || !DvbData->IsConnected())
    return 0;

  return DvbData->ReadLiveStream(pBuffer, iBufferSize);
}

long long SeekLiveStream(long long iPosition, int iWhence /* = SEEK_SET */)
{
  if (!DvbData || !DvbData->IsConnected())
    return -1;

  return DvbData->SeekLiveStream(iPosition, iWhence);
}

long long PositionLiveStream(void)
{
  if (!DvbData || !DvbData->IsConnected())
    return -1;

  return DvbData->PositionLiveStream();
}

long long LengthLiveStream(void)
{
  if (!DvbData || !DvbData->IsConnected())
    return 0;

  return DvbData->LengthLiveStream();
}

PVR_ERROR SignalStatus(PVR_SIGNAL_STATUS &signalStatus)
{
  // the RS api doesn't provide information about signal quality (yet)
  strncpy(signalStatus.strAdapterName, "DVBViewer Recording Service",
      sizeof(signalStatus.strAdapterName));
  strncpy(signalStatus.strAdapterStatus, "OK",
      sizeof(signalStatus.strAdapterStatus));
  return PVR_ERROR_NO_ERROR;
}

/** UNUSED API FUNCTIONS */
PVR_ERROR GetStreamProperties(PVR_STREAM_PROPERTIES* _UNUSED(pProperties)) { return PVR_ERROR_NOT_IMPLEMENTED; }
void DemuxAbort(void) { return; }
DemuxPacket* DemuxRead(void) { return NULL; }
PVR_ERROR DialogChannelScan(void) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR CallMenuHook(const PVR_MENUHOOK &_UNUSED(menuhook), const PVR_MENUHOOK_DATA &_UNUSED(item)) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DeleteChannel(const PVR_CHANNEL &_UNUSED(channel)) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR RenameChannel(const PVR_CHANNEL &_UNUSED(channel)) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR MoveChannel(const PVR_CHANNEL &_UNUSED(channel)) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DialogChannelSettings(const PVR_CHANNEL &_UNUSED(channel)) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DialogAddChannel(const PVR_CHANNEL &_UNUSED(channel)) { return PVR_ERROR_NOT_IMPLEMENTED; }
bool OpenRecordedStream(const PVR_RECORDING &_UNUSED(recording)) { return false; }
void CloseRecordedStream(void) {}
int ReadRecordedStream(unsigned char *_UNUSED(pBuffer), unsigned int _UNUSED(iBufferSize)) { return 0; }
long long SeekRecordedStream(long long _UNUSED(iPosition), int _UNUSED(iWhence) /* = SEEK_SET */) { return 0; }
void DemuxReset(void) {}
void DemuxFlush(void) {}
long long PositionRecordedStream(void) { return -1; }
long long LengthRecordedStream(void) { return -1; }
PVR_ERROR SetRecordingPlayCount(const PVR_RECORDING &_UNUSED(recording), int _UNUSED(count)) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR SetRecordingLastPlayedPosition(const PVR_RECORDING &_UNUSED(recording), int _UNUSED(lastplayedposition)) { return PVR_ERROR_NOT_IMPLEMENTED; }
int GetRecordingLastPlayedPosition(const PVR_RECORDING &_UNUSED(recording)) { return -1; }
PVR_ERROR GetRecordingEdl(const PVR_RECORDING&, PVR_EDL_ENTRY[], int*) { return PVR_ERROR_NOT_IMPLEMENTED; };
unsigned int GetChannelSwitchDelay(void) { return 0; }
void PauseStream(bool _UNUSED(bPaused)) {}
bool SeekTime(int, bool, double*) { return false; }
void SetSpeed(int) {};
time_t GetPlayingTime() { return 0; }
time_t GetBufferTimeStart() { return 0; }
time_t GetBufferTimeEnd() { return 0; }
}
