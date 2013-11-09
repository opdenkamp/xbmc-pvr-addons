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

#include "client.h"
#include "xbmc_pvr_dll.h"
#include "DVBLinkClient.h"
#include "platform/util/util.h"

using namespace std;
using namespace ADDON;


#ifdef TARGET_WINDOWS
#define snprintf _snprintf
#endif

bool           m_bCreated           = false;
ADDON_STATUS   m_CurStatus          = ADDON_STATUS_UNKNOWN;


std::string g_strUserPath           = "";
std::string g_strClientPath         = "";

DVBLinkClient* dvblinkclient = NULL;

std::string g_szHostname            = DEFAULT_HOST;                  ///< The Host name or IP of the DVBLink Server
long        g_lPort                 = DEFAULT_PORT;                  ///< The DVBLink Connect Server listening port (default: 8080)
int         g_iConnectTimeout       = DEFAULT_TIMEOUT;               ///< The Socket connection timeout
DVBLINK_STREAMTYPE g_eStreamType    = DEFAULT_STREAMTYPE;            ///< Stream type used by video stream
std::string g_szClientname          = DEFAULT_CLIENTNAME;            ///< Name of dvblink client
std::string g_szUsername            = DEFAULT_USERNAME;              ///< Username
std::string g_szPassword            = DEFAULT_PASSWORD;              ///< Password
bool        g_bUseChlHandle         = DEFAULT_USECHLHANDLE;          ///< Use channel handle instead of client id
bool        g_bShowInfoMSG          = DEFAULT_SHOWINFOMSG;           ///< Show information messages
int         g_iHeight               = DEFAULT_HEIGHT;                ///< Height of stream when using transcoding
int         g_iWidth                = DEFAULT_WIDTH;                 ///< Width of stream when using transcoding
int         g_iBitrate              = DEFAULT_BITRATE;               ///< Bitrate of stream when using transcoding
std::string g_szAudiotrack          = DEFAULT_AUDIOTRACK;            ///< Audiotrack to include in stream when using transcoding
std::string g_szTimeShiftBufferPath = DEFAULT_TIMESHIFTBUFFERPATH;   ///< Path to timeshift buffer 
bool        g_bUseTimeshift         = DEFAULT_USETIMESHIFT;             ///< Use timeshift
CHelper_libXBMC_addon *XBMC         = NULL;
CHelper_libXBMC_pvr   *PVR          = NULL;

