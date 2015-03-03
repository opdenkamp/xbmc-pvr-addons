/*
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
 *  the Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301  USA
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "client.h"
#include "xbmc_pvr_dll.h"
#include <stdlib.h>
#include "PctvData.h"
#include "platform/util/util.h"

using namespace std;
using namespace ADDON;

bool           m_bCreated         = false;
ADDON_STATUS   m_CurStatus        = ADDON_STATUS_UNKNOWN;

PctvChannel    m_currentChannel;

/* User adjustable settings are saved here.
 * Default values are defined inside client.h
 * and exported to the other source files.
 */
std::string g_strHostname         = DEFAULT_HOST;
int         g_iPortWeb            = DEFAULT_WEB_PORT;
std::string g_strPin              = DEFAULT_PIN;
std::string g_strAuth             = DEFAULT_AUTH;
std::string g_strBaseUrl          = "";
int         g_iStartNumber        = 1;
std::string g_strUserPath         = "";
std::string g_strClientPath       = "";
int         g_iEPGTimeShift       = 0;
bool        g_bTranscode          = DEFAULT_TRANSCODE;
bool        g_bUsePIN			  = DEFAULT_USEPIN;
int         g_iBitrate            = DEFAULT_BITRATE;
CHelper_libXBMC_addon *XBMC       = NULL;
CHelper_libXBMC_pvr   *PVR        = NULL;
Pctv                  *PctvData   = NULL;


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
  g_iStartNumber = 1;
    
  /* Read setting "host" from settings.xml */
  int iValue;
  char * buffer;
  buffer = (char*) malloc (1024);
  buffer[0] = 0; /* Set the end of string */

  if (XBMC->GetSetting("host", buffer))
  {
	  g_strHostname = buffer;
  }   
  else 
  {
	  g_strHostname = DEFAULT_HOST;
  }  
  buffer[0] = 0; /* Set the end of string */

  /* Read setting "webport" from settings.xml */
  if (!XBMC->GetSetting("webport", &g_iPortWeb)) 
  {
	  g_iPortWeb = DEFAULT_WEB_PORT;
  }  

  /* Read setting "usepin" from settings.xml */
  if (!XBMC->GetSetting("usepin", &g_bUsePIN)) 
  {
	  g_bUsePIN = DEFAULT_USEPIN;
  }	

  /* Read setting "pin" from settings.xml */  
  if (XBMC->GetSetting("pin", &iValue))
  {
	  sprintf(buffer, "%04i", iValue);
	  g_strPin = buffer;
  }
  else
  {
	  g_strPin = DEFAULT_PIN;
  }
  buffer[0] = 0; /* Set the end of string */

  /* Read setting "transcode" from settings.xml */
  if (!XBMC->GetSetting("transcode", &g_bTranscode))  
    g_bTranscode = DEFAULT_TRANSCODE;

  /* Read setting "bitrate" from settings.xml */
  if (!XBMC->GetSetting("bitrate", &g_iBitrate))
    g_iBitrate = DEFAULT_BITRATE; 


  free (buffer);
}

ADDON_STATUS ADDON_Create(void* hdl, void* props)
{
  if (!hdl || !props)
    return ADDON_STATUS_UNKNOWN;

  PVR_PROPERTIES* pvrprops = (PVR_PROPERTIES*)props;

  XBMC = new CHelper_libXBMC_addon;
  if (!XBMC->RegisterMe(hdl))
  {
    SAFE_DELETE(XBMC);
    return ADDON_STATUS_PERMANENT_FAILURE;
  }

  XBMC->Log(LOG_DEBUG, "%s - Creating PCTV Systems PVR-Client", __FUNCTION__);

  PVR = new CHelper_libXBMC_pvr;
  if (!PVR->RegisterMe(hdl))
  {
    SAFE_DELETE(PVR);
    SAFE_DELETE(XBMC);
    return ADDON_STATUS_PERMANENT_FAILURE;
  }

  m_CurStatus = ADDON_STATUS_UNKNOWN;
  
  g_strUserPath = pvrprops->strUserPath;
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

  PctvData = new Pctv;

  if (!PctvData->Open())
  {
    SAFE_DELETE(PctvData);
    SAFE_DELETE(PVR);
    SAFE_DELETE(XBMC);
    m_CurStatus = ADDON_STATUS_LOST_CONNECTION;
    return m_CurStatus;
  }

  m_CurStatus = ADDON_STATUS_OK;
  m_bCreated = true;
  return m_CurStatus;
}

ADDON_STATUS ADDON_GetStatus()
{
  /* check whether we're still connected */
  if (m_CurStatus == ADDON_STATUS_OK && !PctvData->IsConnected())
    m_CurStatus = ADDON_STATUS_LOST_CONNECTION;

  return m_CurStatus;
}

