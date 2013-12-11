/*
 *      Copyright (C) 2010 Alwin Esch (Team XBMC)
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
#include "xbmc_pvr_dll.h"
#include "VNSIDemux.h"
#include "VNSIRecording.h"
#include "VNSIData.h"
#include "VNSIChannelScan.h"
#include "VNSIAdmin.h"
#include "platform/util/util.h"

#include <sstream>
#include <string>
#include <iostream>

using namespace std;
using namespace ADDON;

ADDON_STATUS m_CurStatus      = ADDON_STATUS_UNKNOWN;

/* User adjustable settings are saved here.
 * Default values are defined inside client.h
 * and exported to the other source files.
 */
std::string   g_szHostname              = DEFAULT_HOST;
int           g_iPort                   = DEFAULT_PORT;
bool          g_bCharsetConv            = DEFAULT_CHARCONV;     ///< Convert VDR's incoming strings to UTF8 character set
bool          g_bHandleMessages         = DEFAULT_HANDLE_MSG;   ///< Send VDR's OSD status messages to XBMC OSD
int           g_iConnectTimeout         = DEFAULT_TIMEOUT;      ///< The Socket connection timeout
int           g_iPriority               = DEFAULT_PRIORITY;     ///< The Priority this client have in response to other clients
bool          g_bAutoChannelGroups      = DEFAULT_AUTOGROUPS;
int           g_iTimeshift              = 1;

CHelper_libXBMC_addon *XBMC   = NULL;
CHelper_libXBMC_codec *CODEC  = NULL;
CHelper_libXBMC_gui   *GUI    = NULL;
CHelper_libXBMC_pvr   *PVR    = NULL;

cVNSIDemux      *VNSIDemuxer       = NULL;
cVNSIData       *VNSIData          = NULL;
cVNSIRecording  *VNSIRecording     = NULL;

extern "C" {

/***********************************************************
 * Standart AddOn related public library functions
 ***********************************************************/

ADDON_STATUS ADDON_Create(void* hdl, void* props)
{
  if (!hdl || !props)
    return ADDON_STATUS_UNKNOWN;

  XBMC = new CHelper_libXBMC_addon;
  if (!XBMC->RegisterMe(hdl))
  {
    SAFE_DELETE(XBMC);
    return ADDON_STATUS_PERMANENT_FAILURE;
  }

  GUI = new CHelper_libXBMC_gui;
  if (!GUI->RegisterMe(hdl))
  {
    SAFE_DELETE(GUI);
    SAFE_DELETE(XBMC);
    return ADDON_STATUS_PERMANENT_FAILURE;
  }

  CODEC = new CHelper_libXBMC_codec;
  if (!CODEC->RegisterMe(hdl))
  {
    SAFE_DELETE(CODEC);
    SAFE_DELETE(GUI);
    SAFE_DELETE(XBMC);
    return ADDON_STATUS_PERMANENT_FAILURE;
  }

  PVR = new CHelper_libXBMC_pvr;
  if (!PVR->RegisterMe(hdl))
  {
    SAFE_DELETE(PVR);
    SAFE_DELETE(CODEC);
    SAFE_DELETE(GUI);
    SAFE_DELETE(XBMC);
    return ADDON_STATUS_PERMANENT_FAILURE;
  }

  XBMC->Log(LOG_DEBUG, "Creating VDR VNSI PVR-Client");

  m_CurStatus    = ADDON_STATUS_UNKNOWN;

  /* Read setting "host" from settings.xml */
  char * buffer = (char*) malloc(128);
  buffer[0] = 0; /* Set the end of string */

  if (XBMC->GetSetting("host", buffer))
    g_szHostname = buffer;
  else
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'host' setting, falling back to '%s' as default", DEFAULT_HOST);
    g_szHostname = DEFAULT_HOST;
  }
  free(buffer);

  /* Read setting "port" from settings.xml */
  if (!XBMC->GetSetting("port", &g_iPort))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'port' setting, falling back to '%i' as default", DEFAULT_PORT);
    g_iPort = DEFAULT_PORT;
  }

  /* Read setting "priority" from settings.xml */
  if (!XBMC->GetSetting("priority", &g_iPriority))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'priority' setting, falling back to %i as default", DEFAULT_PRIORITY);
    g_iPriority = DEFAULT_PRIORITY;
  }

  /* Read setting "timeshift" from settings.xml */
  if (!XBMC->GetSetting("timeshift", &g_iTimeshift))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'timeshift' setting, falling back to %i as default", 1);
    g_iTimeshift = 1;
  }

  /* Read setting "convertchar" from settings.xml */
  if (!XBMC->GetSetting("convertchar", &g_bCharsetConv))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'convertchar' setting, falling back to 'false' as default");
    g_bCharsetConv = DEFAULT_CHARCONV;
  }

  /* Read setting "timeout" from settings.xml */
  if (!XBMC->GetSetting("timeout", &g_iConnectTimeout))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'timeout' setting, falling back to %i seconds as default", DEFAULT_TIMEOUT);
    g_iConnectTimeout = DEFAULT_TIMEOUT;
  }

  /* Read setting "handlemessages" from settings.xml */
  if (!XBMC->GetSetting("handlemessages", &g_bHandleMessages))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'handlemessages' setting, falling back to 'true' as default");
    g_bHandleMessages = DEFAULT_HANDLE_MSG;
  }

  /* Read setting "autochannelgroups" from settings.xml */
  if (!XBMC->GetSetting("autochannelgroups", &g_bAutoChannelGroups))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'autochannelgroups' setting, falling back to 'false' as default");
    g_bAutoChannelGroups = DEFAULT_AUTOGROUPS;
  }

  VNSIData = new cVNSIData;
  if (!VNSIData->Open(g_szHostname, g_iPort))
  {
    ADDON_Destroy();
    m_CurStatus = ADDON_STATUS_LOST_CONNECTION;
    return m_CurStatus;
  }

  if (!VNSIData->Login())
  {
    ADDON_Destroy();
    m_CurStatus = ADDON_STATUS_LOST_CONNECTION;
    return m_CurStatus;
  }

  if (!VNSIData->EnableStatusInterface(g_bHandleMessages))
  {
    ADDON_Destroy();
    m_CurStatus = ADDON_STATUS_LOST_CONNECTION;
    return m_CurStatus;
  }

  PVR_MENUHOOK hook;
  hook.iHookId = 1;
  hook.category = PVR_MENUHOOK_SETTING;
  hook.iLocalizedStringId = 30107;
  PVR->AddMenuHook(&hook);

  m_CurStatus = ADDON_STATUS_OK;
  return m_CurStatus;
}