extern "C" {


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

  PVR = new CHelper_libXBMC_pvr;
  if (!PVR->RegisterMe(hdl))
  {
    SAFE_DELETE(PVR);
    SAFE_DELETE(XBMC);
    return ADDON_STATUS_PERMANENT_FAILURE;
  }

  XBMC->Log(LOG_DEBUG, "%s - Creating the PVR DVBlink add-on", __FUNCTION__);

  m_CurStatus     = ADDON_STATUS_UNKNOWN;
  g_strUserPath   = pvrprops->strUserPath;
  g_strClientPath = pvrprops->strClientPath;

  char * buffer = (char*) malloc(128);
  buffer[0] = 0; 

  /* Connection settings */
  /***********************/

  if (XBMC->GetSetting("host", buffer))
  {
    g_szHostname = buffer;
  }
  else
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'host' setting, falling back to '127.0.0.1' as default");
    g_szHostname = DEFAULT_HOST;
  }

  /* Read setting "client" from settings.xml */
  if (XBMC->GetSetting("client", buffer))
  {
    g_szClientname = buffer;
  }
  else
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'clientname' setting, falling back to 'xbmc' as default");
    g_szClientname = DEFAULT_CLIENTNAME;
  }

  /* Read setting "username" from settings.xml */
  if (XBMC->GetSetting("username", buffer))
  {
    g_szUsername = buffer;
  }
  else
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'username' setting, falling back to '' as default");
    g_szUsername = DEFAULT_USERNAME;
  }

  /* Read setting "password" from settings.xml */
  if (XBMC->GetSetting("password", buffer))
  {
    g_szPassword = buffer;
  }
  else
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'password' setting, falling back to '' as default");
    g_szPassword = DEFAULT_PASSWORD;
  }

  /* Read setting "streamtype" from settings.xml */
  if (!XBMC->GetSetting("streamtype", &g_eStreamType))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'streamtype' setting, falling back to 'http' as default");
    g_eStreamType = DEFAULT_STREAMTYPE;
  }

  /* Read setting "port" from settings.xml */
  if (!XBMC->GetSetting("port", &g_lPort))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'port' setting, falling back to '8080' as default");
    g_lPort = DEFAULT_PORT;
  }

  /* Read setting "timeout" from settings.xml */
  if (!XBMC->GetSetting("timeout", &g_iConnectTimeout))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'timeout' setting, falling back to %i seconds as default", DEFAULT_TIMEOUT);
    g_iConnectTimeout = DEFAULT_TIMEOUT;
  }

  /* Read setting "ch_handle" from settings.xml */
  if (!XBMC->GetSetting("ch_handle", &g_bUseChlHandle))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'ch_handle' setting, falling back to 'true' as default");
    g_bUseChlHandle = DEFAULT_USECHLHANDLE;
  }

  /* Read setting "timeshift" from settings.xml */
  if (!XBMC->GetSetting("timeshift", &g_bUseTimeshift))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'timeshift' setting, falling back to 'false' as default");
    g_bUseTimeshift = DEFAULT_USETIMESHIFT;
  }

  /* Read setting "timeshiftpath" from settings.xml */
  if (XBMC->GetSetting("timeshiftpath", buffer))
  {
    g_szTimeShiftBufferPath = buffer;
  }
  else
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'timeshiftpath' setting, falling back to 'special://userdata/addon_data/pvr.dvblink/' as default");
    g_szTimeShiftBufferPath = DEFAULT_TIMESHIFTBUFFERPATH;
  }


  /* Read setting "ch_handle" from settings.xml */
  if (!XBMC->GetSetting("showinfomsg", &g_bShowInfoMSG))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'showinfomsg' setting, falling back to 'true' as default");
    g_bShowInfoMSG = DEFAULT_SHOWINFOMSG;
  }

  /* Read setting "height" from settings.xml */
  if (!XBMC->GetSetting("height", &g_iHeight))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'Height' setting, falling back to '720' as default");
    g_iHeight = DEFAULT_HEIGHT;
  }

  /* Read setting "width" from settings.xml */
  if (!XBMC->GetSetting("width", &g_iWidth))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'Width' setting, falling back to '576' as default");
    g_iWidth = DEFAULT_WIDTH;
  }

  /* Read setting "bitrate" from settings.xml */
  if (!XBMC->GetSetting("bitrate", &g_iBitrate))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'Biterate' setting, falling back to '512' as default");
    g_iBitrate = DEFAULT_BITRATE;
  }

  /* Read setting "audiotrack" from settings.xml */
  if (XBMC->GetSetting("audiotrack", buffer))
  {
    g_szAudiotrack = buffer;
  }else
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'Audiotrack' setting, falling back to 'eng' as default");
    g_szAudiotrack = DEFAULT_AUDIOTRACK;
  }

  /* Log the current settings for debugging purposes */
  XBMC->Log(LOG_DEBUG, "settings: streamtype='%i' host='%s', port=%i, timeout=%i", g_eStreamType, g_szHostname.c_str(), g_lPort, g_iConnectTimeout);
  
  dvblinkclient = new DVBLinkClient(XBMC,PVR, g_szClientname, g_szHostname, g_lPort, g_bShowInfoMSG, g_szUsername, g_szPassword, g_bUseTimeshift, g_szTimeShiftBufferPath);

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
  delete dvblinkclient;
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
  else if (str == "client")
  {
    string tmp_sClientname;
    XBMC->Log(LOG_INFO, "Changed Setting 'client' from %s to %s", g_szClientname.c_str(), (const char*) settingValue);
    tmp_sClientname = g_szClientname;
    g_szClientname = (const char*) settingValue;
    if (tmp_sClientname != g_szClientname)
      return ADDON_STATUS_NEED_RESTART;
  }
  else if (str == "username")
  {
    string tmp_sUsername;
    XBMC->Log(LOG_INFO, "Changed Setting 'username' from %s to %s", g_szUsername.c_str(), (const char*) settingValue);
    tmp_sUsername = g_szUsername;
    g_szUsername = (const char*) settingValue;
    if (tmp_sUsername != g_szUsername)
      return ADDON_STATUS_NEED_RESTART;
  }
  else if (str == "password")
  {
    string tmp_sPassword;
    XBMC->Log(LOG_INFO, "Changed Setting 'password' from %s to %s", g_szPassword.c_str(), (const char*) settingValue);
    tmp_sPassword = g_szPassword;
    g_szPassword = (const char*) settingValue;
    if (tmp_sPassword != g_szPassword)
      return ADDON_STATUS_NEED_RESTART;
  }
  else if (str == "streamtype")
  {
    DVBLINK_STREAMTYPE tmp_eStreamtype;
    XBMC->Log(LOG_INFO, "Changed Setting 'streamtype' from %i to %i", g_eStreamType, *(const DVBLINK_STREAMTYPE *) settingValue);
    tmp_eStreamtype = g_eStreamType;
    g_eStreamType = *((const DVBLINK_STREAMTYPE *)settingValue);
    if (tmp_eStreamtype != g_eStreamType)
      return ADDON_STATUS_NEED_RESTART;
  }
  else if (str == "port")
  {
    XBMC->Log(LOG_INFO, "Changed Setting 'port' from %i to %i", g_lPort, *(int*) settingValue);
    if (g_lPort != (long)(*(int*) settingValue))
    {
      g_lPort = (long)(*(int*) settingValue);
      XBMC->Log(LOG_INFO, "Changed Setting 'port' to %i", g_lPort);
      return ADDON_STATUS_NEED_RESTART;
    }
  }
  else if (str == "timeout")
  {
    XBMC->Log(LOG_INFO, "Changed setting 'timeout' from %u to %u", g_iConnectTimeout, *(int*) settingValue);
    g_iConnectTimeout = *(int*) settingValue;
  }
  else if (str == "ch_handle")
  {
    XBMC->Log(LOG_INFO, "Changed Setting 'ch_handle' from %u to %u", g_bUseChlHandle, *(int*) settingValue);
    g_bUseChlHandle = *(bool*) settingValue;
  }
  else if (str == "timeshift")
  {
    XBMC->Log(LOG_INFO, "Changed Setting 'timeshift' from %u to %u", g_bUseTimeshift, *(int*) settingValue);
    g_bUseTimeshift = *(bool*) settingValue;
       return ADDON_STATUS_NEED_RESTART;
  }
  else if (str == "timeshiftpath")
  {
    string tmp_sTimeShiftBufferPath;
    XBMC->Log(LOG_INFO, "Changed Setting 'timeshiftpath' from %s to %s", g_szTimeShiftBufferPath.c_str(), (const char*) settingValue);
    tmp_sTimeShiftBufferPath = g_szTimeShiftBufferPath;
    g_szTimeShiftBufferPath = (const char*) settingValue;
    if (tmp_sTimeShiftBufferPath != g_szTimeShiftBufferPath)
      return ADDON_STATUS_NEED_RESTART;
  }
  else if (str == "showinfomsg")
  {
    XBMC->Log(LOG_INFO, "Changed Setting 'showinfomsg' from %u to %u", g_bShowInfoMSG, *(int*) settingValue);
    g_bShowInfoMSG = *(bool*) settingValue;
  }
  else if (str == "height")
  {
    XBMC->Log(LOG_INFO, "Changed Setting 'height' from %u to %u", g_iHeight, *(int*) settingValue);
    g_iHeight = *(int*) settingValue;
  }
  else if (str == "width")
  {
    XBMC->Log(LOG_INFO, "Changed Setting 'width' from %u to %u", g_iWidth, *(int*) settingValue);
    g_iWidth = *(int*) settingValue;
  }
  else if (str == "bitrate")
  {
    XBMC->Log(LOG_INFO, "Changed Setting 'bitrate' from %u to %u", g_iBitrate, *(int*) settingValue);
    g_iBitrate = *(int*) settingValue;
  }
  else if (str == "audiotrack")
  {
    string tmp_sAudiotrack;
    XBMC->Log(LOG_INFO, "Changed Setting 'audiotrack' from %s to %s", g_szAudiotrack.c_str(), (const char*) settingValue);
    tmp_sAudiotrack = g_szAudiotrack;
    g_szAudiotrack = (const char*) settingValue;
    if (tmp_sAudiotrack != g_szAudiotrack)
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
  pCapabilities->bSupportsRecordings         = true; //TODO: ADD when possible to see recording
  pCapabilities->bSupportsTimers             = true;
  pCapabilities->bSupportsTV                 = true;
  pCapabilities->bSupportsRadio              = true;
  pCapabilities->bHandlesInputStream         = true;
  return PVR_ERROR_NO_ERROR;
}