void ADDON_Destroy()
{
  if (m_bCreated)
  {
    m_bCreated = false;
  }
    
  SAFE_DELETE(PctvData);
  SAFE_DELETE(PVR);
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
    string strNewHostname = (const char*)settingValue;
    if (strNewHostname != g_strHostname)
    {
      g_strHostname = strNewHostname;
      XBMC->Log(LOG_INFO, "%s - Changed Setting 'host' from %s to %s", __FUNCTION__, g_strHostname.c_str(), (const char*)settingValue);
      return ADDON_STATUS_NEED_RESTART;
    }    
  }
  else if (str == "webport")
  {
    int iNewValue = *(int*) settingValue;
    if (g_iPortWeb != iNewValue)
    {
      g_iPortWeb = iNewValue;
      XBMC->Log(LOG_INFO, "%s - Changed Setting 'webport' from %u to %u", __FUNCTION__, g_iPortWeb, iNewValue);      
      return ADDON_STATUS_NEED_RESTART;
    }
  }
  else if (str == "usepin")
  {
	  bool bNewUsePIN = *(bool*)settingValue;
	  if (bNewUsePIN != g_bUsePIN)
	  {
		  g_bUsePIN = bNewUsePIN;
		  XBMC->Log(LOG_INFO, "%s - Changed Setting 'usepin'", __FUNCTION__);
		  return ADDON_STATUS_NEED_RESTART;      // restart is needed
	  }
  }
  else if (str == "pin")
  {
    string strNewPin = (const char*)settingValue;        
    if (strNewPin != g_strPin)
    {
      g_strPin = strNewPin;
      XBMC->Log(LOG_INFO, "%s - Changed Setting 'pin'", __FUNCTION__);      
      return ADDON_STATUS_NEED_RESTART;
    }
  } 
  else if (str == "transcode")
  {
    bool bNewTranscode = *(bool*)settingValue;    
    if (bNewTranscode != g_bTranscode)
    {
      g_bTranscode = bNewTranscode;
      XBMC->Log(LOG_INFO, "%s - Changed Setting 'transcode'", __FUNCTION__);      
      return ADDON_STATUS_NEED_RESTART;      // restart is needed to update strStreamURL in Channel entries
    }
  }
  else if (str == "bitrate")
  {
    int iNewBitrate = *(int*)settingValue;
    if (g_iBitrate != iNewBitrate)
    {
      g_iBitrate = *(int*)settingValue;
      XBMC->Log(LOG_INFO, "%s - Changed Setting 'bitrate' from %u to %u", __FUNCTION__, g_iBitrate, iNewBitrate);      
      return ADDON_STATUS_NEED_RESTART;     // restart is needed to update strStreamURL in Channel entries
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
  pCapabilities->bSupportsEPG                 = true;
  pCapabilities->bSupportsTV                  = true;
  pCapabilities->bSupportsRadio               = false;
  pCapabilities->bSupportsChannelGroups       = true;
  pCapabilities->bSupportsRecordings          = true;
  pCapabilities->bSupportsTimers              = true;
  pCapabilities->bSupportsChannelScan         = false;
  pCapabilities->bHandlesInputStream          = false;
  pCapabilities->bHandlesDemuxing             = false;
  pCapabilities->bSupportsLastPlayedPosition  = false;

  return PVR_ERROR_NO_ERROR;
}

const char *GetBackendName(void)
{  
  static const char *strBackendName = PctvData ? PctvData->GetBackendName() : "unknown";
  return strBackendName;
}

const char *GetBackendVersion(void)
{
  static const char *strBackendVersion = PctvData ? PctvData->GetBackendVersion() : "unknown";  
  return strBackendVersion;
}

const char *GetBackendHostname(void)
{
    return g_strHostname.c_str();
}

const char *GetConnectionString(void)
{
  //static CStdString strConnectionString = "connected";
  static CStdString strConnectionString;
  if (PctvData)
    strConnectionString.Format("%s%s", g_strHostname.c_str(), PctvData->IsConnected() ? "" : " (Not connected!)");
  else
    strConnectionString.Format("%s (addon error!)", g_strHostname.c_str());
  return strConnectionString.c_str();
}

PVR_ERROR GetDriveSpace(long long *iTotal, long long *iUsed)
{   
  if (!PctvData || !PctvData->IsConnected())
	  return PVR_ERROR_SERVER_ERROR;

  if (!PctvData->IsSupported("storage"))
	  return PVR_ERROR_NOT_IMPLEMENTED;	
	
  return PctvData->GetStorageInfo(iTotal, iUsed);
}

int GetChannelsAmount(void)
{
  if (!PctvData || !PctvData->IsConnected())
    return -1;

  return PctvData->GetChannelsAmount();
}

PVR_ERROR GetChannels(ADDON_HANDLE handle, bool bRadio)
{
  if (!PctvData || !PctvData->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return PctvData->GetChannels(handle, bRadio);  
}

bool OpenLiveStream(const PVR_CHANNEL &channel)
{
  if (!PctvData || !PctvData->IsConnected())
    return false;
    
  CloseLiveStream();

  if (PctvData->GetChannel(channel, m_currentChannel))
  {    
    return true;
  }  

  return false;
}

void CloseLiveStream(void)
{  
  if (PctvData) {
    PctvData->CloseLiveStream();
  }  
}

bool SwitchChannel(const PVR_CHANNEL &channel)
{
  CloseLiveStream();

  return OpenLiveStream(channel);
}

int GetCurrentClientChannel(void)
{
  return m_currentChannel.iUniqueId;
}

PVR_ERROR GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd)
{
  if (!PctvData || !PctvData->IsConnected())
    return PVR_ERROR_SERVER_ERROR;
  
  return PctvData->GetEPGForChannel(handle, channel, iStart, iEnd);
}

int GetRecordingsAmount(bool deleted)
{
  if (!PctvData || !PctvData->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return PctvData->GetRecordingsAmount();
}

PVR_ERROR GetRecordings(ADDON_HANDLE handle, bool deleted)
{
  if (!PctvData || !PctvData->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return PctvData->GetRecordings(handle);
}

int GetTimersAmount(void)
{
  if (!PctvData || !PctvData->IsConnected())
    return 0;

  return PctvData->GetTimersAmount();
}

PVR_ERROR GetTimers(ADDON_HANDLE handle)
{
  if (!PctvData || !PctvData->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return PctvData->GetTimers(handle);
}

PVR_ERROR AddTimer(const PVR_TIMER &timer) { 
  
  if (!PctvData || !PctvData->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return PctvData->AddTimer(timer);

}

int GetChannelGroupsAmount(void)
{
  if (!PctvData || !PctvData->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return PctvData->GetChannelGroupsAmount();
}

PVR_ERROR GetChannelGroups(ADDON_HANDLE handle, bool bRadio) 
{ 
  if (!PctvData || !PctvData->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return PctvData->GetChannelGroups(handle, bRadio);
}

PVR_ERROR GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group)
{
  if (group.bIsRadio)
    return PVR_ERROR_NO_ERROR;

  if (!PctvData || !PctvData->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return PctvData->GetChannelGroupMembers(handle, group);
}


/** UNUSED API FUNCTIONS */
const char * GetLiveStreamURL(const PVR_CHANNEL &channel)  { return ""; }
PVR_ERROR SignalStatus(PVR_SIGNAL_STATUS &signalStatus) { return PVR_ERROR_NO_ERROR; }
PVR_ERROR GetStreamProperties(PVR_STREAM_PROPERTIES* pProperties) { return PVR_ERROR_NOT_IMPLEMENTED; }
void DemuxAbort(void) { return; }
DemuxPacket* DemuxRead(void) { return NULL; }
PVR_ERROR DialogChannelScan(void) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR CallMenuHook(const PVR_MENUHOOK &menuhook, const PVR_MENUHOOK_DATA &item) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DeleteChannel(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR RenameChannel(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR MoveChannel(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DialogChannelSettings(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DialogAddChannel(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR UpdateTimer(const PVR_TIMER &timer) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR SetRecordingLastPlayedPosition(const PVR_RECORDING &recording, int lastplayedposition) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR RenameRecording(const PVR_RECORDING &recording) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DeleteRecording(const PVR_RECORDING &recording) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DeleteTimer(const PVR_TIMER &timer, bool bForceDelete)  { return PVR_ERROR_NOT_IMPLEMENTED; }
int GetRecordingLastPlayedPosition(const PVR_RECORDING &recording) { return PVR_ERROR_NOT_IMPLEMENTED; }
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
PVR_ERROR SetRecordingPlayCount(const PVR_RECORDING &recording, int count) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR GetRecordingEdl(const PVR_RECORDING&, PVR_EDL_ENTRY[], int*) { return PVR_ERROR_NOT_IMPLEMENTED; };
unsigned int GetChannelSwitchDelay(void) { return 0; }
void PauseStream(bool bPaused) {}
bool CanPauseStream(void) { return false; }
bool CanSeekStream(void) { return false; }
bool SeekTime(int, bool, double*) { return false; }
void SetSpeed(int) {};
time_t GetPlayingTime() { return 0; }
time_t GetBufferTimeStart() { return 0; }
time_t GetBufferTimeEnd() { return 0; }
PVR_ERROR UndeleteRecording(const PVR_RECORDING& recording) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DeleteAllRecordingsFromTrash() { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR OpenDialogChannelScan(void) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR OpenDialogChannelSettings(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR OpenDialogChannelAdd(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
}
