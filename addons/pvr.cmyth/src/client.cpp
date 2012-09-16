/*
 *      Copyright (C) 2005-2012 Team XBMC
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

#include "client.h"
#include "xbmc_pvr_dll.h"
#include "PVRcmyth.h"

using namespace std;
using namespace ADDON;

#ifdef TARGET_WINDOWS
#define snprintf _snprintf
#endif

bool            m_bCreated       = false;
ADDON_STATUS    m_CurStatus      = ADDON_STATUS_UNKNOWN;
PVRcmyth       *g_cmyth          = NULL;
bool            m_bIsPlaying     = false;
PVRcmythChannel m_currentChannel;

/* User adjustable settings are saved here.
 * Default values are defined inside client.h
 * and exported to the other source files.
 */
std::string g_strUserPath             = "";
std::string g_strClientPath           = "";

std::string  g_strHostname            = DEFAULT_HOST;
int          g_iMythPort              = DEFAULT_PORT;
std::string  g_strMythDBuser          = DEFAULT_DB_USER;
std::string  g_strMythDBpassword      = DEFAULT_DB_PASSWORD;
std::string  g_strMythDBname          = DEFAULT_DB_NAME;
bool         g_bExtraDebug            = DEFAULT_EXTRA_DEBUG;
bool         g_bLiveTVPriority        = DEFAULT_LIVETV_PRIORITY;
int          g_iMinMovieLength        = DEFAULT_MIN_MOVIE_LENGTH;
std::string  g_strSeriesRegEx         = DEFAULT_SERIES_REGEX;
std::string  g_strSeriesIdentifier    = DEFAULT_SERIES_IDENTIFIER;

CHelper_libXBMC_addon *XBMC           = NULL;
CHelper_libXBMC_pvr   *PVR            = NULL;
//CHelper_libcmyth      *CMYTH          = NULL;

extern "C" {

void ADDON_ReadSettings(void)
{
  /* Read setting "host" from settings.xml */
  char * buffer;
  buffer = (char*) malloc (1024);
  buffer[0] = 0; /* Set the end of string */

  if (XBMC->GetSetting("host", buffer))
    g_strHostname = buffer;
  else
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'host' setting, falling back to '%s' as default", DEFAULT_HOST);
    g_strHostname = DEFAULT_HOST;
  }
  buffer[0] = 0;

  /* Read setting "port" from settings.xml */
  if (!XBMC->GetSetting("port", &g_iMythPort))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'port' setting, falling back to '%i' as default", DEFAULT_PORT);
    g_iMythPort = DEFAULT_PORT;
  }

    /* Read setting "extradebug" from settings.xml */
  if (!XBMC->GetSetting("extradebug", &g_bExtraDebug))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'extradebug' setting, falling back to '%b' as default", DEFAULT_EXTRA_DEBUG);
    g_bExtraDebug = DEFAULT_EXTRA_DEBUG;
  }

    /* Read setting "db_username" from settings.xml */
  if (XBMC->GetSetting("db_user", buffer))
    g_strMythDBuser = buffer;
  else
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'db_user' setting, falling back to '%s' as default", DEFAULT_DB_USER);
    g_strMythDBuser = DEFAULT_DB_USER;
  }
  buffer[0] = 0;
  
    /* Read setting "db_password" from settings.xml */
  if (XBMC->GetSetting("db_password", buffer))
    g_strMythDBpassword = buffer;
  else
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'db_password' setting, falling back to '%s' as default", DEFAULT_DB_PASSWORD);
    g_strMythDBpassword = DEFAULT_DB_PASSWORD;
  }
  buffer[0] = 0;
  
  /* Read setting "db_name" from settings.xml */
  if (XBMC->GetSetting("db_name", buffer))
    g_strMythDBname = buffer;
  else
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'db_name' setting, falling back to '%s' as default", DEFAULT_DB_NAME);
    g_strMythDBname = DEFAULT_DB_NAME;
  }
  buffer[0] = 0;

  /* Read setting "minmovielength" from settings.xml */
  if (!XBMC->GetSetting("min_movie_length", &g_iMinMovieLength))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'min_movie_length' setting, falling back to '%i' as default", DEFAULT_MIN_MOVIE_LENGTH);
    g_iMinMovieLength = DEFAULT_MIN_MOVIE_LENGTH;
  }
  
  /* Read setting "db_name" from settings.xml */
  if (XBMC->GetSetting("series_regex", buffer))
    g_strSeriesRegEx = buffer;
  else
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'series_regex' setting, falling back to '%s' as default", DEFAULT_SERIES_REGEX);
    g_strSeriesRegEx = DEFAULT_SERIES_REGEX;
  }
  buffer[0] = 0;

  /* Read setting "db_name" from settings.xml */
  if (XBMC->GetSetting("series_regex_id", buffer))
    g_strSeriesIdentifier = buffer;
  else
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'series_regex_id' setting, falling back to '%s' as default", DEFAULT_SERIES_IDENTIFIER);
    g_strSeriesIdentifier = DEFAULT_SERIES_IDENTIFIER;
  }
  buffer[0] = 0;

  free (buffer);
  
}

