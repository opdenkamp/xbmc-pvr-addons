/*
 *      Copyright (C) 2005-2012 Team XBMC
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
#include "pvrclient-mythtv.h"

#include <xbmc_pvr_dll.h>

using namespace std;
using namespace ADDON;

#define SEEK_POSSIBLE 0x10 // Flag used to check if protocol allows seeks

/* User adjustable settings are saved here.
 * Default values are defined inside client.h
 * and exported to the other source files.
 */
CStdString   g_szHostname             = DEFAULT_HOST;             ///< The Host name or IP of the mythtv server
int          g_iMythPort              = DEFAULT_PORT;             ///< The mythtv Port (default is 6543)
CStdString   g_szMythDBuser           = DEFAULT_DB_USER;          ///< The mythtv sql username (default is mythtv)
CStdString   g_szMythDBpassword       = DEFAULT_DB_PASSWORD;      ///< The mythtv sql password (default is mythtv)
CStdString   g_szMythDBname           = DEFAULT_DB_NAME;          ///< The mythtv sql database name (default is mythconverg)
bool         g_bExtraDebug            = DEFAULT_EXTRA_DEBUG;      ///< Output extensive debug information to the log
bool         g_bLiveTV                = DEFAULT_LIVETV;           ///< LiveTV support (or recordings only)
bool         g_bLiveTVPriority        = DEFAULT_LIVETV_PRIORITY;  ///< MythTV Backend setting to allow live TV to move scheduled shows

///* Client member variables */
bool         m_recordingFirstRead;
char         m_noSignalStreamData[6 + 0xffff];
long         m_noSignalStreamSize     = 0;
long         m_noSignalStreamReadPos  = 0;
bool         m_bPlayingNoSignal       = false;
int          m_iCurrentChannel        = 1;
ADDON_STATUS m_CurStatus              = ADDON_STATUS_UNKNOWN;
bool         g_bCreated               = false;
int          g_iClientID              = -1;
CStdString   g_szUserPath             = "";
CStdString   g_szClientPath           = "";

PVRClientMythTV       *g_client       = NULL;

CHelper_libXBMC_addon *XBMC           = NULL;
CHelper_libXBMC_pvr   *PVR            = NULL;
CHelper_libXBMC_gui   *GUI            = NULL;