const char *GetBackendName(void)
{
  static const char *strBackendName = "DVBLink Connect! Server";
  return strBackendName;
}

const char *GetBackendVersion(void)
{
  static  const char * strBackendVersion = "0.2";
  return strBackendVersion;
}

const char *GetConnectionString(void)
{
  return g_szHostname.c_str();
}

PVR_ERROR GetDriveSpace(long long *iTotal, long long *iUsed)
{
 if (dvblinkclient)
 {
   dvblinkclient->GetDriveSpace(iTotal, iUsed);
   return PVR_ERROR_NO_ERROR;
 }
  return PVR_ERROR_SERVER_ERROR;
}

PVR_ERROR GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd)
{
  if (dvblinkclient)
    return dvblinkclient->GetEPGForChannel(handle, channel, iStart, iEnd);

  return PVR_ERROR_SERVER_ERROR;
}

int GetChannelsAmount(void)
{
  if (dvblinkclient)
    return dvblinkclient->GetChannelsAmount();

  return -1;
}

PVR_ERROR GetChannels(ADDON_HANDLE handle, bool bRadio)
{
  if (dvblinkclient)
    return dvblinkclient->GetChannels(handle, bRadio);

  return PVR_ERROR_SERVER_ERROR;
}

bool OpenLiveStream(const PVR_CHANNEL &channel)
{
  if (dvblinkclient)
    return dvblinkclient->OpenLiveStream(channel, g_eStreamType, g_iWidth, g_iHeight, g_iBitrate, g_szAudiotrack);
  return false;
}

