/*
 *      Copyright (C) 2005-2011 Team XBMC
 *      http://www.xbmc.org
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "client.h"
#include "xbmc_pvr_dll.h"
#include "pvrclient-mediaportal.h"
#include "utils.h"

using namespace std;
using namespace ADDON;

/* User adjustable settings are saved here.
 * Default values are defined inside client.h
 * and exported to the other source files.
 */
std::string      g_szHostname           = DEFAULT_HOST;                  ///< The Host name or IP of the MediaPortal TV Server
int              g_iPort                = DEFAULT_PORT;                  ///< The TVServerXBMC listening port (default: 9596)
int              g_iConnectTimeout      = DEFAULT_TIMEOUT;               ///< The Socket connection timeout
int              g_iSleepOnRTSPurl      = DEFAULT_SLEEP_RTSP_URL;        ///< An optional delay between tuning a channel and opening the corresponding RTSP stream in XBMC (default: 0)
bool             g_bOnlyFTA             = DEFAULT_FTA_ONLY;              ///< Send only Free-To-Air Channels inside Channel list to XBMC
bool             g_bRadioEnabled        = DEFAULT_RADIO;                 ///< Send also Radio channels list to XBMC
bool             g_bHandleMessages      = DEFAULT_HANDLE_MSG;            ///< Send VDR's OSD status messages to XBMC OSD
bool             g_bResolveRTSPHostname = DEFAULT_RESOLVE_RTSP_HOSTNAME; ///< Resolve the server hostname in the rtsp URLs to an IP at the TV Server side (default: false)
bool             g_bReadGenre           = DEFAULT_READ_GENRE;            ///< Read the genre strings from MediaPortal and translate them into XBMC DVB genre id's (only English)
CStdString       g_szTVGroup            = DEFAULT_TVGROUP;               ///< Import only TV channels from this TV Server TV group
CStdString       g_szRadioGroup         = DEFAULT_RADIOGROUP;            ///< Import only radio channels from this TV Server radio group
std::string      g_szSMBusername        = DEFAULT_SMBUSERNAME;           ///< Windows user account used to access share
std::string      g_szSMBpassword        = DEFAULT_SMBPASSWORD;           ///< Windows user password used to access share
                                                                         ///< Leave empty to use current user when running on Windows
eStreamingMethod g_eStreamingMethod     = TSReader;
bool             g_bFastChannelSwitch   = true;                          ///< Don't stop an existing timeshift on a channel switch
bool             g_bUseRTSP             = false;                         ///< Use RTSP streaming when using the tsreader

/* Client member variables */
ADDON_STATUS           m_CurStatus    = ADDON_STATUS_UNKNOWN;
cPVRClientMediaPortal *g_client       = NULL;
std::string            g_szUserPath   = "";
std::string            g_szClientPath = "";
CHelper_libXBMC_addon *XBMC           = NULL;
CHelper_libXBMC_pvr   *PVR            = NULL;

extern "C" {

void ADDON_ReadSettings(void);

/***********************************************************
 * Standard AddOn related public library functions
 ***********************************************************/

//-- Create -------------------------------------------------------------------
// Called after loading of the dll, all steps to become Client functional
// must be performed here.
//-----------------------------------------------------------------------------
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

  XBMC->Log(LOG_INFO, "Creating MediaPortal PVR-Client");

  m_CurStatus    = ADDON_STATUS_UNKNOWN;
  g_szUserPath   = pvrprops->strUserPath;
  g_szClientPath = pvrprops->strClientPath;

  ADDON_ReadSettings();

  /* Create connection to MediaPortal XBMC TV client */
  g_client       = new cPVRClientMediaPortal();

  m_CurStatus = g_client->Connect();
  if (m_CurStatus != ADDON_STATUS_OK)
  {
    SAFE_DELETE(g_client);
    SAFE_DELETE(PVR);
    SAFE_DELETE(XBMC);
  }

  return m_CurStatus;
}

//-- Destroy ------------------------------------------------------------------
// Used during destruction of the client, all steps to do clean and safe Create
// again must be done.
//-----------------------------------------------------------------------------
void ADDON_Destroy()
{
  SAFE_DELETE(g_client);
  SAFE_DELETE(PVR);
  SAFE_DELETE(XBMC);

  m_CurStatus = ADDON_STATUS_UNKNOWN;
}