ADDON_STATUS ADDON_GetStatus()
{
  return m_CurStatus;
}

void ADDON_Destroy()
{
  SAFE_DELETE(CODEC);

  if (VNSIDemuxer)
    SAFE_DELETE(VNSIDemuxer);

  if (VNSIRecording)
    SAFE_DELETE(VNSIRecording);

  if (VNSIData)
    SAFE_DELETE(VNSIData);

  if (PVR)
    SAFE_DELETE(PVR);

  if (GUI)
    SAFE_DELETE(GUI);

  if (XBMC)
    SAFE_DELETE(XBMC);

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
  string str = settingName;
  if (str == "host")
  {
    string tmp_sHostname;
    XBMC->Log(LOG_INFO, "Changed Setting 'host' from %s to %s", g_szHostname.c_str(), (const char*) settingValue);
    tmp_sHostname = g_szHostname;
    g_szHostname = (const char*) settingValue;
    if (tmp_sHostname != g_szHostname)
      return ADDON_STATUS_NEED_RESTART;
  }
  else if (str == "port")
  {
    XBMC->Log(LOG_INFO, "Changed Setting 'port' from %u to %u", g_iPort, *(int*) settingValue);
    if (g_iPort != *(int*) settingValue)
    {
      g_iPort = *(int*) settingValue;
      return ADDON_STATUS_NEED_RESTART;
    }
  }
  else if (str == "priority")
  {
    XBMC->Log(LOG_INFO, "Changed Setting 'priority' from %u to %u", g_iPriority, *(int*) settingValue);
    g_iPriority = *(int*) settingValue;
  }
  else if (str == "timeshift")
  {
    XBMC->Log(LOG_INFO, "Changed Setting 'timeshift' from %u to %u", g_iTimeshift, *(int*) settingValue);
    g_iPriority = *(int*) settingValue;
  }
  else if (str == "convertchar")
  {
    XBMC->Log(LOG_INFO, "Changed Setting 'convertchar' from %u to %u", g_bCharsetConv, *(bool*) settingValue);
    g_bCharsetConv = *(bool*) settingValue;
  }
  else if (str == "timeout")
  {
    XBMC->Log(LOG_INFO, "Changed Setting 'timeout' from %u to %u", g_iConnectTimeout, *(int*) settingValue);
    g_iConnectTimeout = *(int*) settingValue;
  }
  else if (str == "handlemessages")
  {
    XBMC->Log(LOG_INFO, "Changed Setting 'handlemessages' from %u to %u", g_bHandleMessages, *(bool*) settingValue);
    g_bHandleMessages = *(bool*) settingValue;
    if (VNSIData) VNSIData->EnableStatusInterface(g_bHandleMessages);
  }
  else if (str == "autochannelgroups")
  {
    XBMC->Log(LOG_INFO, "Changed Setting 'autochannelgroups' from %u to %u", g_bAutoChannelGroups, *(bool*) settingValue);
    if (g_bAutoChannelGroups != *(bool*) settingValue)
    {
      g_bAutoChannelGroups = *(bool*) settingValue;
      return ADDON_STATUS_NEED_RESTART;
    }
  }

  return ADDON_STATUS_OK;
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
  pCapabilities->bSupportsEPG                = true;
  pCapabilities->bSupportsRecordings         = true;
  pCapabilities->bSupportsRecordingEdl       = true;
  pCapabilities->bSupportsTimers             = true;
  pCapabilities->bSupportsTV                 = true;
  pCapabilities->bSupportsRadio              = true;
  pCapabilities->bSupportsChannelGroups      = true;
  pCapabilities->bHandlesInputStream         = true;
  pCapabilities->bHandlesDemuxing            = true;
  if (VNSIData && VNSIData->SupportChannelScan())
    pCapabilities->bSupportsChannelScan      = true;

  return PVR_ERROR_NO_ERROR;
}