extern "C" {

/***********************************************************
 * Standard AddOn related public library functions
 ***********************************************************/

ADDON_STATUS ADDON_Create(void *hdl, void *props)
{
  if (!hdl)
    return ADDON_STATUS_PERMANENT_FAILURE;

  // Register handles
  XBMC = new CHelper_libXBMC_addon;

  if (!XBMC->RegisterMe(hdl))
    return ADDON_STATUS_PERMANENT_FAILURE;

  XBMC->Log(LOG_DEBUG, "Creating MythTV cmyth PVR-Client");

  XBMC->Log(LOG_DEBUG, "Register handle @ libXBMC_addon...done");
  
  XBMC->Log(LOG_DEBUG, "Checking props...");
  if (!props)
  {
    SAFE_DELETE(XBMC);
    return ADDON_STATUS_PERMANENT_FAILURE;
  }
  XBMC->Log(LOG_DEBUG, "Checking props...done");
  PVR_PROPERTIES* pvrprops = (PVR_PROPERTIES*)props;

  XBMC->Log(LOG_DEBUG, "Register handle @ libXBMC_pvr...");
  PVR = new CHelper_libXBMC_pvr;
  if (!PVR->RegisterMe(hdl))
  {
    SAFE_DELETE(XBMC);
    return ADDON_STATUS_PERMANENT_FAILURE;
  }
  XBMC->Log(LOG_DEBUG, "Register handle @ libXBMC_addon...done");

  XBMC->Log(LOG_DEBUG, "Register handle @ libXBMC_gui...");
  GUI = new CHelper_libXBMC_gui;
  if (!GUI->RegisterMe(hdl))
  {
    SAFE_DELETE(PVR);
    SAFE_DELETE(XBMC);
    return ADDON_STATUS_PERMANENT_FAILURE;
  }
  XBMC->Log(LOG_DEBUG, "Register handle @ libXBMC_gui...done");

  m_CurStatus    = ADDON_STATUS_UNKNOWN;
  g_szUserPath   = pvrprops->strUserPath;
  g_szClientPath = pvrprops->strClientPath;

  // Read settings
  char *buffer;
  buffer = (char*)malloc(1024);
  buffer[0] = 0; /* Set the end of string */

  /* Read setting "host" from settings.xml */
  if (XBMC->GetSetting("host", buffer))
    g_szHostname = buffer;
  else
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'host' setting, falling back to '%s' as default", DEFAULT_HOST);
    g_szHostname = DEFAULT_HOST;
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
    g_szMythDBuser = buffer;
  else
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'db_user' setting, falling back to '%s' as default", DEFAULT_DB_USER);
    g_szMythDBuser = DEFAULT_DB_USER;
  }
  buffer[0] = 0;

  /* Read setting "db_password" from settings.xml */
  if (XBMC->GetSetting("db_password", buffer))
    g_szMythDBpassword = buffer;
  else
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'db_password' setting, falling back to '%s' as default", DEFAULT_DB_PASSWORD);
    g_szMythDBpassword = DEFAULT_DB_PASSWORD;
  }
  buffer[0] = 0;
  
  /* Read setting "db_name" from settings.xml */
  if (XBMC->GetSetting("db_name", buffer))
    g_szMythDBname = buffer;
  else
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'db_name' setting, falling back to '%s' as default", DEFAULT_DB_NAME);
    g_szMythDBname = DEFAULT_DB_NAME;
  }
  buffer[0] = 0;

  /* Read setting "LiveTV" from settings.xml */
  if (!XBMC->GetSetting("livetv", &g_bLiveTV))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'livetv' setting, falling back to '%b' as default", DEFAULT_LIVETV);
    g_bLiveTV = DEFAULT_LIVETV;
  }

  free (buffer);

  // Create our addon
  XBMC->Log(LOG_DEBUG,"Creating PVRClientMythTV...");
  g_client = new PVRClientMythTV();
  if (!g_client->Connect())
  {
    XBMC->Log(LOG_ERROR, "Failed to connect to backend");
    SAFE_DELETE(g_client);
    SAFE_DELETE(GUI);
    SAFE_DELETE(PVR);
    SAFE_DELETE(XBMC);
    m_CurStatus = ADDON_STATUS_NEED_SETTINGS;
    return m_CurStatus;
  }
  XBMC->Log(LOG_DEBUG,"Creating PVRClientMythTV...done");

  /* Read setting "LiveTV Priority" from backend database */
  bool savedLiveTVPriority;
  if (!XBMC->GetSetting("livetv_priority", &savedLiveTVPriority))
    savedLiveTVPriority = DEFAULT_LIVETV_PRIORITY;
  g_bLiveTVPriority = g_client->GetLiveTVPriority();
  if (g_bLiveTVPriority != savedLiveTVPriority)
  {
    g_client->SetLiveTVPriority(savedLiveTVPriority);
  }

  XBMC->Log(LOG_DEBUG, "MythTV cmyth PVR-Client successfully created");
  m_CurStatus = ADDON_STATUS_OK;
  g_bCreated = true;
  return m_CurStatus;
}

void ADDON_Destroy()
{
  if (g_bCreated)
  {
    delete g_client;
    g_client = NULL;
    g_bCreated = false;
  }

  if (PVR)
  {
    delete(PVR);
    PVR = NULL;
  }

  if (XBMC)
  {
    delete(XBMC);
    XBMC = NULL;
  }

  if (GUI)
  {
    delete(GUI);
    GUI = NULL;
  }

  m_CurStatus = ADDON_STATUS_UNKNOWN;
}

ADDON_STATUS ADDON_GetStatus()
{
  return m_CurStatus;
}

bool ADDON_HasSettings()
{
  return true;
}

unsigned int ADDON_GetSettings(ADDON_StructSetting ***sSet)
{
  (void)sSet;
  return 0;
}