//-- GetStatus ----------------------------------------------------------------
// Report the current Add-On Status to XBMC
//-----------------------------------------------------------------------------
ADDON_STATUS ADDON_GetStatus()
{
  /* check whether we're still connected */
  if (m_CurStatus == ADDON_STATUS_OK && g_client && !g_client->IsUp())
    m_CurStatus = ADDON_STATUS_LOST_CONNECTION;

  return m_CurStatus;
}

//-- HasSettings --------------------------------------------------------------
// Report "true", yes this AddOn have settings
//-----------------------------------------------------------------------------
bool ADDON_HasSettings()
{
  return true;
}

unsigned int ADDON_GetSettings(ADDON_StructSetting ***sSet)
{
  return 0;
}

void ADDON_ReadSettings(void)
{
  /* Read setting "host" from settings.xml */
  char buffer[1024];

  if (!XBMC)
    return;

  /* Connection settings */
  /***********************/
  if (XBMC->GetSetting("host", &buffer))
  {
    g_szHostname = buffer;
    uri::decode(g_szHostname);
  }
  else
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'host' setting, falling back to '127.0.0.1' as default");
    g_szHostname = DEFAULT_HOST;
  }

  /* Read setting "port" from settings.xml */
  if (!XBMC->GetSetting("port", &g_iPort))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'port' setting, falling back to '9596' as default");
    g_iPort = DEFAULT_PORT;
  }

  /* Read setting "timeout" from settings.xml */
  if (!XBMC->GetSetting("timeout", &g_iConnectTimeout))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'timeout' setting, falling back to %i seconds as default", DEFAULT_TIMEOUT);
    g_iConnectTimeout = DEFAULT_TIMEOUT;
  }

  /* MediaPortal settings */
  /***********************/

  /* Read setting "ftaonly" from settings.xml */
  if (!XBMC->GetSetting("ftaonly", &g_bOnlyFTA))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'ftaonly' setting, falling back to 'false' as default");
    g_bOnlyFTA = DEFAULT_FTA_ONLY;
  }

  /* Read setting "useradio" from settings.xml */
  if (!XBMC->GetSetting("useradio", &g_bRadioEnabled))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'useradio' setting, falling back to 'true' as default");
    g_bRadioEnabled = DEFAULT_RADIO;
  }

  if (!XBMC->GetSetting("tvgroup", &buffer))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'tvgroup' setting, falling back to '' as default");
  } else {
    g_szTVGroup = buffer;
    g_szTVGroup.Replace(";","|");
  }

  if (!XBMC->GetSetting("radiogroup", &buffer))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'radiogroup' setting, falling back to '' as default");
  } else {
    g_szRadioGroup = buffer;
    g_szRadioGroup.Replace(";","|");
  }

  if (!XBMC->GetSetting("streamingmethod", &g_eStreamingMethod))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'streamingmethod' setting, falling back to 'tsreader' as default");
    g_eStreamingMethod = TSReader;
  }

  /* Read setting "resolvertsphostname" from settings.xml */
  if (!XBMC->GetSetting("resolvertsphostname", &g_bResolveRTSPHostname))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'resolvertsphostname' setting, falling back to 'true' as default");
    g_bResolveRTSPHostname = DEFAULT_RESOLVE_RTSP_HOSTNAME;
  }

  /* Read setting "readgenre" from settings.xml */
  if (!XBMC->GetSetting("readgenre", &g_bReadGenre))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'readgenre' setting, falling back to 'true' as default");
    g_bReadGenre = DEFAULT_READ_GENRE;
  }

  /* Read setting "sleeponrtspurl" from settings.xml */
  if (!XBMC->GetSetting("sleeponrtspurl", &g_iSleepOnRTSPurl))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'sleeponrtspurl' setting, falling back to %i seconds as default", DEFAULT_SLEEP_RTSP_URL);
    g_iSleepOnRTSPurl = DEFAULT_SLEEP_RTSP_URL;
  }


  /* TSReader settings */
  /*********************/
  /* Read setting "fastchannelswitch" from settings.xml */
  if (!XBMC->GetSetting("fastchannelswitch", &g_bFastChannelSwitch))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'fastchannelswitch' setting, falling back to 'false' as default");
    g_bFastChannelSwitch = false;
  }

  /* read setting "user" from settings.xml */
  if (!XBMC->GetSetting("smbusername", &buffer))
  {
    XBMC->Log(LOG_ERROR, "Couldn't get 'smbusername' setting, falling back to '%s' as default", DEFAULT_SMBUSERNAME);
    g_szSMBusername = DEFAULT_SMBUSERNAME;
  }
  else
    g_szSMBusername = buffer;

  /* read setting "pass" from settings.xml */
  if (!XBMC->GetSetting("smbpassword", &buffer))
  {
    XBMC->Log(LOG_ERROR, "Couldn't get 'smbpassword' setting, falling back to '%s' as default", DEFAULT_SMBPASSWORD);
    g_szSMBpassword = DEFAULT_SMBPASSWORD;
  }
  else
    g_szSMBpassword = buffer;

  /* Read setting "usertsp" from settings.xml */
  if (!XBMC->GetSetting("usertsp", &g_bUseRTSP))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'usertsp' setting, falling back to 'false' as default");
    g_bUseRTSP = false;
  }

  /* Log the current settings for debugging purposes */
  XBMC->Log(LOG_DEBUG, "settings: streamingmethod: %s, usertsp=%i", (( g_eStreamingMethod == TSReader) ? "TSReader" : "ffmpeg"), (int) g_bUseRTSP);
  XBMC->Log(LOG_DEBUG, "settings: host='%s', port=%i, timeout=%i", g_szHostname.c_str(), g_iPort, g_iConnectTimeout);
  XBMC->Log(LOG_DEBUG, "settings: ftaonly=%i, useradio=%i, tvgroup='%s', radiogroup='%s'", (int) g_bOnlyFTA, (int) g_bRadioEnabled, g_szTVGroup.c_str(), g_szRadioGroup.c_str());
  XBMC->Log(LOG_DEBUG, "settings: readgenre=%i, sleeponrtspurl=%i", (int) g_bReadGenre, g_iSleepOnRTSPurl);
  XBMC->Log(LOG_DEBUG, "settings: resolvertsphostname=%i", (int) g_bResolveRTSPHostname);
  XBMC->Log(LOG_DEBUG, "settings: fastchannelswitch=%i", (int) g_bFastChannelSwitch);
  XBMC->Log(LOG_DEBUG, "settings: smb user='%s', pass=%s", g_szSMBusername.c_str(), (g_szSMBpassword.length() > 0 ? "<set>" : "<empty>"));
}

