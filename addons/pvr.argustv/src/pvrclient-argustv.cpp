/*
 *      Copyright (C) 2010 Marcel Groothuis
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
//#include "timers.h"
#include "channel.h"
#include "activerecording.h"
#include "upcomingrecording.h"
#include "recordinggroup.h"
#include "recordingsummary.h"
#include "recording.h"
#include "epg.h"
#include "utils.h"
#include "pvrclient-argustv.h"
#include "argustvrpc.h"
#include "platform/util/timeutils.h"
#include "platform/util/StdString.h"

#include "lib/tsreader/TSReader.h"

using namespace std;
using namespace ADDON;

#if !defined(TARGET_WINDOWS)
using namespace PLATFORM;
#endif

#define SIGNALQUALITY_INTERVAL 10

/************************************************************/
/** Class interface */

cPVRClientArgusTV::cPVRClientArgusTV()
{
  m_bConnected             = false;
  //m_bStop                  = true;
  m_bTimeShiftStarted      = false;
  m_BackendUTCoffset       = 0;
  m_BackendTime            = 0;
  m_tsreader               = NULL;
  m_channel_id_offset      = 0;
  m_epg_id_offset          = 0;
  m_iCurrentChannel        = -1;
  // due to lack of static constructors, we initialize manually
  ArgusTV::Initialize();
#if defined(ATV_DUMPTS)
  strncpy(ofn, "/tmp/atv.XXXXXX", sizeof(ofn));
  ofd = -1;
#endif
}

cPVRClientArgusTV::~cPVRClientArgusTV()
{
  XBMC->Log(LOG_DEBUG, "->~cPVRClientArgusTV()");
  // Check if we are still reading a TV/Radio stream and close it here
  if (m_bTimeShiftStarted)
  {
    CloseLiveStream();
  }
}


bool cPVRClientArgusTV::Connect()
{
  string result;
  char buffer[256];

  snprintf(buffer, 256, "http://%s:%i/", g_szHostname.c_str(), g_iPort);
  g_szBaseURL = buffer;

  XBMC->Log(LOG_INFO, "Connect() - Connecting to %s", g_szBaseURL.c_str());

  int backendversion = ATV_REST_MAXIMUM_API_VERSION;
  int rc = -2;
  int attemps = 0;

  while (rc != 0)
  {
    attemps++;
    rc = ArgusTV::Ping(backendversion);
    if (rc == 1)
    {
      backendversion = ATV_REST_MINIMUM_API_VERSION;
      rc = ArgusTV::Ping(backendversion);
    }
    m_iBackendVersion = backendversion;

    switch (rc)
    {
      case 0:
        XBMC->Log(LOG_INFO, "Ping Ok. The client and server are compatible, API version %d.\n", m_iBackendVersion);
        break;
      case -1:
        XBMC->Log(LOG_NOTICE, "Ping Ok. The ARGUS TV server is too new for this version of the add-on.\n");
        XBMC->QueueNotification(QUEUE_ERROR, "The ARGUS TV server is too new for this version of the add-on");
        return false;
      case 1:
        XBMC->Log(LOG_NOTICE, "Ping Ok. The ARGUS TV server is too old for this version of the add-on.\n");
        XBMC->QueueNotification(QUEUE_ERROR, "The ARGUS TV server is too old for this version of the add-on");
        return false;
      default:
         XBMC->Log(LOG_ERROR, "Ping failed... No connection to Argus TV.\n");
         usleep(1000000);
         if (attemps > 3)
         {
           XBMC->QueueNotification(QUEUE_ERROR, "No connection to Argus TV server");
           return false;
         }
    }
  }

  // Check the accessibility status of all the shares used by ArgusTV tuners
  // TODO: this is temporarily disabled until the caching of smb:// directories is resolved
//  if (ShareErrorsFound())
//  {
//    XBMC->QueueNotification(QUEUE_ERROR, "Share errors: see xbmc.log");
//  }

  m_bConnected = true;
  return true;
}

void cPVRClientArgusTV::Disconnect()
{
  string result;

  XBMC->Log(LOG_INFO, "Disconnect");

  if (m_bTimeShiftStarted)
  {
    //TODO: tell ArgusTV that it should stop streaming
  }

  m_bConnected = false;
}

bool cPVRClientArgusTV::ShareErrorsFound(void)
{
  bool bShareErrors = false;
  Json::Value activeplugins;
  int rc = ArgusTV::GetPluginServices(false, activeplugins);
  if (rc < 0)
  {
    XBMC->Log(LOG_ERROR, "Unable to get the ARGUS TV plugin services to check share accessiblity.");
    return false;
  }
 
  // parse plugins list
  int size = activeplugins.size();
  for ( int index =0; index < size; ++index )
  {
    std::string tunerName = activeplugins[index]["Name"].asString();
    XBMC->Log(LOG_DEBUG, "Checking tuner \"%s\" for accessibility.", tunerName.c_str());
    Json::Value accesibleshares;
    rc = ArgusTV::AreRecordingSharesAccessible(activeplugins[index], accesibleshares);
    if (rc < 0)
    {
      XBMC->Log(LOG_ERROR, "Unable to get the share status for tuner \"%s\".", tunerName.c_str());
      continue;
    }
    int numberofshares = accesibleshares.size();
    for (int j = 0; j < numberofshares; j++)
    {
      Json::Value accesibleshare = accesibleshares[j];
      tunerName = accesibleshare["RecorderTunerName"].asString();
      std::string sharename = accesibleshare["Share"].asString();
      bool isAccessibleByATV = accesibleshare["ShareAccessible"].asBool();
      bool isAccessibleByAddon = false;
      std::string accessMsg = "";
#if defined(TARGET_WINDOWS)
      // Try to open the directory
      CStdStringW strWFile = UTF8Util::ConvertUTF8ToUTF16(sharename.c_str());
      HANDLE hFile = ::CreateFileW(strWFile,      // The filename
        (DWORD) GENERIC_READ,             // File access
        (DWORD) FILE_SHARE_READ,          // Share access
        NULL,                             // Security
        (DWORD) OPEN_EXISTING,            // Open flags
        (DWORD) FILE_FLAG_BACKUP_SEMANTICS, // More flags
        NULL);                            // Template
      if (hFile != INVALID_HANDLE_VALUE)
      {
        (void) CloseHandle(hFile);
        isAccessibleByAddon = true;
      }
      else
      {
        LPVOID lpMsgBuf;
        DWORD dwErr = GetLastError();
        FormatMessage(
          FORMAT_MESSAGE_ALLOCATE_BUFFER | 
          FORMAT_MESSAGE_FROM_SYSTEM |
          FORMAT_MESSAGE_IGNORE_INSERTS,
          NULL,
          dwErr,
          MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
          (LPTSTR) &lpMsgBuf,
          0, NULL );
        accessMsg = (char*) lpMsgBuf;
        LocalFree(lpMsgBuf);
      }
#elif defined(TARGET_LINUX) || defined(TARGET_DARWIN) || defined(TARGET_FREEBSD)
      std::string CIFSname = sharename;
      std::string SMBPrefix = "smb://";
      if (g_szUser.length() > 0)
      {
        SMBPrefix += g_szUser;
        if (g_szPass.length() > 0)
        {
          SMBPrefix += ":" + g_szPass;
        }
      }
      else
      {
        SMBPrefix += "Guest";
      }
      SMBPrefix += "@";
      size_t found;
      while ((found = CIFSname.find("\\")) != std::string::npos)
      {
        CIFSname.replace(found, 1, "/");
      }
      CIFSname.erase(0,2);
      CIFSname.insert(0, SMBPrefix);
      isAccessibleByAddon = XBMC->CanOpenDirectory(CIFSname.c_str());
#else
#error implement for your OS!
#endif
      // write analysis results to the log
      if (isAccessibleByATV)
      {
        XBMC->Log(LOG_DEBUG, "  Share \"%s\" is accessible to the ARGUS TV server.", sharename.c_str());
      }
      else
      {
        bShareErrors = true;
        XBMC->Log(LOG_ERROR, "  Share \"%s\" is NOT accessible to the ARGUS TV server.", sharename.c_str());
      }
      if (isAccessibleByAddon)
      {
        XBMC->Log(LOG_DEBUG, "  Share \"%s\" is readable from this client add-on.", sharename.c_str());
      }
      else
      {
        bShareErrors = true;
        XBMC->Log(LOG_ERROR, "  Share \"%s\" is NOT readable from this client add-on (\"%s\").", sharename.c_str(), accessMsg.c_str());
      }
    }
  }
  return bShareErrors;
}