ADDON_STATUS ADDON_SetSetting(const char *settingName, const void *settingValue)
{
  string str = settingName;
  if (!g_bCreated)
    return ADDON_STATUS_OK;
  
  if (str == "host")
  {
    string tmp_sHostname;
    XBMC->Log(LOG_INFO, "Changed Setting 'host' from %s to %s", g_szHostname.c_str(), (const char*)settingValue);
    tmp_sHostname = g_szHostname;
    g_szHostname = (const char*)settingValue;
    if (tmp_sHostname != g_szHostname)
      return ADDON_STATUS_NEED_RESTART;
  }
  else if (str == "port")
  {
    XBMC->Log(LOG_INFO, "Changed Setting 'port' from %u to %u", g_iMythPort, *(int*)settingValue);
    if (g_iMythPort != *(int*)settingValue)
    {
      g_iMythPort = *(int*)settingValue;
      return ADDON_STATUS_NEED_RESTART;
    }
  }
  else if (str == "db_user")
  {
    string tmp_sMythDBuser;
    XBMC->Log(LOG_INFO, "Changed Setting 'db_user' from %s to %s", g_szMythDBuser.c_str(), (const char*)settingValue);
    tmp_sMythDBuser = g_szMythDBuser;
    g_szMythDBuser = (const char*)settingValue;
    if (tmp_sMythDBuser != g_szMythDBuser)
      return ADDON_STATUS_NEED_RESTART;
  }
  else if (str == "db_password")
  {
    string tmp_sMythDBpassword;
    XBMC->Log(LOG_INFO, "Changed Setting 'db_password' from %s to %s", g_szMythDBpassword.c_str(), (const char*)settingValue);
    tmp_sMythDBpassword = g_szMythDBpassword;
    g_szMythDBpassword = (const char*)settingValue;
    if (tmp_sMythDBpassword != g_szMythDBpassword)
      return ADDON_STATUS_NEED_RESTART;
  }
  else if (str == "db_name")
  {
    string tmp_sMythDBname;
    XBMC->Log(LOG_INFO, "Changed Setting 'db_name' from %s to %s", g_szMythDBname.c_str(), (const char*)settingValue);
    tmp_sMythDBname = g_szMythDBname;
    g_szMythDBname = (const char*)settingValue;
    if (tmp_sMythDBname != g_szMythDBname)
      return ADDON_STATUS_NEED_RESTART;
  }
  else if (str == "extradebug")
  {
    XBMC->Log(LOG_INFO, "Changed Setting 'extra debug' from %u to %u", g_bExtraDebug, *(bool*)settingValue);
    if (g_bExtraDebug != *(bool*)settingValue)
    {
     g_bExtraDebug = *(bool*)settingValue;
      return ADDON_STATUS_OK;
    }
  }
  else if (str == "livetv")
  {
    XBMC->Log(LOG_INFO, "Changed Setting 'livetv' from %u to %u", g_bLiveTV, *(bool*)settingValue);
    if (g_bLiveTV != *(bool*)settingValue)
    {
      g_bLiveTV = *(bool*)settingValue;
      return ADDON_STATUS_OK;
    }
  }
  else if (str == "livetv_priority")
  {
    XBMC->Log(LOG_INFO, "Changed Setting 'extra debug' from %u to %u", g_bLiveTVPriority, *(bool*)settingValue);
    if (g_bLiveTVPriority != *(bool*) settingValue && m_CurStatus != ADDON_STATUS_LOST_CONNECTION)
    {
      g_bLiveTVPriority = *(bool*)settingValue;
      g_client->SetLiveTVPriority(g_bLiveTVPriority);
      return ADDON_STATUS_OK;
    }
  }
  return ADDON_STATUS_OK;
}

void ADDON_Stop()
{
  //ADDON_Destroy();
}

void ADDON_FreeSettings()
{
  return;
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

PVR_ERROR GetAddonCapabilities(PVR_ADDON_CAPABILITIES *pCapabilities)
{
  if (g_client != NULL)
  {
    pCapabilities->bSupportsTV                 = g_bLiveTV;
    pCapabilities->bSupportsRadio              = g_bLiveTV;
    //pCapabilities->bSupportsChannelSettings    = false;
    pCapabilities->bSupportsChannelGroups      = true;
    pCapabilities->bSupportsChannelScan        = false;
    //pCapabilities->bSupportsTimeshift          = g_bLiveTV;
    pCapabilities->bSupportsEPG                = true;
    pCapabilities->bSupportsTimers             = true;

    pCapabilities->bHandlesInputStream           = true;
    pCapabilities->bHandlesDemuxing              = false;

    pCapabilities->bSupportsRecordings           = true;
    pCapabilities->bSupportsRecordingPlayCount   = true;
    pCapabilities->bSupportsLastPlayedPosition   = true;
    return PVR_ERROR_NO_ERROR;
  }
  else
  {
    return PVR_ERROR_FAILED;
  }
}

PVR_ERROR GetStreamProperties(PVR_STREAM_PROPERTIES* props)
{
  (void)props;
  return PVR_ERROR_NOT_IMPLEMENTED;
}

const char *GetBackendName()
{
  return g_client->GetBackendName();
}

const char *GetBackendVersion()
{
  return g_client->GetBackendVersion();
}

const char *GetConnectionString()
{
  return g_client->GetConnectionString();
}

PVR_ERROR GetDriveSpace(long long *iTotal, long long *iUsed)
{
  return g_client->GetDriveSpace(iTotal,iUsed) ? PVR_ERROR_NO_ERROR : PVR_ERROR_UNKNOWN;
}

PVR_ERROR DialogChannelScan()
{
  return PVR_ERROR_FAILED;
}

PVR_ERROR CallMenuHook(const PVR_MENUHOOK &menuhook)
{
  if (g_client == NULL)
    return PVR_ERROR_SERVER_ERROR;

  return g_client->CallMenuHook(menuhook);
}

/*******************************************/
/** PVR EPG Functions                     **/

PVR_ERROR GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd)
{
  if (g_client == NULL)
    return PVR_ERROR_SERVER_ERROR;

  return g_client->GetEPGForChannel(handle, channel, iStart, iEnd);
}