//-- SetSetting ---------------------------------------------------------------
// Called everytime a setting is changed by the user and to inform AddOn about
// new setting and to do required stuff to apply it.
//-----------------------------------------------------------------------------
ADDON_STATUS ADDON_SetSetting(const char *settingName, const void *settingValue)
{
  string str = settingName;

  // SetSetting can occur when the addon is enabled, but TV support still
  // disabled. In that case the addon is not loaded, so we should not try
  // to change its settings.
  if (!XBMC)
    return ADDON_STATUS_OK;

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
  else if (str == "ftaonly")
  {
    XBMC->Log(LOG_INFO, "Changed setting 'ftaonly' from %u to %u", g_bOnlyFTA, *(bool*) settingValue);
    g_bOnlyFTA = *(bool*) settingValue;
  }
  else if (str == "useradio")
  {
    XBMC->Log(LOG_INFO, "Changed setting 'useradio' from %u to %u", g_bRadioEnabled, *(bool*) settingValue);
    g_bRadioEnabled = *(bool*) settingValue;
  }
  else if (str == "timeout")
  {
    XBMC->Log(LOG_INFO, "Changed setting 'timeout' from %u to %u", g_iConnectTimeout, *(int*) settingValue);
    g_iConnectTimeout = *(int*) settingValue;
  }
  else if (str == "tvgroup")
  {
    XBMC->Log(LOG_INFO, "Changed setting 'tvgroup' from '%s' to '%s'", g_szTVGroup.c_str(), (const char*) settingValue);
    g_szTVGroup = (const char*) settingValue;
  }
  else if (str == "radiogroup")
  {
    XBMC->Log(LOG_INFO, "Changed setting 'radiogroup' from '%s' to '%s'", g_szRadioGroup.c_str(), (const char*) settingValue);
    g_szRadioGroup = (const char*) settingValue;
  }
  else if (str == "resolvertsphostname")
  {
    XBMC->Log(LOG_INFO, "Changed setting 'resolvertsphostname' from %u to %u", g_bResolveRTSPHostname, *(bool*) settingValue);
    g_bResolveRTSPHostname = *(bool*) settingValue;
  }
  else if (str == "readgenre")
  {
    XBMC->Log(LOG_INFO, "Changed setting 'readgenre' from %u to %u", g_bReadGenre, *(bool*) settingValue);
    g_bReadGenre = *(bool*) settingValue;
  }
  else if (str == "sleeponrtspurl")
  {
    XBMC->Log(LOG_INFO, "Changed setting 'sleeponrtspurl' from %u to %u", g_iSleepOnRTSPurl, *(int*) settingValue);
    g_iSleepOnRTSPurl = *(int*) settingValue;
  }
  else if (str == "smbusername")
  {
    XBMC->Log(LOG_INFO, "Changed setting 'smbusername' from '%s' to '%s'", g_szSMBusername.c_str(), (const char*) settingValue);
    g_szSMBusername = (const char*) settingValue;
  }
  else if (str == "smbpassword")
  {
    XBMC->Log(LOG_INFO, "Changed setting 'smbpassword' from '%s' to '%s'", g_szSMBpassword.c_str(), (const char*) settingValue);
   g_szSMBpassword = (const char*) settingValue;
  }
  else if (str == "fastchannelswitch")
  {
    XBMC->Log(LOG_INFO, "Changed setting 'fastchannelswitch' from %u to %u", g_bFastChannelSwitch, *(bool*) settingValue);
    g_bFastChannelSwitch = *(bool*) settingValue;
  }
  else if (str == "streamingmethod")
  {
    if (g_eStreamingMethod != *(eStreamingMethod*) settingValue)
    {
      XBMC->Log(LOG_INFO, "Changed setting 'streamingmethod' from %u to %u", g_eStreamingMethod, *(int*) settingValue);
      g_eStreamingMethod = *(eStreamingMethod*) settingValue;
      /* Switching between ffmpeg and tsreader mode requires a restart due to different channel streams */
      return ADDON_STATUS_NEED_RESTART;
    }
  }
  else if (str == "usertsp")
  {
    XBMC->Log(LOG_INFO, "Changed setting 'usertsp' from %u to %u", g_bUseRTSP, *(bool*) settingValue);
    g_bUseRTSP = *(bool*) settingValue;
  }

  return ADDON_STATUS_OK;
}