/************************************************************/
/** General handling */

// Used among others for the server name string in the "Recordings" view
const char* cPVRClientArgusTV::GetBackendName(void)
{
  XBMC->Log(LOG_DEBUG, "->GetBackendName()");

  if(m_BackendName.length() == 0)
  {
    m_BackendName = "ARGUS TV (";
    m_BackendName += g_szHostname.c_str();
    m_BackendName += ")";
  }

  return m_BackendName.c_str();
}

const char* cPVRClientArgusTV::GetBackendVersion(void)
{
  XBMC->Log(LOG_DEBUG, "->GetBackendVersion");
  m_sBackendVersion = "unknown";
  Json::Value response;
  int retval;

  retval = ArgusTV::GetDisplayVersion(response);

  if (retval != E_FAILED)
  {
    m_sBackendVersion = response.asString();
    XBMC->Log(LOG_DEBUG, "GetDisplayVersion: \"%s\".", m_sBackendVersion.c_str());
  }

  return m_sBackendVersion.c_str();
}

const char* cPVRClientArgusTV::GetConnectionString(void)
{
  XBMC->Log(LOG_DEBUG, "->GetConnectionString()");

  return g_szBaseURL.c_str();
}

PVR_ERROR cPVRClientArgusTV::GetDriveSpace(long long *iTotal, long long *iUsed)
{
  XBMC->Log(LOG_DEBUG, "->GetDriveSpace");
  *iTotal = *iUsed = 0;
  Json::Value response;
  int retval;

  retval = ArgusTV::GetRecordingDisksInfo(response);

  if (retval != E_FAILED)
  {
    double _totalSize = response["TotalSizeBytes"].asDouble()/1024;
    double _freeSize = response["FreeSpaceBytes"].asDouble()/1024;
    *iTotal = (long long) _totalSize;
    *iUsed = (long long) (_totalSize - _freeSize);
    XBMC->Log(LOG_DEBUG, "GetDriveSpace, %lld used kiloBytes of %lld total kiloBytes.", *iUsed, *iTotal);
  }

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR cPVRClientArgusTV::GetBackendTime(time_t *localTime, int *gmtOffset)
{
  NOTUSED(localTime); NOTUSED(gmtOffset);
  return PVR_ERROR_SERVER_ERROR;
}

/************************************************************/
/** EPG handling */

PVR_ERROR cPVRClientArgusTV::GetEpg(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd)
{
  XBMC->Log(LOG_DEBUG, "->RequestEPGForChannel(%i)", channel.iUniqueId);

  cChannel* atvchannel = FetchChannel(channel.iUniqueId);

  struct tm* convert = localtime(&iStart);
  struct tm tm_start = *convert;
  convert = localtime(&iEnd);
  struct tm tm_end = *convert;

  if(atvchannel)
  {
    Json::Value response;
    int retval;

    retval = ArgusTV::GetEPGData(atvchannel->GuideChannelID(), tm_start, tm_end, response);

    if (retval != E_FAILED)
    {
      XBMC->Log(LOG_DEBUG, "GetEPGData returned %i, response.type == %i, response.size == %i.", retval, response.type(), response.size());
      if( response.type() == Json::arrayValue)
      {
        int size = response.size();
        EPG_TAG broadcast;
        cEpg epg;

        memset(&broadcast, 0, sizeof(EPG_TAG));

        // parse channel list
        for ( int index =0; index < size; ++index )
        {
          if (epg.Parse(response[index]))
          {
            m_epg_id_offset++;
            broadcast.iUniqueBroadcastId  = m_epg_id_offset;
            broadcast.strTitle            = epg.Title();
            broadcast.iChannelNumber      = channel.iUniqueId;
            broadcast.startTime           = epg.StartTime();
            broadcast.endTime             = epg.EndTime();
            broadcast.strPlotOutline      = epg.Subtitle();
            broadcast.strPlot             = epg.Description();
            broadcast.strIconPath         = "";
            broadcast.iGenreType          = EPG_GENRE_USE_STRING;
            broadcast.iGenreSubType       = 0;
            broadcast.strGenreDescription = epg.Genre();
            broadcast.firstAired          = 0;
            broadcast.iParentalRating     = 0;
            broadcast.iStarRating         = 0;
            broadcast.bNotify             = false;
            broadcast.iSeriesNumber       = 0;
            broadcast.iEpisodeNumber      = 0;
            broadcast.iEpisodePartNumber  = 0;
            broadcast.strEpisodeName      = "";

            PVR->TransferEpgEntry(handle, &broadcast);
          }
          epg.Reset();
        }
      }
    }
    else
    {
      XBMC->Log(LOG_ERROR, "GetEPGData failed for channel id:%i", channel.iUniqueId);
    }
  }
  else
  {
    XBMC->Log(LOG_ERROR, "Channel (%i) did not return a channel class.", channel.iUniqueId);
    XBMC->QueueNotification(QUEUE_ERROR, "GUID to XBMC Channel");
  }

  return PVR_ERROR_NO_ERROR;
}

bool cPVRClientArgusTV::FetchGuideProgramDetails(std::string Id, cGuideProgram& guideprogram)
{ 
  bool fRc = false;
  Json::Value guideprogramresponse;

  int retval = ArgusTV::GetProgramById(Id, guideprogramresponse);
  if (retval >= 0)
  {
    fRc = guideprogram.Parse(guideprogramresponse);
  }
  return fRc;
}

/************************************************************/
/** Channel handling */

int cPVRClientArgusTV::GetNumChannels()
{
  // Not directly possible in ARGUS TV
  Json::Value response;

  XBMC->Log(LOG_DEBUG, "GetNumChannels()");

  // pick up the channellist for TV
  int retval = ArgusTV::GetChannelList(ArgusTV::Television, response);
  if (retval < 0) 
  {
    return 0;
  }

  int numberofchannels = response.size();

  // When radio is enabled, add the number of radio channels
  if (g_bRadioEnabled)
  {
    retval = ArgusTV::GetChannelList(ArgusTV::Radio, response);
    if (retval >= 0)
    {
      numberofchannels += response.size();
    }
  }

  return numberofchannels;
}

PVR_ERROR cPVRClientArgusTV::GetChannels(ADDON_HANDLE handle, bool bRadio)
{
  Json::Value response;
  int retval = -1;

  if (bRadio && !g_bRadioEnabled) return PVR_ERROR_NO_ERROR;

  XBMC->Log(LOG_DEBUG, "%s(%s)", __FUNCTION__, bRadio ? "radio" : "television");
  if (!bRadio)
  {
    retval = ArgusTV::GetChannelList(ArgusTV::Television, response);
  }
  else
  {
    retval = ArgusTV::GetChannelList(ArgusTV::Radio, response);
  }

  if(retval >= 0)
  {           
    int size = response.size();

    // parse channel list
    for ( int index = 0; index < size; ++index )
    {

      cChannel channel;
      if( channel.Parse(response[index]) )
      {
        PVR_CHANNEL tag;
        memset(&tag, 0 , sizeof(tag));
        //Hack: assumes that the order of the channel list is fixed.
        //      We can't use the ARGUS TV channel id's. They are GUID strings (128 bit int).       
        //      But only if it isn't cached yet!
        if (FetchChannel(channel.Guid(), false) == NULL)
        {
          tag.iUniqueId =  m_channel_id_offset + 1;
          m_channel_id_offset++;
        }
        else
        {
          tag.iUniqueId = FetchChannel(channel.Guid())->ID();
        }
        strncpy(tag.strChannelName, channel.Name(), sizeof(tag.strChannelName));
        std::string logopath = ArgusTV::GetChannelLogo(channel.Guid()).c_str();
        strncpy(tag.strIconPath, logopath.c_str(), sizeof(tag.strIconPath));
        tag.iEncryptionSystem = (unsigned int) -1; //How to fetch this from ARGUS TV??
        tag.bIsRadio = (channel.Type() == ArgusTV::Radio ? true : false);
        tag.bIsHidden = false;
        //Use OpenLiveStream to read from the timeshift .ts file or an rtsp stream
        memset(tag.strStreamURL, 0, sizeof(tag.strStreamURL));
        strncpy(tag.strInputFormat, "video/x-mpegts", sizeof(tag.strInputFormat));
        tag.iChannelNumber = channel.LCN();

        if (!tag.bIsRadio)
        {
          XBMC->Log(LOG_DEBUG, "Found TV channel: %s, Unique id: %d, Backend channel: %d\n", channel.Name(), tag.iUniqueId, tag.iChannelNumber);
        }
        else
        {
          XBMC->Log(LOG_DEBUG, "Found Radio channel: %s, Unique id: %d, Backend channel: %d\n", channel.Name(), tag.iUniqueId, tag.iChannelNumber);
        }
        channel.SetID(tag.iUniqueId);
        if (FetchChannel(channel.Guid(), false) == NULL)
        {
          m_Channels.push_back(channel); //Local cache...
        }
        PVR->TransferChannelEntry(handle, &tag);
      }
    }

    return PVR_ERROR_NO_ERROR;
  }
  else
  {
    XBMC->Log(LOG_DEBUG, "RequestChannelList failed. Return value: %i\n", retval);
  }

  return PVR_ERROR_SERVER_ERROR;
}

/************************************************************/
/** Channel group handling **/

int cPVRClientArgusTV::GetChannelGroupsAmount(void)
{
  Json::Value response;
  int num = 0;
  if (ArgusTV::RequestTVChannelGroups(response) >= 0) num += response.size();
  if (ArgusTV::RequestRadioChannelGroups(response) >= 0) num += response.size();
  return num;
}

PVR_ERROR cPVRClientArgusTV::GetChannelGroups(ADDON_HANDLE handle, bool bRadio)
{
  Json::Value response;
  int retval;

  if (bRadio && !g_bRadioEnabled) return PVR_ERROR_NO_ERROR;

  if (!bRadio)
  {
    retval = ArgusTV::RequestTVChannelGroups(response);
  }
  else
  {
    retval = ArgusTV::RequestRadioChannelGroups(response);
  }
  if (retval >= 0)
  {
    int size = response.size();

    // parse channel group list
    for (int index = 0; index < size; ++index)
    {
      std::string name = response[index]["GroupName"].asString();
      std::string guid = response[index]["ChannelGroupId"].asString();
      if (!bRadio)
      {
        XBMC->Log(LOG_DEBUG, "Found TV channel group %s: %s\n", guid.c_str(), name.c_str());
      }
      else
      {
        XBMC->Log(LOG_DEBUG, "Found Radio channel group %s: %s\n", guid.c_str(), name.c_str());
      }
      PVR_CHANNEL_GROUP tag;
      memset(&tag, 0 , sizeof(PVR_CHANNEL_GROUP));

      tag.bIsRadio     = bRadio;
      strncpy(tag.strGroupName, name.c_str(), sizeof(tag.strGroupName));

      PVR->TransferChannelGroup(handle, &tag);
    }
    return PVR_ERROR_NO_ERROR;
  }
  else
  {
    return PVR_ERROR_SERVER_ERROR;
  }
}

PVR_ERROR cPVRClientArgusTV::GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group)
{
  Json::Value response;
  int retval;

  // Step 1, find the GUID for this channelgroup
  if (!group.bIsRadio)
  {
    retval = ArgusTV::RequestTVChannelGroups(response);
  }
  else
  {
    retval = ArgusTV::RequestRadioChannelGroups(response);
  }
  if (retval < 0)
  {
    XBMC->Log(LOG_ERROR, "Could not get Channelgroups from server.");
    return PVR_ERROR_SERVER_ERROR;
  }

  std::string guid = "";
  std::string name = "";
  int size = response.size();
  for (int index = 0; index < size; ++index)
  {
    name = response[index]["GroupName"].asString();
    guid = response[index]["ChannelGroupId"].asString();
    if (name == group.strGroupName) break;
  }
  if (name != group.strGroupName)
  {
    XBMC->Log(LOG_ERROR, "Channelgroup %s was not found while trying to retrieve the channelgroup members.", group.strGroupName);
    return PVR_ERROR_SERVER_ERROR;
  }

  // Step 2 use the guid to retrieve the list of member channels
  retval = ArgusTV::RequestChannelGroupMembers(guid, response);
  if (retval < 0)
  {
    XBMC->Log(LOG_ERROR, "Could not get members for Channelgroup \"%s\" (%s) from server.", name.c_str(), guid.c_str());
    return PVR_ERROR_SERVER_ERROR;
  }
  size = response.size();
  for (int index = 0; index < size; index++)
  {
    std::string channelId = response[index]["ChannelId"].asString();
    std::string channelName = response[index]["DisplayName"].asString();
    cChannel* pChannel    = FetchChannel(channelId);
    if (pChannel == NULL)
    {
      XBMC->Log(LOG_ERROR, "Unable to translate channel \"%s\" (\"%s\") to XBMC channel number, channel group member skipped.",
        channelId.c_str(), channelName.c_str());
      XBMC->QueueNotification(QUEUE_ERROR, "GUID to XBMC Channel");
      continue;
    }

    PVR_CHANNEL_GROUP_MEMBER tag;
    memset(&tag,0 , sizeof(PVR_CHANNEL_GROUP_MEMBER));

    strncpy(tag.strGroupName, group.strGroupName, sizeof(tag.strGroupName));
    tag.iChannelUniqueId = pChannel->ID();
    tag.iChannelNumber   = pChannel->LCN();

    XBMC->Log(LOG_DEBUG, "%s - add channel %s (%d) to group '%s' channel number %d",
      __FUNCTION__, pChannel->Name(), tag.iChannelUniqueId, tag.strGroupName, tag.iChannelNumber);

    PVR->TransferChannelGroupMember(handle, &tag);
  }
  return PVR_ERROR_NO_ERROR;
}

