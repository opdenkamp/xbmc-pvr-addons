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
 *  the Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301  USA
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "client.h"
#include "xbmc_pvr_dll.h"
#include "DVBLinkClient.h"
#include "platform/util/util.h"
#include "platform/util/timeutils.h"
#include "RecordingStreamer.h"

using namespace std;
using namespace ADDON;

#ifdef TARGET_WINDOWS
#define snprintf _snprintf
#endif

ADDON_STATUS   m_CurStatus          = ADDON_STATUS_UNKNOWN;


std::string g_strUserPath           = "";
std::string g_strClientPath         = "";

DVBLinkClient* dvblinkclient = NULL;
RecordingStreamer* recording_streamer = NULL;

std::string g_szHostname            = DEFAULT_HOST;                  ///< The Host name or IP of the DVBLink Server
long        g_lPort                 = DEFAULT_PORT;                  ///< The DVBLink Connect Server listening port (default: 8080)
bool        g_bUseTranscoding		= DEFAULT_USETRANSCODING;        ///< Use transcoding
std::string g_szClientname;                                          ///< Name of dvblink client
std::string g_szUsername            = DEFAULT_USERNAME;              ///< Username
std::string g_szPassword            = DEFAULT_PASSWORD;              ///< Password
bool        g_bShowInfoMSG          = DEFAULT_SHOWINFOMSG;           ///< Show information messages
int         g_iHeight               = DEFAULT_HEIGHT;                ///< Height of stream when using transcoding (0: autodetect)
int         g_iWidth                = DEFAULT_WIDTH;                 ///< Width of stream when using transcoding (0: autodetect)
int         g_iBitrate              = DEFAULT_BITRATE;               ///< Bitrate of stream when using transcoding
std::string g_szAudiotrack          = DEFAULT_AUDIOTRACK;            ///< Audiotrack to include in stream when using transcoding
bool        g_bUseTimeshift         = DEFAULT_USETIMESHIFT;          ///< Use timeshift
bool        g_bAddRecEpisode2title  = DEFAULT_ADDRECEPISODE2TITLE;   ///< Concatenate title and episode info for recordings
bool        g_bGroupRecBySeries = DEFAULT_GROUPRECBYSERIES;         ///< Group Recordings as Directories by series
CHelper_libXBMC_addon *XBMC = NULL;
CHelper_libXBMC_pvr   *PVR          = NULL;
CHelper_libXBMC_gui   *GUI          = NULL;

