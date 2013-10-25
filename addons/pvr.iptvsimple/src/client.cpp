/*
 *      Copyright (C) 2013 Anton Fedchin
 *      http://github.com/afedchin/xbmc-addon-iptvsimple/
 *
 *      Copyright (C) 2011 Pulse-Eight
 *      http://www.pulse-eight.com/
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
#include "xbmc_pvr_dll.h"
#include "PVRIptvData.h"
#include "platform/util/util.h"

using namespace std;
using namespace ADDON;

#ifdef TARGET_WINDOWS
#define snprintf _snprintf
#endif

bool           m_bCreated       = false;
ADDON_STATUS   m_CurStatus      = ADDON_STATUS_UNKNOWN;
PVRIptvData   *m_data           = NULL;
bool           m_bIsPlaying     = false;
PVRIptvChannel m_currentChannel;

/* User adjustable settings are saved here.
 * Default values are defined inside client.h
 * and exported to the other source files.
 */
std::string g_strUserPath   = "";
std::string g_strClientPath = "";

CHelper_libXBMC_addon *XBMC = NULL;
CHelper_libXBMC_pvr   *PVR  = NULL;

std::string g_strTvgPath    = "";
std::string g_strM3UPath    = "";
std::string g_strLogoPath   = "";
int         g_iEPGTimeShift = 0;
int         g_iStartNumber  = 1;
bool        g_bTSOverride   = true;

extern std::string PathCombine(const std::string &strPath, const std::string &strFileName)
{
  std::string strResult = strPath;
  if (strResult.at(strResult.size() - 1) == '\\' ||
      strResult.at(strResult.size() - 1) == '/') 
  {
    strResult.append(strFileName);
  }
  else 
  {
    strResult.append("/");
    strResult.append(strFileName);
  }

  return strResult;
}

extern std::string GetClientFilePath(const std::string &strFileName)
{
  return PathCombine(g_strClientPath, strFileName);
}

extern std::string GetUserFilePath(const std::string &strFileName)
{
  return PathCombine(g_strUserPath, strFileName);
}