const char * GetBackendName(void)
{
  static std::string BackendName = VNSIData ? VNSIData->GetServerName() : "unknown";
  return BackendName.c_str();
}

const char * GetBackendVersion(void)
{
  static std::string BackendVersion;
  if (VNSIData) {
    std::stringstream format;
    format << VNSIData->GetVersion() << "(Protocol: " << VNSIData->GetProtocol() << ")";
    BackendVersion = format.str();
  }
  return BackendVersion.c_str();
}

const char * GetConnectionString(void)
{
  static std::string ConnectionString;
  std::stringstream format;

  if (VNSIData) {
    format << g_szHostname << ":" << g_iPort;
  }
  else {
    format << g_szHostname << ":" << g_iPort << " (addon error!)";
  }
  ConnectionString = format.str();
  return ConnectionString.c_str();
}

PVR_ERROR GetDriveSpace(long long *iTotal, long long *iUsed)
{
  if (!VNSIData)
    return PVR_ERROR_SERVER_ERROR;

  return (VNSIData->GetDriveSpace(iTotal, iUsed) ? PVR_ERROR_NO_ERROR : PVR_ERROR_SERVER_ERROR);
}

PVR_ERROR DialogChannelScan(void)
{
  cVNSIChannelScan scanner;
  scanner.Open(g_szHostname, g_iPort);
  return PVR_ERROR_NO_ERROR;
}

/*******************************************/
/** PVR EPG Functions                     **/

PVR_ERROR GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd)
{
  if (!VNSIData)
    return PVR_ERROR_SERVER_ERROR;

  return (VNSIData->GetEPGForChannel(handle, channel, iStart, iEnd) ? PVR_ERROR_NO_ERROR: PVR_ERROR_SERVER_ERROR);
}


/*******************************************/
/** PVR Channel Functions                 **/

int GetChannelsAmount(void)
{
  if (!VNSIData)
    return 0;

  return VNSIData->GetChannelsCount();
}

PVR_ERROR GetChannels(ADDON_HANDLE handle, bool bRadio)
{
  if (!VNSIData)
    return PVR_ERROR_SERVER_ERROR;

  return (VNSIData->GetChannelsList(handle, bRadio) ? PVR_ERROR_NO_ERROR : PVR_ERROR_SERVER_ERROR);
}


/*******************************************/
/** PVR Channelgroups Functions           **/

int GetChannelGroupsAmount()
{
  if (!VNSIData)
    return PVR_ERROR_SERVER_ERROR;

  return VNSIData->GetChannelGroupCount(g_bAutoChannelGroups);
}