ADDON_STATUS ADDON_Create(void* hdl, void* props)
{
  if (!hdl || !props)
    return ADDON_STATUS_UNKNOWN;

  PVR_PROPERTIES* pvrprops = (PVR_PROPERTIES*)props;

  XBMC = new CHelper_libXBMC_addon;
  if (!XBMC->RegisterMe(hdl))
    return ADDON_STATUS_UNKNOWN;

  PVR = new CHelper_libXBMC_pvr;
  if (!PVR->RegisterMe(hdl))
    return ADDON_STATUS_UNKNOWN;

  XBMC->Log(LOG_DEBUG, "%s - Creating the PVR cmyth add-on", __FUNCTION__);

  //XBMC->Log(LOG_DEBUG, "Loading cmyth library...");
  //CMYTH = new CHelper_libcmyth;
  //if (!CMYTH->RegisterMe(hdl))
  //{
  //  XBMC->Log(LOG_ERROR, "Failed to load cmyth library!");
  //  return ADDON_STATUS_UNKNOWN;
  //}
  
  m_CurStatus     = ADDON_STATUS_UNKNOWN;
  g_strUserPath   = pvrprops->strUserPath;
  g_strClientPath = pvrprops->strClientPath;

  ADDON_ReadSettings();

  XBMC->Log(LOG_DEBUG,"Creating cmyth client...");
  g_cmyth = new PVRcmyth;
  if (!g_cmyth->Connect())
  {
    XBMC->Log(LOG_ERROR,"Failed to connect to backend");
	m_CurStatus = ADDON_STATUS_LOST_CONNECTION;
    return m_CurStatus;
  }
  XBMC->Log(LOG_DEBUG,"Creating cmyth client done");

  /* Read setting "LiveTV Priority" from backend database */
  bool savedLiveTVPriority;
  if(!XBMC->GetSetting("livetv_priority", &savedLiveTVPriority))
    savedLiveTVPriority = DEFAULT_LIVETV_PRIORITY;
  g_bLiveTVPriority = g_cmyth->GetLiveTVPriority();

  if (g_bLiveTVPriority != savedLiveTVPriority)
  {
    g_cmyth->SetLiveTVPriority(savedLiveTVPriority);
  }
  
  m_CurStatus = ADDON_STATUS_OK;
  m_bCreated = true;
 
  XBMC->Log(LOG_DEBUG, "PVR cmyth successfully created");
  return m_CurStatus;
}

ADDON_STATUS ADDON_GetStatus()
{
  return m_CurStatus;
}

void ADDON_Destroy()
{
  delete g_cmyth;
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
  return ADDON_STATUS_OK;
}

void ADDON_Stop()
{
}

void ADDON_FreeSettings()
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

PVR_ERROR GetAddonCapabilities(PVR_ADDON_CAPABILITIES* pCapabilities)
{
  pCapabilities->bSupportsEPG                = true;
  pCapabilities->bSupportsTV                 = true;
  pCapabilities->bSupportsRadio              = true;
  pCapabilities->bSupportsChannelGroups      = true;
  pCapabilities->bHandlesInputStream         = true;
  //pCapabilities->bSupportsChannelSettings    = false;
  //pCapabilities->bHandlesDemuxing            = false;
  //pCapabilities->bSupportsChannelScan        = false;
  pCapabilities->bSupportsRecordingPlayCount = true;
  //pCapabilities->bSupportsLastPlayedPosition = false;
    
  return PVR_ERROR_NO_ERROR;
}

const char *GetBackendName(void)
{
  static const char *strBackendName = "pulse-eight demo pvr add-on";
  return strBackendName;
}

const char *GetBackendVersion(void)
{
  static CStdString strBackendVersion = "0.1";
  return strBackendVersion.c_str();
}

const char *GetConnectionString(void)
{
  static CStdString strConnectionString = "connected";
  return strConnectionString.c_str();
}

PVR_ERROR GetDriveSpace(long long *iTotal, long long *iUsed)
{
  *iTotal = 1024 * 1024 * 1024;
  *iUsed  = 0;
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd)
{
  if (g_cmyth)
    return g_cmyth->GetEPGForChannel(handle, channel, iStart, iEnd);

  return PVR_ERROR_SERVER_ERROR;
}

int GetChannelsAmount(void)
{
  if (g_cmyth)
    return g_cmyth->GetChannelsAmount();

  return -1;
}

PVR_ERROR GetChannels(ADDON_HANDLE handle, bool bRadio)
{
  if (g_cmyth)
    return g_cmyth->GetChannels(handle, bRadio);

  return PVR_ERROR_SERVER_ERROR;
}

bool OpenLiveStream(const PVR_CHANNEL &channel)
{
  if (g_cmyth)
  {
    if (m_bIsPlaying)
    {
      CloseLiveStream();
    }

    if (g_cmyth->GetChannel(channel, m_currentChannel))
    {
XBMC->Log(LOG_INFO,"%s: CMYTH Channel exists.",__FUNCTION__);
      m_bIsPlaying = g_cmyth->OpenLiveStream(channel);
      return m_bIsPlaying;
    }
  }
XBMC->Log(LOG_ERROR,"%s: CMYTH cannot open stream.",__FUNCTION__);
  return false;
}