extern "C" {

static void generate_uuid(std::string& uuid)
{
  int64_t seed_value = PLATFORM::GetTimeMs();
  seed_value = seed_value % 1000000000;
  srand((unsigned int)seed_value);

  //fill in uuid string from a template
  std::string template_str = "xxxx-xx-xx-xx-xxxxxx";
  for (size_t i=0; i<template_str.size(); i++)
  {
    if (template_str[i] != '-')
    {
      double a1 = rand();
      double a3 = RAND_MAX;
      unsigned char ch = (unsigned char)(a1 * 255 / a3);
      char buf[16];
      sprintf(buf, "%02x", ch);
      uuid += buf;
		} else
		{
			uuid += '-';
		}
  }
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

  PVR = new CHelper_libXBMC_pvr;
  if (!PVR->RegisterMe(hdl))
  {
    SAFE_DELETE(PVR);
    SAFE_DELETE(XBMC);
    return ADDON_STATUS_PERMANENT_FAILURE;
  }

  GUI = new CHelper_libXBMC_gui;
  if (!GUI->RegisterMe(hdl))
  {
      SAFE_DELETE(GUI);
      SAFE_DELETE(PVR);
      SAFE_DELETE(XBMC);
      return ADDON_STATUS_PERMANENT_FAILURE;
  }

  XBMC->Log(LOG_DEBUG, "%s - Creating the PVR DVBlink add-on", __FUNCTION__);

  //generate a guid to use as a client identification
  generate_uuid(g_szClientname);
  XBMC->Log(LOG_NOTICE, "Generated guid %s to use as a DVBLink client ID", g_szClientname.c_str());

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

  /* Read setting "enable_transcoding" from settings.xml */
  if (!XBMC->GetSetting("enable_transcoding", &g_bUseTranscoding))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'enable_transcoding' setting, falling back to false as default");
	g_bUseTranscoding = DEFAULT_USETRANSCODING;
  }

  /* Read setting "port" from settings.xml */
  if (!XBMC->GetSetting("port", &g_lPort))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'port' setting, falling back to '8080' as default");
    g_lPort = DEFAULT_PORT;
  }

  /* Read setting "timeshift" from settings.xml */
  if (!XBMC->GetSetting("timeshift", &g_bUseTimeshift))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'timeshift' setting, falling back to 'false' as default");
    g_bUseTimeshift = DEFAULT_USETIMESHIFT;
  }

  /* Read setting "shoow info message" from settings.xml */
  if (!XBMC->GetSetting("showinfomsg", &g_bShowInfoMSG))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'showinfomsg' setting, falling back to 'true' as default");
    g_bShowInfoMSG = DEFAULT_SHOWINFOMSG;
  }

  /* Read setting "Add episode name to title for recordings" from settings.xml */
  if (!XBMC->GetSetting("add_rec_episode_info", &g_bAddRecEpisode2title))
  {
      /* If setting is unknown fallback to defaults */
      XBMC->Log(LOG_ERROR, "Couldn't get 'add_rec_episode_info' setting, falling back to 'true' as default");
      g_bAddRecEpisode2title = DEFAULT_ADDRECEPISODE2TITLE;
  }

  /* Read setting "Group recordings by title" from settings.xml */
  if (!XBMC->GetSetting("group_recordings_by_series", &g_bGroupRecBySeries))
  {
      /* If setting is unknown fallback to defaults */
      XBMC->Log(LOG_ERROR, "Couldn't get 'group_recordings_by_series' setting, falling back to 'true' as default");
      g_bGroupRecBySeries = DEFAULT_GROUPRECBYSERIES;
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
  XBMC->Log(LOG_DEBUG, "settings: enable_transcoding='%i' host='%s', port=%i", g_bUseTranscoding, g_szHostname.c_str(), g_lPort);
  
  dvblinkclient = new DVBLinkClient(XBMC, PVR, GUI, g_szClientname, g_szHostname, g_lPort, g_bShowInfoMSG, g_szUsername, g_szPassword, g_bAddRecEpisode2title, g_bGroupRecBySeries);

    if (dvblinkclient->GetStatus())
        m_CurStatus = ADDON_STATUS_OK;
    else
        m_CurStatus = ADDON_STATUS_LOST_CONNECTION;


  return m_CurStatus;
}

ADDON_STATUS ADDON_GetStatus()
{
  return m_CurStatus;
}

void ADDON_Destroy()
{
  delete dvblinkclient;
  m_CurStatus = ADDON_STATUS_UNKNOWN;
  SAFE_DELETE(PVR);
  SAFE_DELETE(XBMC);
  SAFE_DELETE(GUI);
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
  else if (str == "enable_transcoding")
  {
	XBMC->Log(LOG_INFO, "Changed Setting 'enable_transcoding' from %u to %u", g_bUseTranscoding, *(int*)settingValue);
	g_bUseTranscoding = *(bool*) settingValue;
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
  else if (str == "timeshift")
  {
    XBMC->Log(LOG_INFO, "Changed Setting 'timeshift' from %u to %u", g_bUseTimeshift, *(int*) settingValue);
    g_bUseTimeshift = *(bool*) settingValue;
    return ADDON_STATUS_NEED_RESTART;
  }
  else if (str == "showinfomsg")
  {
    XBMC->Log(LOG_INFO, "Changed Setting 'showinfomsg' from %u to %u", g_bShowInfoMSG, *(int*) settingValue);
    g_bShowInfoMSG = *(bool*) settingValue;
  }
  else if (str == "add_rec_episode_info")
  {
      XBMC->Log(LOG_INFO, "Changed Setting 'add_rec_episode_info' from %u to %u", g_bAddRecEpisode2title, *(int*)settingValue);
      g_bAddRecEpisode2title = *(bool*)settingValue;
      return ADDON_STATUS_NEED_RESTART;
  }
  else if (str == "group_recordings_by_series")
  {
      XBMC->Log(LOG_INFO, "Changed Setting 'group_recordings_by_series' from %u to %u", g_bGroupRecBySeries, *(int*)settingValue);
      g_bGroupRecBySeries = *(bool*)settingValue;
      return ADDON_STATUS_NEED_RESTART;
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
  pCapabilities->bSupportsRecordings         = true;
  pCapabilities->bSupportsTimers             = true;
  pCapabilities->bSupportsTV                 = true;
  pCapabilities->bSupportsRadio              = true;
  pCapabilities->bHandlesInputStream         = true;
  return PVR_ERROR_NO_ERROR;
}

const char *GetBackendName(void)
{
  static const char *strBackendName = "DVBLink Server";
  return strBackendName;
}

const char *GetBackendVersion(void)
{
  static  const char * strBackendVersion = "5.x";
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
    if (dvblinkclient->GetStatus())
    {
        dvblinkclient->GetDriveSpace(iTotal, iUsed);
        return PVR_ERROR_NO_ERROR;
    }
 }
  return PVR_ERROR_SERVER_ERROR;
}

PVR_ERROR GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd)
{
    if (dvblinkclient)
    {
        if (dvblinkclient->GetStatus())
        {
            return dvblinkclient->GetEPGForChannel(handle, channel, iStart, iEnd);
        }
    }

  return PVR_ERROR_SERVER_ERROR;
}