/*******************************************/
/** PVR Channel Functions                 **/

unsigned int GetChannelSwitchDelay(void)
{
  return 0;
}

int GetChannelsAmount()
{
  if (g_client == NULL)
    return PVR_ERROR_SERVER_ERROR;

  return g_client->GetNumChannels();
}

PVR_ERROR GetChannels(ADDON_HANDLE handle, bool bRadio)
{
  if (g_client == NULL)
    return PVR_ERROR_SERVER_ERROR;

  return g_client->GetChannels(handle, bRadio);
}

PVR_ERROR DeleteChannel(const PVR_CHANNEL &channel)
{
  (void)channel;
  return PVR_ERROR_NOT_IMPLEMENTED;
}

PVR_ERROR RenameChannel(const PVR_CHANNEL &channel)
{
  (void)channel;
  return PVR_ERROR_NOT_IMPLEMENTED;
}

PVR_ERROR MoveChannel(const PVR_CHANNEL &channel)
{
  (void)channel;
  return PVR_ERROR_NOT_IMPLEMENTED;
}

PVR_ERROR DialogChannelSettings(const PVR_CHANNEL &channel)
{
  (void)channel;
  return PVR_ERROR_NOT_IMPLEMENTED;
}

PVR_ERROR DialogAddChannel(const PVR_CHANNEL &channel)
{
  (void)channel;
  return PVR_ERROR_NOT_IMPLEMENTED;
}

int GetChannelGroupsAmount(void)
{
  if (g_client == NULL)
    return -1;

  return g_client->GetChannelGroupsAmount();
}

PVR_ERROR GetChannelGroups(ADDON_HANDLE handle, bool bRadio)
{
  if (g_client == NULL)
    return PVR_ERROR_SERVER_ERROR;

  return g_client->GetChannelGroups(handle, bRadio);
}

PVR_ERROR GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group){
  if (g_client == NULL)
    return PVR_ERROR_SERVER_ERROR;

  return g_client->GetChannelGroupMembers(handle, group);
}


/*******************************************/
/** PVR Recording Functions               **/

int GetRecordingsAmount(void)
{
  if (g_client == NULL)
    return 0;

  return g_client->GetRecordingsAmount();
}

PVR_ERROR GetRecordings(ADDON_HANDLE handle)
{
  if (g_client == NULL)
    return PVR_ERROR_SERVER_ERROR;

  return g_client->GetRecordings(handle);
}

PVR_ERROR DeleteRecording(const PVR_RECORDING &recording)
{
  if (g_client == NULL)
    return PVR_ERROR_SERVER_ERROR;

  return g_client->DeleteRecording(recording);
}

PVR_ERROR RenameRecording(const PVR_RECORDING &recording)
{
  (void)recording;
  return PVR_ERROR_NOT_IMPLEMENTED;
}

PVR_ERROR SetRecordingPlayCount(const PVR_RECORDING &recording, int count)
{
  if (g_client == NULL)
    return PVR_ERROR_SERVER_ERROR;

  return g_client->SetRecordingPlayCount(recording, count);
}

PVR_ERROR SetRecordingLastPlayedPosition(const PVR_RECORDING &recording, int lastplayedposition)
{
  if (g_client == NULL)
    return PVR_ERROR_SERVER_ERROR;

  return g_client->SetRecordingLastPlayedPosition(recording, lastplayedposition);
}

int GetRecordingLastPlayedPosition(const PVR_RECORDING &recording)
{
  if (g_client == NULL)
    return PVR_ERROR_SERVER_ERROR;

  return g_client->GetRecordingLastPlayedPosition(recording);
}


/*******************************************/
/** PVR Timer Functions                   **/