void ADDON_Stop()
{
  ADDON_Destroy();
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

//-- GetAddonCapabilities -----------------------------------------------------
// Tell XBMC our requirements
//-----------------------------------------------------------------------------
PVR_ERROR GetAddonCapabilities(PVR_ADDON_CAPABILITIES *pCapabilities)
{
  XBMC->Log(LOG_DEBUG, "->GetProperties()");

  pCapabilities->bSupportsEPG                = true;
  pCapabilities->bSupportsRecordings         = true;
  pCapabilities->bSupportsTimers             = true;
  pCapabilities->bSupportsTV                 = true;
  pCapabilities->bSupportsRadio              = g_bRadioEnabled;
  pCapabilities->bSupportsChannelGroups      = true;
  pCapabilities->bHandlesInputStream         = true;
  pCapabilities->bHandlesDemuxing            = false;
  pCapabilities->bSupportsChannelScan        = false;
  pCapabilities->bSupportsRecordingPlayCount = (g_iTVServerXBMCBuild < 117) ? false : true;
  pCapabilities->bSupportsLastPlayedPosition = (g_iTVServerXBMCBuild < 121) ? false : true;
  pCapabilities->bSupportsRecordingFolders   = false; // Don't show the timer directory field. This does not influence the displaying directories in the recordings list.

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR GetStreamProperties(PVR_STREAM_PROPERTIES *pProperties)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}

//-- GetBackendName -----------------------------------------------------------
// Return the Name of the Backend
//-----------------------------------------------------------------------------
const char * GetBackendName(void)
{
  if (g_client)
    return g_client->GetBackendName();
  else
    return "";
}

//-- GetBackendVersion --------------------------------------------------------
// Return the Version of the Backend as String
//-----------------------------------------------------------------------------
const char * GetBackendVersion(void)
{
  if (g_client)
    return g_client->GetBackendVersion();
  else
    return "";
}

//-- GetConnectionString ------------------------------------------------------
// Return a String with connection info, if available
//-----------------------------------------------------------------------------
const char * GetConnectionString(void)
{
  if (g_client)
    return g_client->GetConnectionString();
  else
    return "addon error!";
}

//-- GetDriveSpace ------------------------------------------------------------
// Return the Total and Free Drive space on the PVR Backend
//-----------------------------------------------------------------------------
PVR_ERROR GetDriveSpace(long long *iTotal, long long *iUsed)
{
  if (!g_client)
    return PVR_ERROR_SERVER_ERROR;
  else
    return g_client->GetDriveSpace(iTotal, iUsed);
}

PVR_ERROR GetBackendTime(time_t *localTime, int *gmtOffset)
{
  if (!g_client)
    return PVR_ERROR_SERVER_ERROR;
  else
    return g_client->GetBackendTime(localTime, gmtOffset);
}

PVR_ERROR DialogChannelScan()
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}