/************************************************************/
/** Record handling **/

int cPVRClientArgusTV::GetNumRecordings(void)
{
  Json::Value response;
  int retval = -1;
  int iNumRecordings = 0;

  XBMC->Log(LOG_DEBUG, "GetNumRecordings()");
  retval = ArgusTV::GetRecordingGroupByTitle(response);
  if (retval >= 0)
  {
    int size = response.size();

    // parse channelgroup list
    for ( int index = 0; index < size; ++index )
    {
      cRecordingGroup recordinggroup;
      if (recordinggroup.Parse(response[index]))
      {
        iNumRecordings += recordinggroup.RecordingsCount();
      }
    }
  }
  return iNumRecordings;
}

PVR_ERROR cPVRClientArgusTV::GetRecordings(ADDON_HANDLE handle)
{
  Json::Value recordinggroupresponse;
  int retval = -1;
  int iNumRecordings = 0;

  XBMC->Log(LOG_DEBUG, "RequestRecordingsList()");
  retval = ArgusTV::GetRecordingGroupByTitle(recordinggroupresponse);
  if(retval >= 0)
  {           
    // process list of recording groups
    int size = recordinggroupresponse.size();
    for ( int recordinggroupindex = 0; recordinggroupindex < size; ++recordinggroupindex )
    {
      cRecordingGroup recordinggroup;
      if (recordinggroup.Parse(recordinggroupresponse[recordinggroupindex]))
      {
        Json::Value recordingsbytitleresponse;
        retval = ArgusTV::GetRecordingsForTitle(recordinggroup.ProgramTitle(), recordingsbytitleresponse);
        if (retval >= 0)
        {
          // process list of recording summaries for this group
          int nrOfRecordings = recordingsbytitleresponse.size();
          for (int recordingindex = 0; recordingindex < nrOfRecordings; recordingindex++)
          {
            cRecording recording;

            cRecordingSummary recordingsummary;
            if (recordingsummary.Parse(recordingsbytitleresponse[recordingindex]) && FetchRecordingDetails(recordingsummary.RecordingId(), recording))
            {
              PVR_RECORDING tag;
              memset(&tag, 0 , sizeof(tag));

              strncpy(tag.strRecordingId, recording.RecordingId(), sizeof(tag.strRecordingId));
              strncpy(tag.strChannelName, recording.ChannelDisplayName(), sizeof(tag.strChannelName));
              tag.iLifetime      = MAXLIFETIME; //TODO: recording.Lifetime();
              tag.iPriority      = 0; //TODO? recording.Priority();
              tag.recordingTime  = recording.RecordingStartTime();
              tag.iDuration      = recording.RecordingStopTime() - recording.RecordingStartTime();
              strncpy(tag.strPlot, recording.Description(), sizeof(tag.strPlot));
              tag.iPlayCount     = recording.FullyWatchedCount();
              if (nrOfRecordings > 1)
              {
                recording.Transform(true);
                strncpy(tag.strDirectory, recordinggroup.ProgramTitle().c_str(), sizeof(tag.strDirectory)); //used in XBMC as directory structure below "Server X - hostname"
              }
              else
              {
                recording.Transform(false);
                tag.strDirectory[0] = '\0';
              }
              strncpy(tag.strTitle, recording.Title(), sizeof(tag.strTitle));
              strncpy(tag.strPlotOutline, recording.SubTitle(), sizeof(tag.strPlotOutline));
#ifdef TARGET_WINDOWS
              strncpy(tag.strStreamURL, recording.RecordingFileName(), sizeof(tag.strStreamURL));
#else
              strncpy(tag.strStreamURL, recording.CIFSRecordingFileName(), sizeof(tag.strStreamURL));
#endif
              PVR->TransferRecordingEntry(handle, &tag);
              iNumRecordings++;
            }
          }
        }
      }
    }
  }
  return PVR_ERROR_NO_ERROR;
}