void CloseLiveStream(void)
{
  if (dvblinkclient)
    dvblinkclient->StopStreaming(g_bUseChlHandle);
}

const char * GetLiveStreamURL(const PVR_CHANNEL &channel)
{
  if (dvblinkclient)
    return dvblinkclient->GetLiveStreamURL(channel, g_eStreamType, g_iWidth, g_iHeight, g_iBitrate, g_szAudiotrack);
  return "";
}

int ReadLiveStream(unsigned char *pBuffer, unsigned int iBufferSize)
{
  if (dvblinkclient)
    return dvblinkclient->ReadLiveStream(pBuffer,iBufferSize);
  return 0;
}

long long SeekLiveStream(long long iPosition, int iWhence /* = SEEK_SET */)
{
  if (dvblinkclient)
    return dvblinkclient->SeekLiveStream(iPosition,iWhence);
  return -1;
}

long long PositionLiveStream(void)
{
  if (dvblinkclient)
    return dvblinkclient->PositionLiveStream();
  return -1;
}

long long LengthLiveStream(void)
{
  if (dvblinkclient)
    return dvblinkclient->LengthLiveStream();
  return -1;
}

void PauseStream(bool bPaused)
{
}

bool CanPauseStream(void)
{
  return g_bUseTimeshift;
}

bool CanSeekStream(void)
{
  return g_bUseTimeshift;
}

int GetTimersAmount(void)
{
  if (dvblinkclient)
    return dvblinkclient->GetTimersAmount();

  return -1;
}

PVR_ERROR GetTimers(ADDON_HANDLE handle)
{
  if (dvblinkclient)
    return dvblinkclient->GetTimers(handle); 

  return PVR_ERROR_FAILED;
}

PVR_ERROR AddTimer(const PVR_TIMER &timer) 
{
  if (dvblinkclient)
    return dvblinkclient->AddTimer(timer);

  return PVR_ERROR_FAILED;
}