PVR_ERROR CallMenuHook(const PVR_MENUHOOK &menuhook, const PVR_MENUHOOK_DATA &item)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}


/*******************************************/
/** PVR EPG Functions                     **/

PVR_ERROR GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd)
{
  if (!g_client)
    return PVR_ERROR_SERVER_ERROR;
  else
    return g_client->GetEpg(handle, channel, iStart, iEnd);
}


/*******************************************/
/** PVR Channel Functions                 **/

int GetChannelsAmount()
{
  if (!g_client)
    return 0;
  else
    return g_client->GetNumChannels();
}

PVR_ERROR GetChannels(ADDON_HANDLE handle, bool bRadio)
{
  if (!g_client)
    return PVR_ERROR_SERVER_ERROR;
  else
    return g_client->GetChannels(handle, bRadio);
}

PVR_ERROR DeleteChannel(const PVR_CHANNEL &channel)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}

PVR_ERROR RenameChannel(const PVR_CHANNEL &channel)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}

PVR_ERROR DialogChannelSettings(const PVR_CHANNEL &channelinfo)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}

PVR_ERROR DialogAddChannel(const PVR_CHANNEL &channelinfo)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}


/*******************************************/
/** PVR Channel group Functions           **/

int GetChannelGroupsAmount(void)
{
  if (!g_client)
    return 0;
  else
    return g_client->GetChannelGroupsAmount();
}

PVR_ERROR GetChannelGroups(ADDON_HANDLE handle, bool bRadio)
{
  if (!g_client)
    return PVR_ERROR_SERVER_ERROR;
  else
    return g_client->GetChannelGroups(handle, bRadio);
}

PVR_ERROR GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group)
{
  if (!g_client)
    return PVR_ERROR_SERVER_ERROR;
  else
    return g_client->GetChannelGroupMembers(handle, group);
}


/*******************************************/
/** PVR Recording Functions               **/

int GetRecordingsAmount(void)
{
  if (!g_client)
    return 0;
  else
    return g_client->GetNumRecordings();
}

PVR_ERROR GetRecordings(ADDON_HANDLE handle)
{
  if (!g_client)
    return PVR_ERROR_SERVER_ERROR;
  else
    return g_client->GetRecordings(handle);
}

PVR_ERROR DeleteRecording(const PVR_RECORDING &recording)
{
  if (!g_client)
    return PVR_ERROR_SERVER_ERROR;
  else
    return g_client->DeleteRecording(recording);
}

PVR_ERROR RenameRecording(const PVR_RECORDING &recording)
{
  if (!g_client)
    return PVR_ERROR_SERVER_ERROR;
  else
    return g_client->RenameRecording(recording);
}

PVR_ERROR SetRecordingPlayCount(const PVR_RECORDING &recording, int count)
{
  if (!g_client)
    return PVR_ERROR_SERVER_ERROR;
  else
    return g_client->SetRecordingPlayCount(recording, count);
}

PVR_ERROR SetRecordingLastPlayedPosition(const PVR_RECORDING &recording, int lastplayedposition)
{
  if (!g_client)
    return PVR_ERROR_SERVER_ERROR;
  else
    return g_client->SetRecordingLastPlayedPosition(recording, lastplayedposition);
}

int GetRecordingLastPlayedPosition(const PVR_RECORDING &recording)
{
  if (!g_client)
    return PVR_ERROR_SERVER_ERROR;
  else
    return g_client->GetRecordingLastPlayedPosition(recording);
}

/*******************************************/
/** PVR Timer Functions                   **/

int GetTimersAmount(void)
{
  if (!g_client)
    return 0;
  else
    return g_client->GetNumTimers();
}

PVR_ERROR GetTimers(ADDON_HANDLE handle)
{
  if (!g_client)
    return PVR_ERROR_SERVER_ERROR;
  else
    return g_client->GetTimers(handle);
}

PVR_ERROR AddTimer(const PVR_TIMER &timer)
{
  if (!g_client)
    return PVR_ERROR_SERVER_ERROR;
  else
    return g_client->AddTimer(timer);
}