void CloseLiveStream(void)
{
  if (g_cmyth)
  {
    g_cmyth->CloseLiveStream();
    m_bIsPlaying = false;
  }
}

int GetCurrentClientChannel(void)
{
  return m_currentChannel.iUniqueId;
}

bool SwitchChannel(const PVR_CHANNEL &channel)
{
/*
  if (g_cmyth)
  {    
    if (g_cmyth->GetChannel(channel, m_currentChannel) && g_cmyth->SwitchChannel(channel))
    return true;
    XBMC->QueueNotification(QUEUE_WARNING,"Failed to change channel. No free tuners?");  
  }
  return false;
*/
  CloseLiveStream();
  return OpenLiveStream(channel);
}

int ReadLiveStream(unsigned char *pBuffer, unsigned int iBufferSize)
{
  if (g_cmyth)
  {
    int dataread=g_cmyth->ReadLiveStream(pBuffer,iBufferSize);
    if(dataread<0)
    {
      XBMC->Log(LOG_ERROR,"%s: Failed to read liveStream. Errorcode: %i!",__FUNCTION__,dataread);
    }
    return dataread;
  }
  return -1;
}

long long SeekLiveStream(long long iPosition, int iWhence /* = SEEK_SET */)
{
   if (g_cmyth)
   {
     return g_cmyth->SeekLiveStream(iPosition,iWhence);
   }
  return -1;
}

long long PositionLiveStream(void)
{
  if (g_cmyth)
   {
     //return g_cmyth->PositionLiveStream();
     return g_cmyth->SeekLiveStream(0,SEEK_CUR);
   }
  return -1;
}

long long LengthLiveStream(void)
{
  if (g_cmyth)
  {
    return g_cmyth->LengthLiveStream();
  }
  return -1;
}

const char * GetLiveStreamURL(const PVR_CHANNEL &channel)
{
  return "";  
}

PVR_ERROR GetStreamProperties(PVR_STREAM_PROPERTIES* pProperties)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}

int GetChannelGroupsAmount(void)
{
  if (g_cmyth)
    return g_cmyth->GetChannelGroupsAmount();

  return -1;
}

PVR_ERROR GetChannelGroups(ADDON_HANDLE handle, bool bRadio)
{
  if (g_cmyth)
    return g_cmyth->GetChannelGroups(handle, bRadio);

  return PVR_ERROR_SERVER_ERROR;
}

PVR_ERROR GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group)
{
  if (g_cmyth)
    return g_cmyth->GetChannelGroupMembers(handle, group);

  return PVR_ERROR_SERVER_ERROR;
}

PVR_ERROR SignalStatus(PVR_SIGNAL_STATUS &signalStatus)
{
  if (g_cmyth)
    return g_cmyth->SignalStatus(signalStatus);

  return PVR_ERROR_SERVER_ERROR;
}

/** UNUSED API FUNCTIONS */
int GetTimersAmount(void) { return -1; }
PVR_ERROR GetTimers(ADDON_HANDLE handle) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR AddTimer(const PVR_TIMER &timer) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DeleteTimer(const PVR_TIMER &timer, bool bForceDelete) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR UpdateTimer(const PVR_TIMER &timer) { return PVR_ERROR_NOT_IMPLEMENTED; }
int GetRecordingsAmount(void) { return -1; }
PVR_ERROR GetRecordings(ADDON_HANDLE handle) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DeleteRecording(const PVR_RECORDING &recording) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR RenameRecording(const PVR_RECORDING &recording) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR SetRecordingPlayCount(const PVR_RECORDING &recording, int count) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR SetRecordingLastPlayedPosition(const PVR_RECORDING &recording, int lastplayedposition) { return PVR_ERROR_NOT_IMPLEMENTED; }
int GetRecordingLastPlayedPosition(const PVR_RECORDING &recording) { return -1; }
bool OpenRecordedStream(const PVR_RECORDING &recording) { return false; }
void CloseRecordedStream(void) {}
int ReadRecordedStream(unsigned char *pBuffer, unsigned int iBufferSize) { return 0; }
long long SeekRecordedStream(long long iPosition, int iWhence /* = SEEK_SET */) { return 0; }
long long PositionRecordedStream(void) { return -1; }
long long LengthRecordedStream(void) { return 0; }

PVR_ERROR DialogChannelScan(void) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR CallMenuHook(const PVR_MENUHOOK &menuhook) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DeleteChannel(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR RenameChannel(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR MoveChannel(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DialogChannelSettings(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DialogAddChannel(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
void DemuxReset(void) {}
void DemuxFlush(void) {}
void DemuxAbort(void) {}
DemuxPacket* DemuxRead(void) { return NULL; }
unsigned int GetChannelSwitchDelay(void) { return 0; }
}