PVR_ERROR DeleteTimer(const PVR_TIMER &timer, bool bForceDelete)
{
  if (dvblinkclient)
    return dvblinkclient->DeleteTimer(timer);

  return PVR_ERROR_FAILED;
}

PVR_ERROR UpdateTimer(const PVR_TIMER &timer) 
{
  if (dvblinkclient)
    return dvblinkclient->UpdateTimer(timer);

  return PVR_ERROR_FAILED;
}


int GetRecordingsAmount(void)
{
  if (dvblinkclient)
    return dvblinkclient->GetRecordingsAmount();

  return -1;
}

PVR_ERROR GetRecordings(ADDON_HANDLE handle) 
{
  if (dvblinkclient)
    return dvblinkclient->GetRecordings(handle); 

  return PVR_ERROR_FAILED;
}

PVR_ERROR DeleteRecording(const PVR_RECORDING &recording)
{
  if (dvblinkclient)
    return dvblinkclient->DeleteRecording(recording); 

  return PVR_ERROR_FAILED; 
}

PVR_ERROR GetRecordingEdl(const PVR_RECORDING&, PVR_EDL_ENTRY[], int*) 
{ 
  return PVR_ERROR_NOT_IMPLEMENTED;
}

int GetCurrentClientChannel(void)
{
  if (dvblinkclient)
    return dvblinkclient->GetCurrentChannelId();

  return 0;
}

bool SwitchChannel(const PVR_CHANNEL &channel)
{
  CloseLiveStream();
  return OpenLiveStream(channel);
}

PVR_ERROR SignalStatus(PVR_SIGNAL_STATUS &signalStatus)
{
  return PVR_ERROR_NO_ERROR;
}

/** UNUSED API FUNCTIONS */

PVR_ERROR GetStreamProperties(PVR_STREAM_PROPERTIES* pProperties)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}


PVR_ERROR DialogChannelScan(void)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}

PVR_ERROR CallMenuHook(const PVR_MENUHOOK &menuhook, const PVR_MENUHOOK_DATA &item)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}

PVR_ERROR DeleteChannel(const PVR_CHANNEL &channel)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}

PVR_ERROR RenameChannel(const PVR_CHANNEL &channel)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}

PVR_ERROR MoveChannel(const PVR_CHANNEL &channel)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}

PVR_ERROR DialogChannelSettings(const PVR_CHANNEL &channel)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}

PVR_ERROR DialogAddChannel(const PVR_CHANNEL &channel)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}

int GetChannelGroupsAmount(void)
{
  return -1;
}

PVR_ERROR GetChannelGroups(ADDON_HANDLE handle, bool bRadio)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}

PVR_ERROR GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}


bool OpenRecordedStream(const PVR_RECORDING &recording)
{
  return false;
}
void CloseRecordedStream(void)
{
}

int ReadRecordedStream(unsigned char *pBuffer, unsigned int iBufferSize)
{
  return 0;
}
long long SeekRecordedStream(long long iPosition, int iWhence /* = SEEK_SET */)
{
  return 0;
}

long long PositionRecordedStream(void)
{
  return -1;
}

long long LengthRecordedStream(void)
{
  return 0;
}

void DemuxReset(void)
{
}

void DemuxFlush(void)
{
}


PVR_ERROR RenameRecording(const PVR_RECORDING &recording)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}

PVR_ERROR SetRecordingPlayCount(const PVR_RECORDING &recording, int count)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}

PVR_ERROR SetRecordingLastPlayedPosition(const PVR_RECORDING &recording, int lastplayedposition)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}

int GetRecordingLastPlayedPosition(const PVR_RECORDING &recording)
{
  return -1;
}

void DemuxAbort(void)
{
}

DemuxPacket* DemuxRead(void)
{
  return NULL;
}

unsigned int GetChannelSwitchDelay(void)
{
  return 0;
}



bool SeekTime(int,bool,double*)
{
  return false;
}

void SetSpeed(int)
{
}

time_t GetPlayingTime() { return 0; }
time_t GetBufferTimeStart() { return 0; }
time_t GetBufferTimeEnd() { return 0; }

}