PVR_ERROR DeleteTimer(const PVR_TIMER &timer, bool bForceDelete)
{
  if (!g_client)
    return PVR_ERROR_SERVER_ERROR;
  else
    return g_client->DeleteTimer(timer, bForceDelete);
}

PVR_ERROR UpdateTimer(const PVR_TIMER &timer)
{
  if (!g_client)
    return PVR_ERROR_SERVER_ERROR;
  else
    return g_client->UpdateTimer(timer);
}


/*******************************************/
/** PVR Live Stream Functions             **/

bool OpenLiveStream(const PVR_CHANNEL &channelinfo)
{
  if (!g_client)
    return false;
  else
    return g_client->OpenLiveStream(channelinfo);
}

void CloseLiveStream()
{
  if (g_client)
    g_client->CloseLiveStream();
}

int ReadLiveStream(unsigned char *pBuffer, unsigned int iBufferSize)
{
  if (!g_client)
    return 0;
  else
    return g_client->ReadLiveStream(pBuffer, iBufferSize);
}

long long SeekLiveStream(long long iPosition, int iWhence)
{
  if (!g_client)
    return -1;
  else
    return g_client->SeekLiveStream(iPosition, iWhence);
}

long long PositionLiveStream(void)
{
  if (!g_client)
    return -1;
  else
    return g_client->PositionLiveStream();
}

long long LengthLiveStream(void)
{
  if (!g_client)
    return -1;
  else
    return g_client->LengthLiveStream();
}

int GetCurrentClientChannel()
{
  if (!g_client)
    return 0;
  else
    return g_client->GetCurrentClientChannel();
}

bool SwitchChannel(const PVR_CHANNEL &channelinfo)
{
  if (!g_client)
    return false;
  else
    return g_client->SwitchChannel(channelinfo);
}

PVR_ERROR SignalStatus(PVR_SIGNAL_STATUS &signalStatus)
{
  if (!g_client)
    return PVR_ERROR_SERVER_ERROR;
  else
    return g_client->SignalStatus(signalStatus);
}

/*******************************************/
/** PVR Recording Stream Functions        **/

bool OpenRecordedStream(const PVR_RECORDING &recording)
{
  if (!g_client)
    return false;
  else
    return g_client->OpenRecordedStream(recording);
}

void CloseRecordedStream(void)
{
  if (g_client)
    g_client->CloseRecordedStream();
}

int ReadRecordedStream(unsigned char *pBuffer, unsigned int iBufferSize)
{
  if (!g_client)
    return 0;
  else
    return g_client->ReadRecordedStream(pBuffer, iBufferSize);
}

long long SeekRecordedStream(long long iPosition, int iWhence)
{
  if (!g_client)
    return -1;
  else
    return g_client->SeekRecordedStream(iPosition, iWhence);
}

long long PositionRecordedStream(void)
{
  if (!g_client)
    return -1;
  else
    return g_client->PositionRecordedStream();
}

long long LengthRecordedStream(void)
{
  if (!g_client)
    return -1;
  else
    return g_client->LengthRecordedStream();
}

const char * GetLiveStreamURL(const PVR_CHANNEL &channel)
{
  if (!g_client)
    return "";
  else
    return g_client->GetLiveStreamURL(channel);
}

bool CanPauseStream(void)
{
  if (g_client)
    return g_client->CanPauseAndSeek();

  return false;
}

void PauseStream(bool bPaused)
{
  if (g_client)
    g_client->PauseStream(bPaused);
}

bool CanSeekStream(void)
{
  if (g_client)
    return g_client->CanPauseAndSeek();

  return false;
}

/** UNUSED API FUNCTIONS */
PVR_ERROR MoveChannel(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
DemuxPacket* DemuxRead(void) { return NULL; }
void DemuxAbort(void) {}
void DemuxReset(void) {}
void DemuxFlush(void) {}

PVR_ERROR GetRecordingEdl(const PVR_RECORDING&, PVR_EDL_ENTRY[], int*) { return PVR_ERROR_NOT_IMPLEMENTED; };
unsigned int GetChannelSwitchDelay(void) { return 0; }
bool SeekTime(int,bool,double*) { return false; }
void SetSpeed(int) {};
time_t GetPlayingTime() { return 0; }
time_t GetBufferTimeStart() { return 0; }
time_t GetBufferTimeEnd() { return 0; }
} //end extern "C"