int GetChannelsAmount(void)
{
    if (dvblinkclient)
    {
        if (dvblinkclient->GetStatus())
        {
            return dvblinkclient->GetChannelsAmount();
        }
        else
        {
            return PVR_ERROR_SERVER_ERROR;
        }
    }
    return -1;
}

PVR_ERROR GetChannels(ADDON_HANDLE handle, bool bRadio)
{
    if (dvblinkclient)
    {
        if (dvblinkclient->GetStatus())
        {
            return dvblinkclient->GetChannels(handle, bRadio);
        }
    }

    return PVR_ERROR_SERVER_ERROR;
}

// live / timshifted stream functions

bool OpenLiveStream(const PVR_CHANNEL &channel)
{
  if (dvblinkclient)
      return dvblinkclient->OpenLiveStream(channel, g_bUseTimeshift, g_bUseTranscoding, g_iWidth, g_iHeight, g_iBitrate, g_szAudiotrack);
  return false;
}

void CloseLiveStream(void)
{
  if (dvblinkclient)
    dvblinkclient->StopStreaming(true);
}

const char * GetLiveStreamURL(const PVR_CHANNEL &channel)
{
//not implemented. OpenLiveStream is always used
  return NULL;
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

time_t GetPlayingTime()
{
	if (dvblinkclient)
		return dvblinkclient->GetPlayingTime();
	return 0;
}

time_t GetBufferTimeStart()
{
	if (dvblinkclient)
		return dvblinkclient->GetBufferTimeStart();
	return 0;
}

time_t GetBufferTimeEnd()
{
	if (dvblinkclient)
		return dvblinkclient->GetBufferTimeEnd();
	return 0;
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

//recording timers functions

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

//recording functions

bool OpenRecordedStream(const PVR_RECORDING &recording)
{
    //close previous stream to be sure
    CloseRecordedStream();

    bool ret_val = false;
    std::string url;
    if (dvblinkclient->GetRecordingURL(recording.strRecordingId, url))
    {
        recording_streamer = new RecordingStreamer(XBMC, g_szClientname, g_szHostname, g_lPort, g_szUsername, g_szPassword);
        if (recording_streamer->OpenRecordedStream(recording.strRecordingId, url))
        {
            ret_val = true;
        }
        else
        {
            delete recording_streamer;
            recording_streamer = NULL;
        }
    }
    return ret_val;
}

void CloseRecordedStream(void)
{
    if (recording_streamer != NULL)
    {
        recording_streamer->CloseRecordedStream();
        delete recording_streamer;
        recording_streamer = NULL;
    }
}

int ReadRecordedStream(unsigned char *pBuffer, unsigned int iBufferSize)
{
    if (recording_streamer != NULL)
        return recording_streamer->ReadRecordedStream(pBuffer, iBufferSize);

    return -1;
}

long long SeekRecordedStream(long long iPosition, int iWhence /* = SEEK_SET */)
{
    if (recording_streamer != NULL)
        return recording_streamer->SeekRecordedStream(iPosition, iWhence);

    return -1;
}

long long PositionRecordedStream(void)
{
    if (recording_streamer != NULL)
        return recording_streamer->PositionRecordedStream();

    return -1;
}

long long LengthRecordedStream(void)
{
    if (recording_streamer != NULL)
        return recording_streamer->LengthRecordedStream();

    return -1;
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

}