extern "C" {

void ADDON_ReadSettings(void)
{
  char buffer[1024];
  int iPathType = 0;
  if (!XBMC->GetSetting("m3uPathType", &iPathType)) 
  {
    iPathType = 1;
  }
  CStdString strSettingName = iPathType ? "m3uUrl" : "m3uPath";
  if (XBMC->GetSetting(strSettingName, &buffer)) 
  {
    g_strM3UPath = buffer;
  }
  if (g_strM3UPath == "") 
  {
    g_strM3UPath = GetClientFilePath(M3U_FILE_NAME);
  }
  if (!XBMC->GetSetting("startNum", &g_iStartNumber)) 
  {
    g_iStartNumber = 1;
  }
  if (!XBMC->GetSetting("epgPathType", &iPathType)) 
  {
    iPathType = 1;
  }
  strSettingName = iPathType ? "epgUrl" : "epgPath";
  if (XBMC->GetSetting(strSettingName, &buffer)) 
  {
    g_strTvgPath = buffer;
  }
  // BUG! xbmc does not return slider value 
  //float dTimeShift;
  //if (XBMC->GetSetting("epgTimeShift", &dTimeShift))
  //{
  //  g_iEPGTimeShift = (int)(dTimeShift * 3600.0); // hours to seconds
  //}
  int itmpShift;
  if (XBMC->GetSetting("epgTimeShift_", &itmpShift))
  {
    itmpShift -= 12;
    g_iEPGTimeShift = (int)(itmpShift * 3600.0); // hours to seconds
  }
  if (!XBMC->GetSetting("epgTSOverride", &g_bTSOverride))
  {
    g_bTSOverride = true;
  }
    
  if (XBMC->GetSetting("logoPath", &buffer))
  {
    g_strLogoPath = buffer;
  }
  if (g_strLogoPath == "")
  {
    g_strLogoPath = GetClientFilePath("icons/");
  }
}

ADDON_STATUS ADDON_Create(void* hdl, void* props)
{
  if (!hdl || !props)
  {
    return ADDON_STATUS_UNKNOWN;
  }

  PVR_PROPERTIES* pvrprops = (PVR_PROPERTIES*)props;

  XBMC = new CHelper_libXBMC_addon;
  if (!XBMC->RegisterMe(hdl))
  {
    SAFE_DELETE(XBMC);
    return ADDON_STATUS_PERMANENT_FAILURE;
  }

  PVR = new CHelper_libXBMC_pvr;
  if (!PVR->RegisterMe(hdl))
  {
    SAFE_DELETE(PVR);
    SAFE_DELETE(XBMC);
    return ADDON_STATUS_PERMANENT_FAILURE;
  }

  XBMC->Log(LOG_DEBUG, "%s - Creating the PVR IPTV Simple add-on", __FUNCTION__);

  m_CurStatus     = ADDON_STATUS_UNKNOWN;
  g_strUserPath   = pvrprops->strUserPath;
  g_strClientPath = pvrprops->strClientPath;

  if (!XBMC->DirectoryExists(g_strUserPath.c_str()))
  {
#ifdef TARGET_WINDOWS
    CreateDirectory(g_strUserPath.c_str(), NULL);
#else
    XBMC->CreateDirectory(g_strUserPath.c_str());
#endif
  }

  ADDON_ReadSettings();

  m_data = new PVRIptvData;
  m_CurStatus = ADDON_STATUS_OK;
  m_bCreated = true;

  return m_CurStatus;
}

ADDON_STATUS ADDON_GetStatus()
{
  return m_CurStatus;
}

void ADDON_Destroy()
{
  delete m_data;
  m_bCreated = false;
  m_CurStatus = ADDON_STATUS_UNKNOWN;
}

bool ADDON_HasSettings()
{
  return true;
}

unsigned int ADDON_GetSettings(ADDON_StructSetting ***sSet)
{
  return 0;
}

ADDON_STATUS ADDON_SetSetting(const char *settingName, const void *settingValue)
{
  // reset cache and restart addon 

  string strFile = GetUserFilePath(M3U_FILE_NAME);
  if (XBMC->FileExists(strFile.c_str(), false))
  {
#ifdef TARGET_WINDOWS
    DeleteFile(strFile.c_str());
#else
    XBMC->DeleteFile(strFile.c_str());
#endif
  }

  strFile = GetUserFilePath(TVG_FILE_NAME);
  if (XBMC->FileExists(strFile.c_str(), false))
  {
#ifdef TARGET_WINDOWS
    DeleteFile(strFile.c_str());
#else
    XBMC->DeleteFile(strFile.c_str());
#endif
  }

  return ADDON_STATUS_NEED_RESTART;
}

void ADDON_Stop()
{
}

void ADDON_FreeSettings()
{
}

void ADDON_Announce(const char *flag, const char *sender, const char *message, const void *data)
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
  pCapabilities->bSupportsChannelGroups   = true;
  pCapabilities->bSupportsRecordings      = false;

  return PVR_ERROR_NO_ERROR;
}

const char *GetBackendName(void)
{
  static const char *strBackendName = "IPTV Simple PVR Add-on";
  return strBackendName;
}

const char *GetBackendVersion(void)
{
  static CStdString strBackendVersion = PVR_CLIENT_VERSION;
  return strBackendVersion.c_str();
}

const char *GetConnectionString(void)
{
  static CStdString strConnectionString = "connected";
  return strConnectionString.c_str();
}

PVR_ERROR GetDriveSpace(long long *iTotal, long long *iUsed)
{
  *iTotal = 0;
  *iUsed  = 0;
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd)
{
  if (m_data)
    return m_data->GetEPGForChannel(handle, channel, iStart, iEnd);

  return PVR_ERROR_SERVER_ERROR;
}

int GetChannelsAmount(void)
{
  if (m_data)
    return m_data->GetChannelsAmount();

  return -1;
}

PVR_ERROR GetChannels(ADDON_HANDLE handle, bool bRadio)
{
  if (m_data)
    return m_data->GetChannels(handle, bRadio);

  return PVR_ERROR_SERVER_ERROR;
}

bool OpenLiveStream(const PVR_CHANNEL &channel)
{
  if (m_data)
  {
    CloseLiveStream();

    if (m_data->GetChannel(channel, m_currentChannel))
    {
      m_bIsPlaying = true;
      return true;
    }
  }

  return false;
}

void CloseLiveStream(void)
{
  m_bIsPlaying = false;
}