bool cPVRClientArgusTV::FetchRecordingDetails(std::string recordingid, cRecording& recording)
{ 
  bool fRc = false;
  Json::Value recordingresponse;

  cRecordingSummary recordingsummary;
    int retval = ArgusTV::GetRecordingById(recordingid, recordingresponse);
    if (retval >= 0)
    {
      if (recordingresponse.type() == Json::objectValue)
      {
        fRc = recording.Parse(recordingresponse);
      }
    }
  return fRc;
}

PVR_ERROR cPVRClientArgusTV::DeleteRecording(const PVR_RECORDING &recinfo)
{
  PVR_ERROR rc = PVR_ERROR_FAILED;

  XBMC->Log(LOG_DEBUG, "->DeleteRecording(%s)", recinfo.strRecordingId);

  cRecording recording;
  if (FetchRecordingDetails(recinfo.strRecordingId, recording))
  {
    XBMC->Log(LOG_DEBUG, "->DeleteRecording(%s == \"%s\")", recinfo.strRecordingId, recording.RecordingFileName());
    // JSONify the stream_url
    Json::Value recordingname (recording.RecordingFileName());
    Json::StyledWriter writer;
    std::string jsonval = writer.write(recordingname);
    if (ArgusTV::DeleteRecording(jsonval) >= 0)
    {
      // Trigger XBMC to update it's list
      PVR->TriggerRecordingUpdate();
      rc =  PVR_ERROR_NO_ERROR;
    }
  }
  return rc;
}