int GetTimersAmount(void)
{
  if (g_client == NULL)
    return PVR_ERROR_SERVER_ERROR;

  return g_client->GetTimersAmount();
}

PVR_ERROR GetTimers(ADDON_HANDLE handle)
{
  if (g_client == NULL)
    return PVR_ERROR_SERVER_ERROR;

  return g_client->GetTimers(handle);
}

PVR_ERROR AddTimer(const PVR_TIMER &timer)
{
  if (g_client == NULL)
    return PVR_ERROR_SERVER_ERROR;

  return g_client->AddTimer(timer);
}

PVR_ERROR DeleteTimer(const PVR_TIMER &timer, bool bForceDelete)
{
  if (g_client == NULL)
    return PVR_ERROR_SERVER_ERROR;

  return g_client->DeleteTimer(timer,bForceDelete);
}

PVR_ERROR UpdateTimer(const PVR_TIMER &timer)
{
  if (g_client == NULL)
    return PVR_ERROR_SERVER_ERROR;

  return g_client->UpdateTimer(timer);
}


/*******************************************/
/** PVR Live Stream Functions             **/

bool OpenLiveStream(const PVR_CHANNEL &channel)
{
  if (g_client == NULL)
    return false;

  return g_client->OpenLiveStream(channel);
}

void CloseLiveStream(void)
{
  if (g_client == NULL)
    return;

  g_client->CloseLiveStream();
}

int ReadLiveStream(unsigned char *pBuffer, unsigned int iBufferSize)
{
  if (g_client == NULL)
    return -1;

  int dataread = g_client->ReadLiveStream(pBuffer, iBufferSize);
  if (dataread < 0)
  {
    XBMC->Log(LOG_ERROR,"%s: Failed to read liveStream. Errorcode: %i!", __FUNCTION__, dataread);
  }
  return dataread;
}

int GetCurrentClientChannel()
{
  if (g_client == NULL)
    return -1;

  return g_client->GetCurrentClientChannel();
}

bool SwitchChannel(const PVR_CHANNEL &channelinfo)
{
  if (g_client == NULL)
    return false;

  if (g_client->SwitchChannel(channelinfo))
    return true;
  else
    XBMC->QueueNotification(QUEUE_WARNING, "Failed to change channel. No free tuners?");

  return false;
}

PVR_ERROR SignalStatus(PVR_SIGNAL_STATUS &signalStatus)
{
  if (g_client == NULL)
    return PVR_ERROR_SERVER_ERROR;

  return g_client->SignalStatus(signalStatus);
}

void PauseStream(bool bPaused)
{
  (void)bPaused;
}

bool CanPauseStream(void)
{
  return true;
}

bool CanSeekStream(void)
{
  return true;
}

long long SeekLiveStream(long long iPosition, int iWhence) 
{
  if (g_client == NULL)
    return -1;

  return g_client->SeekLiveStream(iPosition,iWhence);
}

long long PositionLiveStream(void) 
{
  if (g_client == NULL)
    return -1;

  return g_client->SeekLiveStream(0,SEEK_CUR);
}

long long LengthLiveStream(void) 
{
  if (g_client == NULL)
    return -1;

  return g_client->LengthLiveStream();
}


/*******************************************/
/** PVR Recording Stream Functions        **/

bool OpenRecordedStream(const PVR_RECORDING &recinfo)
{
  if (g_client == NULL)
    return false;

  return g_client->OpenRecordedStream(recinfo);
}

void CloseRecordedStream(void)
{
  if (g_client == NULL)
    return;

  g_client->CloseRecordedStream();
}

int ReadRecordedStream(unsigned char *pBuffer, unsigned int iBufferSize)
{
  if (g_client == NULL)
    return -1;

  return g_client->ReadRecordedStream(pBuffer, iBufferSize);
}

long long SeekRecordedStream(long long iPosition, int iWhence)
{
  if (g_client == NULL)
    return -1;

  return g_client->SeekRecordedStream(iPosition, iWhence);
}

long long PositionRecordedStream(void)
{
  if (g_client == NULL)
    return -1;

  return g_client->SeekRecordedStream(0, SEEK_CUR);
}

long long LengthRecordedStream(void)
{
  if (g_client == NULL)
    return -1;

  return g_client->LengthRecordedStream();
}


/*******************************************/
/** Unused API Functions                  **/

DemuxPacket* DemuxRead() { return NULL; }
void DemuxAbort() {}
void DemuxReset() {}
void DemuxFlush() {}
const char * GetLiveStreamURL(const PVR_CHANNEL &) { return ""; }

} //end extern "C"