int GetCurrentClientChannel(void)
{
  return m_currentChannel.iUniqueId;
}

bool SwitchChannel(const PVR_CHANNEL &channel)
{
  CloseLiveStream();

  return OpenLiveStream(channel);
}

PVR_ERROR GetStreamProperties(PVR_STREAM_PROPERTIES* pProperties)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}

int GetChannelGroupsAmount(void)
{
  if (m_data)
    return m_data->GetChannelGroupsAmount();

  return -1;
}

PVR_ERROR GetChannelGroups(ADDON_HANDLE handle, bool bRadio)
{
  if (m_data)
    return m_data->GetChannelGroups(handle, bRadio);

  return PVR_ERROR_SERVER_ERROR;
}

PVR_ERROR GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group)
{
  if (m_data)
    return m_data->GetChannelGroupMembers(handle, group);

  return PVR_ERROR_SERVER_ERROR;
}

PVR_ERROR SignalStatus(PVR_SIGNAL_STATUS &signalStatus)
{
  snprintf(signalStatus.strAdapterName, sizeof(signalStatus.strAdapterName), "IPTV Simple Adapter 1");
  snprintf(signalStatus.strAdapterStatus, sizeof(signalStatus.strAdapterStatus), "OK");

  return PVR_ERROR_NO_ERROR;
}

/** UNUSED API FUNCTIONS */
const char * GetLiveStreamURL(const PVR_CHANNEL &channel)  { return ""; }
bool CanPauseStream(void) { return false; }
int GetRecordingsAmount(void) { return -1; }
PVR_ERROR GetRecordings(ADDON_HANDLE handle) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DialogChannelScan(void) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR CallMenuHook(const PVR_MENUHOOK &menuhook, const PVR_MENUHOOK_DATA &item) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DeleteChannel(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR RenameChannel(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR MoveChannel(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DialogChannelSettings(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DialogAddChannel(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
bool OpenRecordedStream(const PVR_RECORDING &recording) { return false; }
void CloseRecordedStream(void) {}
int ReadRecordedStream(unsigned char *pBuffer, unsigned int iBufferSize) { return 0; }
long long SeekRecordedStream(long long iPosition, int iWhence /* = SEEK_SET */) { return 0; }
long long PositionRecordedStream(void) { return -1; }
long long LengthRecordedStream(void) { return 0; }
void DemuxReset(void) {}
void DemuxFlush(void) {}
int ReadLiveStream(unsigned char *pBuffer, unsigned int iBufferSize) { return 0; }
long long SeekLiveStream(long long iPosition, int iWhence /* = SEEK_SET */) { return -1; }
long long PositionLiveStream(void) { return -1; }
long long LengthLiveStream(void) { return -1; }
PVR_ERROR DeleteRecording(const PVR_RECORDING &recording) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR RenameRecording(const PVR_RECORDING &recording) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR SetRecordingPlayCount(const PVR_RECORDING &recording, int count) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR SetRecordingLastPlayedPosition(const PVR_RECORDING &recording, int lastplayedposition) { return PVR_ERROR_NOT_IMPLEMENTED; }
int GetRecordingLastPlayedPosition(const PVR_RECORDING &recording) { return -1; }
PVR_ERROR GetRecordingEdl(const PVR_RECORDING&, PVR_EDL_ENTRY[], int*) { return PVR_ERROR_NOT_IMPLEMENTED; };
int GetTimersAmount(void) { return -1; }
PVR_ERROR GetTimers(ADDON_HANDLE handle) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR AddTimer(const PVR_TIMER &timer) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DeleteTimer(const PVR_TIMER &timer, bool bForceDelete) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR UpdateTimer(const PVR_TIMER &timer) { return PVR_ERROR_NOT_IMPLEMENTED; }
void DemuxAbort(void) {}
DemuxPacket* DemuxRead(void) { return NULL; }
unsigned int GetChannelSwitchDelay(void) { return 0; }
void PauseStream(bool bPaused) {}
bool CanSeekStream(void) { return false; }
bool SeekTime(int,bool,double*) { return false; }
void SetSpeed(int) {};
time_t GetPlayingTime() { return 0; }
time_t GetBufferTimeStart() { return 0; }
time_t GetBufferTimeEnd() { return 0; }
}