PVR_ERROR GetChannelGroups(ADDON_HANDLE handle, bool bRadio)
{
  if (!VNSIData)
    return PVR_ERROR_SERVER_ERROR;

  if(VNSIData->GetChannelGroupCount(g_bAutoChannelGroups) > 0)
    return VNSIData->GetChannelGroupList(handle, bRadio) ? PVR_ERROR_NO_ERROR : PVR_ERROR_SERVER_ERROR;

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group)
{
  if (!VNSIData)
    return PVR_ERROR_SERVER_ERROR;

  return VNSIData->GetChannelGroupMembers(handle, group) ? PVR_ERROR_NO_ERROR : PVR_ERROR_SERVER_ERROR;
}


/*******************************************/
/** PVR Timer Functions                   **/

int GetTimersAmount(void)
{
  if (!VNSIData)
    return 0;

  return VNSIData->GetTimersCount();
}

PVR_ERROR GetTimers(ADDON_HANDLE handle)
{
  if (!VNSIData)
    return PVR_ERROR_SERVER_ERROR;

  return (VNSIData->GetTimersList(handle) ? PVR_ERROR_NO_ERROR : PVR_ERROR_SERVER_ERROR);
}

PVR_ERROR AddTimer(const PVR_TIMER &timer)
{
  if (!VNSIData)
    return PVR_ERROR_SERVER_ERROR;

  return VNSIData->AddTimer(timer);
}

PVR_ERROR DeleteTimer(const PVR_TIMER &timer, bool bForce)
{
  if (!VNSIData)
    return PVR_ERROR_SERVER_ERROR;

  return VNSIData->DeleteTimer(timer, bForce);
}

PVR_ERROR UpdateTimer(const PVR_TIMER &timer)
{
  if (!VNSIData)
    return PVR_ERROR_SERVER_ERROR;

  return VNSIData->UpdateTimer(timer);
}


/*******************************************/
/** PVR Recording Functions               **/

int GetRecordingsAmount(void)
{
  if (!VNSIData)
    return 0;

  return VNSIData->GetRecordingsCount();
}

PVR_ERROR GetRecordings(ADDON_HANDLE handle)
{
  if (!VNSIData)
    return PVR_ERROR_SERVER_ERROR;

  return VNSIData->GetRecordingsList(handle);
}

PVR_ERROR RenameRecording(const PVR_RECORDING &recording)
{
  if (!VNSIData)
    return PVR_ERROR_SERVER_ERROR;

  return VNSIData->RenameRecording(recording, recording.strTitle);
}

PVR_ERROR DeleteRecording(const PVR_RECORDING &recording)
{
  if (!VNSIData)
    return PVR_ERROR_SERVER_ERROR;

  return VNSIData->DeleteRecording(recording);
}

/*******************************************/
/** PVR Live Stream Functions             **/

bool OpenLiveStream(const PVR_CHANNEL &channel)
{
  CloseLiveStream();

  VNSIDemuxer = new cVNSIDemux;
  return VNSIDemuxer->OpenChannel(channel);
}

void CloseLiveStream(void)
{
  if (VNSIDemuxer)
  {
    VNSIDemuxer->Close();
    delete VNSIDemuxer;
    VNSIDemuxer = NULL;
  }
}

PVR_ERROR GetStreamProperties(PVR_STREAM_PROPERTIES* pProperties)
{
  if (!VNSIDemuxer)
    return PVR_ERROR_SERVER_ERROR;

  return (VNSIDemuxer->GetStreamProperties(pProperties) ? PVR_ERROR_NO_ERROR : PVR_ERROR_SERVER_ERROR);
}

void DemuxAbort(void)
{
  if (VNSIDemuxer) VNSIDemuxer->Abort();
}

DemuxPacket* DemuxRead(void)
{
  if (!VNSIDemuxer)
    return NULL;

  return VNSIDemuxer->Read();
}

int GetCurrentClientChannel(void)
{
  if (VNSIDemuxer)
    return VNSIDemuxer->CurrentChannel();

  return -1;
}

bool SwitchChannel(const PVR_CHANNEL &channel)
{
  if (VNSIDemuxer)
    return VNSIDemuxer->SwitchChannel(channel);

  return false;
}