PVR_ERROR cPVRClientArgusTV::RenameRecording(const PVR_RECORDING &recinfo)
{
  NOTUSED(recinfo);
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR cPVRClientArgusTV::SetRecordingLastPlayedPosition(const PVR_RECORDING &recinfo, int lastplayedposition)
{
  XBMC->Log(LOG_DEBUG, "->SetRecordingLastPlayedPosition(index=%s [%s], %d)", recinfo.strRecordingId, recinfo.strStreamURL, lastplayedposition);

  std::string recordingfilename = recinfo.strStreamURL;
#if !defined(TARGET_WINDOWS)
  recordingfilename = ArgusTV::ToUNC(recordingfilename);
#endif

  // JSONify the stream_url
  Json::Value recordingname (recordingfilename);
  Json::FastWriter writer;
  std::string jsonval = writer.write(recordingname);
  int retval = ArgusTV::SetRecordingLastWatchedPosition(jsonval, lastplayedposition);
  if (retval < 0)
  {
    XBMC->Log(LOG_INFO, "Failed to set recording last watched position (%d)", retval);
    return PVR_ERROR_SERVER_ERROR;
  }

  return PVR_ERROR_NO_ERROR;
}

int cPVRClientArgusTV::GetRecordingLastPlayedPosition(const PVR_RECORDING &recinfo)
{
  XBMC->Log(LOG_DEBUG, "->GetRecordingLastPlayedPosition(index=%s [%s])", recinfo.strRecordingId, recinfo.strStreamURL);

  std::string recordingfilename = recinfo.strStreamURL;
#if !defined(TARGET_WINDOWS)
  recordingfilename = ArgusTV::ToUNC(recordingfilename);
#endif

  // JSONify the stream_url
  Json::Value response;
  Json::Value recordingname (recordingfilename);
  Json::FastWriter writer;
  std::string jsonval = writer.write(recordingname);
  int retval = ArgusTV::GetRecordingLastWatchedPosition(jsonval, response);
  if (retval < 0)
  {
    XBMC->Log(LOG_INFO, "Failed to get recording last watched position (%d)", retval);
    return 0;
  }

  retval = response.asInt();
  XBMC->Log(LOG_DEBUG, "GetRecordingLastPlayedPosition(index=%s [%s]) returns %d.\n", recinfo.strRecordingId, recinfo.strStreamURL, retval);

  return retval;
}

PVR_ERROR cPVRClientArgusTV::SetRecordingPlayCount(const PVR_RECORDING &recinfo, int playcount)
{
  XBMC->Log(LOG_DEBUG, "->SetRecordingPlayCount(index=%s [%s], %d)", recinfo.strRecordingId, recinfo.strStreamURL, playcount);

  std::string recordingfilename = recinfo.strStreamURL;
#if !defined(TARGET_WINDOWS)
  recordingfilename = ArgusTV::ToUNC(recordingfilename);
#endif

  // JSONify the stream_url
  Json::Value recordingname (recordingfilename);
  Json::FastWriter writer;
  std::string jsonval = writer.write(recordingname);
  int retval = ArgusTV::SetRecordingFullyWatchedCount(jsonval, playcount);
  if (retval < 0)
  {
    XBMC->Log(LOG_INFO, "Failed to set recording play count (%d)", retval);
    return PVR_ERROR_SERVER_ERROR;
  }

  return PVR_ERROR_NO_ERROR;
}


/************************************************************/
/** Timer handling */

int cPVRClientArgusTV::GetNumTimers(void)
{
  // Not directly possible in ARGUS TV
  Json::Value response;

  XBMC->Log(LOG_DEBUG, "GetNumTimers()");
  // pick up the schedulelist for TV
  int retval = ArgusTV::GetUpcomingRecordings(response);
  if (retval < 0) 
  {
    return 0;
  }

  return response.size();
}

PVR_ERROR cPVRClientArgusTV::GetTimers(ADDON_HANDLE handle)
{
  Json::Value activeRecordingsResponse, upcomingRecordingsResponse;
  int         iNumberOfTimers = 0;
  PVR_TIMER   tag;
  int         numberoftimers;

  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  // retrieve the currently active recordings
  int retval = ArgusTV::GetActiveRecordings(activeRecordingsResponse);
  if (retval < 0) 
  {
    XBMC->Log(LOG_ERROR, "Unable to retrieve active recordings from server.");
    return PVR_ERROR_SERVER_ERROR;
  }

  // pick up the upcoming recordings
  retval = ArgusTV::GetUpcomingRecordings(upcomingRecordingsResponse);
  if (retval < 0) 
  {
    XBMC->Log(LOG_ERROR, "Unable to retrieve upcoming programs from server.");
    return PVR_ERROR_SERVER_ERROR;
  }

  memset(&tag, 0 , sizeof(tag));
  numberoftimers = upcomingRecordingsResponse.size();

  for (int i = 0; i < numberoftimers; i++)
  {
    cUpcomingRecording upcomingrecording;
    if (upcomingrecording.Parse(upcomingRecordingsResponse[i]))
    {
      tag.iClientIndex      = iNumberOfTimers;
      cChannel* pChannel    = FetchChannel(upcomingrecording.ChannelId());
      if (pChannel == NULL)
      {
        XBMC->Log(LOG_ERROR, "Unable to translate channel \"%s\" (\"%s\") to XBMC channel number, timer skipped.",
          upcomingrecording.ChannelId().c_str(), upcomingrecording.ChannelDisplayname().c_str());
        XBMC->QueueNotification(QUEUE_ERROR, "GUID to XBMC Channel");
        continue;
      }
      tag.iClientChannelUid = pChannel->ID();
      tag.startTime         = upcomingrecording.StartTime();
      tag.endTime           = upcomingrecording.StopTime();

      // build the XBMC PVR State
      if (upcomingrecording.IsCancelled())
      {
        tag.state             = PVR_TIMER_STATE_CANCELLED;
      }
      else if (upcomingrecording.IsInConflict())
      {
        if (upcomingrecording.IsAllocated())
          tag.state             = PVR_TIMER_STATE_CONFLICT_OK;
        else
          tag.state             = PVR_TIMER_STATE_CONFLICT_NOK;
      }
      else if (!upcomingrecording.IsAllocated())
      {
        //not allocated --> won't be recorded
        tag.state             = PVR_TIMER_STATE_ERROR;
      }
      else
      {
        tag.state             = PVR_TIMER_STATE_SCHEDULED;
      }

      if (tag.state == PVR_TIMER_STATE_SCHEDULED || tag.state == PVR_TIMER_STATE_CONFLICT_OK) //check if they are currently recording
      {
        if (activeRecordingsResponse.size() > 0)
        {
          // Is the this upcoming recording in the list of active recordings?
          for (Json::Value::UInt j = 0; j < activeRecordingsResponse.size(); j++)
          {
            cActiveRecording activerecording;
            if (activerecording.Parse(activeRecordingsResponse[j]))
            {
              if (upcomingrecording.UpcomingProgramId() == activerecording.UpcomingProgramId())
              {
                tag.state = PVR_TIMER_STATE_RECORDING;
                break;
              }
            }
          }
        }
      }

      strncpy(tag.strTitle, upcomingrecording.Title().c_str(), sizeof(tag.strTitle));
      tag.strDirectory[0]   = '\0';
      tag.strSummary[0]     = '\0';
      tag.iPriority         = 0;
      tag.iLifetime         = 0;
      tag.bIsRepeating      = false;
      tag.firstDay          = 0;
      tag.iWeekdays         = 0;
      tag.iEpgUid           = 0;
      tag.iMarginStart      = upcomingrecording.PreRecordSeconds() / 60;
      tag.iMarginEnd        = upcomingrecording.PostRecordSeconds() / 60;
      tag.iGenreType        = 0;
      tag.iGenreSubType     = 0;

      PVR->TransferTimerEntry(handle, &tag);
      iNumberOfTimers++;
    }
  }

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR cPVRClientArgusTV::AddTimer(const PVR_TIMER &timerinfo)
{
  XBMC->Log(LOG_DEBUG, "AddTimer(start @ %d, end @ %d)", timerinfo.startTime, timerinfo.endTime);

  // re-synthesize the ARGUS TV channel GUID
  cChannel* pChannel = FetchChannel(timerinfo.iClientChannelUid);
  if (pChannel == NULL)
  {
    XBMC->Log(LOG_ERROR, "Unable to translate XBMC channel %d to ARGUS TV channel GUID, timer not added.",
      timerinfo.iClientChannelUid);
    XBMC->QueueNotification(QUEUE_ERROR, "XBMC Channel to GUID");
    return PVR_ERROR_SERVER_ERROR;
  }

  Json::Value addScheduleResponse;
  time_t starttime = timerinfo.startTime;
  if (starttime == 0) starttime = time(NULL);
  int retval = ArgusTV::AddOneTimeSchedule(pChannel->Guid(), starttime, timerinfo.strTitle, timerinfo.iMarginStart * 60, timerinfo.iMarginEnd * 60, timerinfo.iLifetime, addScheduleResponse);
  if (retval < 0) 
  {
    return PVR_ERROR_SERVER_ERROR;
  }

  std::string scheduleid = addScheduleResponse["ScheduleId"].asString();

  // Ok, we created a schedule, but did that lead to an upcoming recording?
  Json::Value upcomingProgramsResponse;
  retval = ArgusTV::GetUpcomingProgramsForSchedule(addScheduleResponse, upcomingProgramsResponse);

  // We should have at least one upcoming program for this schedule, otherwise nothing will be recorded
  if (retval <= 0)
  {
    XBMC->Log(LOG_INFO, "The new schedule does not lead to an upcoming program, removing schedule and adding a manual one.");
    // remove the added (now stale) schedule, ignore failure (what are we to do anyway?)
    ArgusTV::DeleteSchedule(scheduleid);

    // Okay, add a manual schedule (forced recording) but now we need to add pre- and post-recording ourselves
    time_t manualStartTime = starttime - (timerinfo.iMarginStart * 60);
    time_t manualEndTime = timerinfo.endTime + (timerinfo.iMarginEnd * 60);
    retval = ArgusTV::AddManualSchedule(pChannel->Guid(), manualStartTime, manualEndTime - manualStartTime, timerinfo.strTitle, timerinfo.iMarginStart * 60, timerinfo.iMarginEnd * 60, timerinfo.iLifetime, addScheduleResponse);
    if (retval < 0)
    {
      XBMC->Log(LOG_ERROR, "A manual schedule could not be added.");
      return PVR_ERROR_SERVER_ERROR;
    }
  }

  // Trigger an update of the PVR timers
  PVR->TriggerTimerUpdate();
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR cPVRClientArgusTV::DeleteTimer(const PVR_TIMER &timerinfo, bool force)
{
  NOTUSED(force);
  Json::Value upcomingProgramsResponse, activeRecordingsResponse;

  XBMC->Log(LOG_DEBUG, "DeleteTimer()");

  // re-synthesize the ARGUS TV startime, stoptime and channel GUID
  time_t starttime = timerinfo.startTime;
  time_t stoptime = timerinfo.endTime;
  cChannel* pChannel = FetchChannel(timerinfo.iClientChannelUid);
  if (pChannel == NULL)
  {
    XBMC->Log(LOG_ERROR, "Unable to translate XBMC channel %d to ARGUS TV channel GUID, timer not deleted.",
      timerinfo.iClientChannelUid);
    XBMC->QueueNotification(QUEUE_ERROR, "XBMC Channel to GUID");
    return PVR_ERROR_SERVER_ERROR;
  }

  // retrieve the currently active recordings
  int retval = ArgusTV::GetActiveRecordings(activeRecordingsResponse);
  if (retval < 0) 
  {
    XBMC->Log(LOG_ERROR, "Unable to retrieve active recordings from server.");
    return PVR_ERROR_SERVER_ERROR;
  }

  // pick up the upcoming recordings
  retval = ArgusTV::GetUpcomingRecordings(upcomingProgramsResponse);
  if (retval < 0) 
  {
    XBMC->Log(LOG_ERROR, "Unable to retrieve upcoming programs from server.");
    return PVR_ERROR_SERVER_ERROR;
  }

  // try to find the upcoming recording that matches this xbmc timer
  int numberoftimers = upcomingProgramsResponse.size();
  for (int i = 0; i < numberoftimers; i++)
  {
    cUpcomingRecording upcomingrecording;
    if (upcomingrecording.Parse(upcomingProgramsResponse[i]))
    {
      if (upcomingrecording.ChannelId() == pChannel->Guid())
      {
        if (upcomingrecording.StartTime() == starttime)
        {
          if (upcomingrecording.StopTime() == stoptime)
          {
            // Okay, we matched the timer to an upcoming program, but is it recording right now?
            if (activeRecordingsResponse.size() > 0)
            {
              // Is the this upcoming program in the list of active recordings?
              for (Json::Value::UInt j = 0; j < activeRecordingsResponse.size(); j++)
              {
                cActiveRecording activerecording;
                if (activerecording.Parse(activeRecordingsResponse[j]))
                {
                  if (upcomingrecording.UpcomingProgramId() == activerecording.UpcomingProgramId())
                  {
                    // Abort this recording
                    retval = ArgusTV::AbortActiveRecording(activeRecordingsResponse[j]);
                    if (retval != 0)
                    {
                      XBMC->Log(LOG_ERROR, "Unable to cancel the active recording of \"%s\" on the server. Will try to cancel the program.", upcomingrecording.Title().c_str());
                    }
                    break;
                  }
                }
              }
            }

            Json::Value scheduleResponse;
            retval = ArgusTV::GetScheduleById(upcomingrecording.ScheduleId(), scheduleResponse);
            std::string schedulename = scheduleResponse["Name"].asString();

            if (scheduleResponse["IsOneTime"].asBool() == true)
            {
              retval = ArgusTV::DeleteSchedule(upcomingrecording.ScheduleId());
              if (retval < 0) 	
              {
                XBMC->Log(LOG_NOTICE, "Unable to delete schedule %s from server.", schedulename.c_str());
                return PVR_ERROR_SERVER_ERROR;
              }
            }
            else
            {
              retval = ArgusTV::CancelUpcomingProgram(upcomingrecording.ScheduleId(), upcomingrecording.ChannelId(), 
                upcomingrecording.StartTime(), upcomingrecording.GuideProgramId());
              if (retval < 0) 
              {
                XBMC->Log(LOG_ERROR, "Unable to cancel upcoming program from server.");
                return PVR_ERROR_SERVER_ERROR;	
              }
            }

            // Trigger an update of the PVR timers
            PVR->TriggerTimerUpdate();
            return PVR_ERROR_NO_ERROR;
          }
        }
      }
    }
  }
  return PVR_ERROR_SERVER_ERROR;
}

PVR_ERROR cPVRClientArgusTV::UpdateTimer(const PVR_TIMER &timerinfo)
{
  NOTUSED(timerinfo);
  return PVR_ERROR_NOT_IMPLEMENTED;
}


/************************************************************/
/** Live stream handling */
cChannel* cPVRClientArgusTV::FetchChannel(int channel_uid, bool LogError)
{
  // Search for this channel in our local channel list to find the original ChannelID back:
  vector<cChannel>::iterator it;

  for ( it=m_Channels.begin(); it < m_Channels.end(); it++ )
  {
    if (it->ID() == channel_uid)
    {
      return &*it;
    }
  }

  if (LogError) XBMC->Log(LOG_ERROR, "XBMC channel with id %d not found in the channel cache!.", channel_uid);
  return NULL;
}

cChannel* cPVRClientArgusTV::FetchChannel(std::string channelid, bool LogError)
{
  // Search for this channel in our local channel list to find the original ChannelID back:
  vector<cChannel>::iterator it;

  for ( it=m_Channels.begin(); it < m_Channels.end(); it++ )
  {
    if (it->Guid() == channelid)
    {
      return &*it;
    }
  }
  
  if (LogError) XBMC->Log(LOG_ERROR, "ARGUS TV channel with GUID \"%s\" not found in the channel cache!.", channelid.c_str());
  return NULL;
}

bool cPVRClientArgusTV::_OpenLiveStream(const PVR_CHANNEL &channelinfo)
{
  XBMC->Log(LOG_DEBUG, "->_OpenLiveStream(%i)", channelinfo.iUniqueId);

  cChannel* channel = FetchChannel(channelinfo.iUniqueId);

  if (channel)
  {
    std::string filename;
    XBMC->Log(LOG_INFO, "Tune XBMC channel: %i", channelinfo.iUniqueId);
    XBMC->Log(LOG_INFO, "Corresponding ARGUS TV channel: %s", channel->Guid().c_str());

    int retval = ArgusTV::TuneLiveStream(channel->Guid(), channel->Type(), channel->Name(), filename);
    if (retval == ArgusTV::NoReTunePossible)
    {
      // Ok, we can't re-tune with the current live stream still running
      // So stop it and re-try
      CloseLiveStream();
      XBMC->Log(LOG_INFO, "Re-Tune XBMC channel: %i", channelinfo.iUniqueId);
      retval = ArgusTV::TuneLiveStream(channel->Guid(), channel->Type(), channel->Name(), filename);
    }

    if (retval != E_SUCCESS)
    {
      switch (retval)
      {
        case ArgusTV::NoFreeCardFound:
          XBMC->Log(LOG_INFO, "No free tuner found.");
          XBMC->QueueNotification(QUEUE_ERROR, "No free tuner found!");
          break;
        case ArgusTV::IsScrambled:
          XBMC->Log(LOG_INFO, "Scrambled channel.");
          XBMC->QueueNotification(QUEUE_ERROR, "Scrambled channel!");
          break;
        case ArgusTV::ChannelTuneFailed:
          XBMC->Log(LOG_INFO, "Tuning failed.");
          XBMC->QueueNotification(QUEUE_ERROR, "Tuning failed!");
          break;
        default:
          XBMC->Log(LOG_ERROR, "Tuning failed, unknown error");
          XBMC->QueueNotification(QUEUE_ERROR, "Unknown error!");
          break;
      }
    }

    std::string CIFSname = filename;
    std::string SMBPrefix = "smb://";
    //if (g_szUser.length() > 0)
    //{
    //  SMBPrefix += g_szUser;
    //  if (g_szPass.length() > 0)
    //  {
    //    SMBPrefix += ":" + g_szPass;
    //  }
    //}
    //else
    //{
    //  SMBPrefix += "Guest";
    //}
    //SMBPrefix += "@";
    size_t found;
    while ((found = CIFSname.find("\\")) != std::string::npos)
    {
      CIFSname.replace(found, 1, "/");
    }
    CIFSname.erase(0,2);
    CIFSname.insert(0, SMBPrefix.c_str());
    filename = CIFSname;

    if (retval != E_SUCCESS || filename.length() == 0)
    {
      XBMC->Log(LOG_ERROR, "Could not start the timeshift for channel %i (%s)", channelinfo.iUniqueId, channel->Guid().c_str());
      CloseLiveStream();
      return false;
    }

    // reset the signal quality poll interval after tuning
    m_signalqualityInterval = 0;

    XBMC->Log(LOG_INFO, "Live stream file: %s", filename.c_str());
    m_bTimeShiftStarted = true;
    m_iCurrentChannel = channelinfo.iUniqueId;
    if (!m_keepalive.IsRunning())
    {
      if (!m_keepalive.CreateThread())
      {
        XBMC->Log(LOG_ERROR, "Start keepalive thread failed.");
      }
    }

#if defined(ATV_DUMPTS)
    if (ofd != -1) close(ofd);
    strncpy(ofn, "/tmp/atv.XXXXXX", sizeof(ofn));
    if ((ofd = mkostemp(ofn, O_CREAT|O_TRUNC)) == -1)
    {
      XBMC->Log(LOG_ERROR, "couldn't open dumpfile %s (error %d: %s).", ofn, errno, strerror(errno));
    }
    else
    {
      XBMC->Log(LOG_INFO, "opened dumpfile %s.", ofn);
    }
#endif

    if (m_tsreader != NULL)
    {
      //XBMC->Log(LOG_DEBUG, "Re-using existing TsReader...");
      //usleep(5000000);
      //m_tsreader->OnZap();
      XBMC->Log(LOG_DEBUG, "Close existing and open new TsReader...");
      m_tsreader->Close();
      SAFE_DELETE(m_tsreader);
    }
    // Open Timeshift buffer
    // TODO: rtsp support
    m_tsreader = new CTsReader();
    XBMC->Log(LOG_DEBUG, "Open TsReader");
    m_tsreader->Open(filename.c_str());
    m_tsreader->OnZap();
    XBMC->Log(LOG_DEBUG, "Delaying %ld milliseconds.", (1000 * g_iTuneDelay));
    usleep(1000 * g_iTuneDelay);
    return true;
  }
  else
  {
    XBMC->Log(LOG_ERROR, "Could not get ARGUS TV channel guid for channel %i.", channelinfo.iUniqueId);
    XBMC->QueueNotification(QUEUE_ERROR, "XBMC Channel to GUID");
  }

  CloseLiveStream();
  return false;
}

bool cPVRClientArgusTV::OpenLiveStream(const PVR_CHANNEL &channelinfo)
{
  return _OpenLiveStream(channelinfo);
}

int cPVRClientArgusTV::ReadLiveStream(unsigned char* pBuffer, unsigned int iBufferSize)
{
  unsigned long read_wanted = iBufferSize;
  unsigned long read_done   = 0;
  static int read_timeouts  = 0;
  unsigned char* bufptr = pBuffer;

  // XBMC->Log(LOG_DEBUG, "->ReadLiveStream(buf_size=%i)", iBufferSize);
  if (!m_tsreader)
    return -1;

  while (read_done < (unsigned long) iBufferSize)
  {
    read_wanted = iBufferSize - read_done;

    long lRc = 0;
    if ((lRc = m_tsreader->Read(bufptr, read_wanted, &read_wanted)) > 0)
    {
      usleep(400000);
      read_timeouts++;
      XBMC->Log(LOG_NOTICE, "ReadLiveStream requested %d but only read %d bytes.", iBufferSize, read_wanted);
      return read_wanted;
    }
    read_done += read_wanted;

    if ( read_done < (unsigned long) iBufferSize )
    {
      if (read_timeouts > 25)
      {
        XBMC->Log(LOG_INFO, "No data in 1 second");
        read_timeouts = 0;
        return read_done;
      }
      bufptr += read_wanted;
      read_timeouts++;
      usleep(40000);
    }
  }
#if defined(ATV_DUMPTS)
  if (write(ofd, pBuffer, read_done) < 0)
  {
    XBMC->Log(LOG_ERROR, "couldn't write %d bytes to dumpfile %s (error %d: %s).", read_done, ofn, errno, strerror(errno));
  }
#endif
  // XBMC->Log(LOG_DEBUG, "ReadLiveStream(buf_size=%i), %d timeouts", iBufferSize, read_timeouts);
  read_timeouts = 0;
  return read_done;
}

long long cPVRClientArgusTV::SeekLiveStream(long long pos, int whence)
{
  static std::string zz[] = { "Begin", "Current", "End" };
  XBMC->Log(LOG_DEBUG, "SeekLiveStream (%lld, %s).", pos, zz[whence].c_str());
  if (!m_tsreader)
  {
    return -1;
  }
  return m_tsreader->SetFilePointer(pos, whence);
}


long long cPVRClientArgusTV::PositionLiveStream(void)
{
  if (!m_tsreader)
  {
    return -1;
  }
  return m_tsreader->GetFilePointer();
}

long long cPVRClientArgusTV::LengthLiveStream(void)
{
  if (!m_tsreader)
  {
    return -1;
  }
  return m_tsreader->GetFileSize();
}


void cPVRClientArgusTV::CloseLiveStream()
{
  string result;
  XBMC->Log(LOG_INFO, "CloseLiveStream");

  if (m_keepalive.IsRunning())
  {
    if (!m_keepalive.StopThread())
    {
      XBMC->Log(LOG_ERROR, "Stop keepalive thread failed.");
    }
  } 

#if defined(ATV_DUMPTS)
  if (ofd != -1)
  {
    if (close(ofd) == -1)
    {
      XBMC->Log(LOG_ERROR, "couldn't close dumpfile %s (error %d: %s).", ofn, errno, strerror(errno));
    }
    ofd = -1;
  }
#endif

  if (m_bTimeShiftStarted)
  {
    if (m_tsreader)
    {
      XBMC->Log(LOG_DEBUG, "Close TsReader");
      m_tsreader->Close();
#if defined(TARGET_WINDOWS)
      XBMC->Log(LOG_DEBUG, "ReadLiveStream: %I64d calls took %I64d nanoseconds.", m_tsreader->sigmaCount(), m_tsreader->sigmaTime());
#endif
      SAFE_DELETE(m_tsreader);
    }
    ArgusTV::StopLiveStream();
    m_bTimeShiftStarted = false;
    m_iCurrentChannel = -1;
  } else {
    XBMC->Log(LOG_DEBUG, "CloseLiveStream: Nothing to do.");
  }
}


bool cPVRClientArgusTV::SwitchChannel(const PVR_CHANNEL &channelinfo)
{
  XBMC->Log(LOG_DEBUG, "->SwitchChannel(%i)", channelinfo.iUniqueId);
  bool fRc = false;

  if (g_iTuneDelay == 0)
  {
    // Close existing live stream before opening a new one.
    // This is slower, but it helps XBMC playback when the streams change types (e.g. SD->HD).
    // It also gives a better tuner allocation when using multiple clients with a limited count of tuners.  	
    CloseLiveStream();
  }
  fRc = OpenLiveStream(channelinfo);

  return fRc;
}


int cPVRClientArgusTV::GetCurrentClientChannel()
{
  return m_iCurrentChannel;
}

PVR_ERROR cPVRClientArgusTV::SignalStatus(PVR_SIGNAL_STATUS &signalStatus)
{
  static PVR_SIGNAL_STATUS tag;

  // Only do the REST call once out of N
  if (m_signalqualityInterval-- <= 0)
  {
    m_signalqualityInterval = SIGNALQUALITY_INTERVAL;
    Json::Value response;
    ArgusTV::SignalQuality(response);
    memset(&tag, 0, sizeof(tag));
    std::string cardtype = "";
    switch (response["CardType"].asInt())
    {
    case 0x80:
      cardtype = "Analog";
      break;
    case 8:
      cardtype = "ATSC";
      break;
    case 4:
      cardtype = "DVB-C";
      break;
    case 0x10:
      cardtype = "DVB-IP";
      break;
    case 1:
      cardtype = "DVB-S";
      break;
    case 2:
      cardtype = "DVB-T";
      break;
    default:
      cardtype = "Unknown card type";
      break;
    }
    snprintf(tag.strAdapterName, 1024, "Provider %s, %s",
      response["ProviderName"].asString().c_str(),
      cardtype.c_str());
    snprintf(tag.strAdapterStatus, 1024, "%s, %s",
      response["Name"].asString().c_str(),
      response["IsFreeToAir"].asBool() ? "free to air" : "encrypted");
    tag.iSNR = (int) (response["SignalQuality"].asInt() * 655.35);
    tag.iSignal = (int) (response["SignalStrength"].asInt() * 655.35);
  }

  signalStatus = tag;
  return PVR_ERROR_NO_ERROR;
}


/************************************************************/
/** Record stream handling */
bool cPVRClientArgusTV::OpenRecordedStream(const PVR_RECORDING &recinfo)
{
  XBMC->Log(LOG_DEBUG, "->OpenRecordedStream(index=%s)", recinfo.strRecordingId);
  cRecording recording;
  if (!FetchRecordingDetails(recinfo.strRecordingId, recording))
  {
    XBMC->Log(LOG_ERROR, "Unable to fetch recording details for %s", recinfo.strRecordingId);
    return false;
  }

#if TARGET_WINDOWS
  const char* recordingName = recording.RecordingFileName();
#else
  const char* recordingName = recording.CIFSRecordingFileName();
#endif

  if (m_tsreader != NULL)
  {
    XBMC->Log(LOG_DEBUG, "Close existing TsReader...");
    m_tsreader->Close();
    SAFE_DELETE(m_tsreader);
  }
  m_tsreader = new CTsReader();
  if (m_tsreader->Open(recordingName) != S_OK)
  {
    SAFE_DELETE(m_tsreader);
    return false;
  }

  return true;
}


void cPVRClientArgusTV::CloseRecordedStream(void)
{
  XBMC->Log(LOG_DEBUG, "->CloseRecordedStream()");
  if (m_tsreader)
  {
    XBMC->Log(LOG_DEBUG, "Close TsReader");
    m_tsreader->Close();
    SAFE_DELETE(m_tsreader);
  }
}

int cPVRClientArgusTV::ReadRecordedStream(unsigned char* pBuffer, unsigned int iBuffersize)
{
  unsigned long read_done   = 0;

  // XBMC->Log(LOG_DEBUG, "->ReadRecordedStream(buf_size=%i)", iBufferSize);
  if (!m_tsreader)
    return -1;

  long lRc = 0;
  if ((lRc = m_tsreader->Read(pBuffer, iBuffersize, &read_done)) > 0)
  {
    XBMC->Log(LOG_NOTICE, "ReadRecordedStream requested %d but only read %d bytes.", iBuffersize, read_done);
  }
  return read_done;
}

long long cPVRClientArgusTV::SeekRecordedStream(long long iPosition, int iWhence)
{
  if (!m_tsreader)
  {
    return -1;
  }
  if (iPosition == 0 && iWhence == SEEK_CUR)
  {
    return m_tsreader->GetFilePointer();
  }
  return m_tsreader->SetFilePointer(iPosition, iWhence);
}

long long cPVRClientArgusTV::PositionRecordedStream(void)
{ 
  if (!m_tsreader)
  {
    return -1;
  }
  return m_tsreader->GetFilePointer();
}

long long cPVRClientArgusTV::LengthRecordedStream(void)
{
  if (!m_tsreader)
  {
    return -1;
  }
  return m_tsreader->GetFileSize();
}

/*
 * \brief Request the stream URL for live tv/live radio.
 */
const char* cPVRClientArgusTV::GetLiveStreamURL(const PVR_CHANNEL &channelinfo)
{
  XBMC->Log(LOG_DEBUG, "->GetLiveStreamURL(%i)", channelinfo.iUniqueId);
  bool rc = _OpenLiveStream(channelinfo);
  if (rc)
  {
    m_bTimeShiftStarted = true;
  }
  // sigh, the only reason to use a class member here is to have storage for the const char *
  // pointing to the std::string when this method returns (and locals would go out of scope)
  m_PlaybackURL = ArgusTV::GetLiveStreamURL();
  XBMC->Log(LOG_DEBUG, "<-GetLiveStreamURL returns URL(%s)", m_PlaybackURL.c_str());
  return m_PlaybackURL.c_str();
}

void cPVRClientArgusTV::PauseStream(bool bPaused)
{
  NOTUSED(bPaused);
  //TODO: add m_tsreader->Pause() her when adding RTSP streaming support
}

bool cPVRClientArgusTV::CanPauseAndSeek()
{
  if (m_tsreader)
    return true;
  return false;
}