PVR_ERROR SignalStatus(PVR_SIGNAL_STATUS &signalStatus)
{
  if (!VNSIDemuxer)
    return PVR_ERROR_SERVER_ERROR;

  return (VNSIDemuxer->GetSignalStatus(signalStatus) ? PVR_ERROR_NO_ERROR : PVR_ERROR_SERVER_ERROR);

}

bool CanPauseStream(void)
{
  bool ret = false;
  if (VNSIDemuxer)
    ret = VNSIDemuxer->IsTimeshift();
  return ret;
}

bool CanSeekStream(void)
{
  bool ret = false;
  if (VNSIDemuxer)
    ret = VNSIDemuxer->IsTimeshift();
  return ret;
}

bool SeekTime(int time, bool backwards, double *startpts)
{
  bool ret = false;
  if (VNSIDemuxer)
    ret = VNSIDemuxer->SeekTime(time, backwards, startpts);
  return ret;
}

time_t GetPlayingTime()
{
  time_t time = 0;
  if (VNSIDemuxer)
    time = VNSIDemuxer->GetPlayingTime();
  return time;
}

time_t GetBufferTimeStart()
{
  time_t time = 0;
  if (VNSIDemuxer)
    time = VNSIDemuxer->GetBufferTimeStart();
  return time;
}

time_t GetBufferTimeEnd()
{
  time_t time = 0;
  if (VNSIDemuxer)
    time = VNSIDemuxer->GetBufferTimeEnd();
  return time;
}

void SetSpeed(int) {};
void PauseStream(bool bPaused) {}

/*******************************************/
/** PVR Recording Stream Functions        **/

bool OpenRecordedStream(const PVR_RECORDING &recording)
{
  if(!VNSIData)
    return false;

  CloseRecordedStream();

  VNSIRecording = new cVNSIRecording;
  return VNSIRecording->OpenRecording(recording);
}

void CloseRecordedStream(void)
{
  if (VNSIRecording)
  {
    VNSIRecording->Close();
    delete VNSIRecording;
    VNSIRecording = NULL;
  }
}

int ReadRecordedStream(unsigned char *pBuffer, unsigned int iBufferSize)
{
  if (!VNSIRecording)
    return -1;

  return VNSIRecording->Read(pBuffer, iBufferSize);
}

long long SeekRecordedStream(long long iPosition, int iWhence /* = SEEK_SET */)
{
  if (VNSIRecording)
    return VNSIRecording->Seek(iPosition, iWhence);

  return -1;
}

long long PositionRecordedStream(void)
{
  if (VNSIRecording)
    return VNSIRecording->Position();

  return 0;
}

long long LengthRecordedStream(void)
{
  if (VNSIRecording)
    return VNSIRecording->Length();

  return 0;
}

PVR_ERROR GetRecordingEdl(const PVR_RECORDING& recinfo, PVR_EDL_ENTRY edl[], int *size)
{
  if(!VNSIData)
    return PVR_ERROR_UNKNOWN;

  return VNSIData->GetRecordingEdl(recinfo, edl, size);
}


/*******************************************/
/** PVR Menu Hook Functions               **/

PVR_ERROR CallMenuHook(const PVR_MENUHOOK &menuhook, const PVR_MENUHOOK_DATA &item)
{
  if (menuhook.iHookId == 1)
  {
    cVNSIAdmin osd;
    osd.Open(g_szHostname, g_iPort);
  }
  return PVR_ERROR_NO_ERROR;
}

/** UNUSED API FUNCTIONS */
PVR_ERROR DeleteChannel(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR RenameChannel(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR MoveChannel(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DialogChannelSettings(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DialogAddChannel(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
void DemuxReset(void) {}
void DemuxFlush(void) {}
int ReadLiveStream(unsigned char *pBuffer, unsigned int iBufferSize) { return 0; }
long long SeekLiveStream(long long iPosition, int iWhence /* = SEEK_SET */) { return -1; }
long long PositionLiveStream(void) { return -1; }
long long LengthLiveStream(void) { return -1; }
const char * GetLiveStreamURL(const PVR_CHANNEL &channel) { return ""; }
PVR_ERROR SetRecordingPlayCount(const PVR_RECORDING &recording, int count) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR SetRecordingLastPlayedPosition(const PVR_RECORDING &recording, int lastplayedposition) { return PVR_ERROR_NOT_IMPLEMENTED; }
int GetRecordingLastPlayedPosition(const PVR_RECORDING &recording) { return -1; }
unsigned int GetChannelSwitchDelay(void) { return 0; }
}
