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

#include <ctime>
#include <stdio.h>
#include <stdlib.h>

#include "platform/os.h"
#include "platform/util/timeutils.h"

#include "client.h"
#include "pvrclient-nextpvr.h"
#include "DialogRecordPref.h"

#include "md5.h"

#include "tinyxml/tinyxml.h"

#if defined(TARGET_WINDOWS)
  #define atoll(S) _atoi64(S) 
#endif 


using namespace std;
using namespace ADDON;

/* Globals */
int g_iNextPVRXBMCBuild = 0;

/* PVR client version (don't forget to update also the addon.xml and the Changelog.txt files) */
#define PVRCLIENT_NEXTPVR_VERSION_STRING    "1.0.0.0"
#define NEXTPVRC_MIN_VERSION_STRING         "3.1.1"

#define HTTP_OK 200
#define HTTP_NOTFOUND 404


const char SAFE[256] =
{
    /*      0 1 2 3  4 5 6 7  8 9 A B  C D E F */
    /* 0 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* 1 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* 2 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* 3 */ 1,1,1,1, 1,1,1,1, 1,1,0,0, 0,0,0,0,
    
    /* 4 */ 0,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,
    /* 5 */ 1,1,1,1, 1,1,1,1, 1,1,1,0, 0,0,0,0,
    /* 6 */ 0,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,
    /* 7 */ 1,1,1,1, 1,1,1,1, 1,1,1,0, 0,0,0,0,
    
    /* 8 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* 9 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* A */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* B */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    
    /* C */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* D */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* E */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* F */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0
};

std::string UriEncode(const std::string sSrc)
{
  const char DEC2HEX[16 + 1] = "0123456789ABCDEF";
  const unsigned char * pSrc = (const unsigned char *)sSrc.c_str();
  const int SRC_LEN = sSrc.length();
  unsigned char * const pStart = new unsigned char[SRC_LEN * 3];
  unsigned char * pEnd = pStart;
  const unsigned char * const SRC_END = pSrc + SRC_LEN;

  for (; pSrc < SRC_END; ++pSrc)
  {
    if (SAFE[*pSrc]) 
    {
      *pEnd++ = *pSrc;
    }
    else
    {
      // escape this char
      *pEnd++ = '%';
      *pEnd++ = DEC2HEX[*pSrc >> 4];
      *pEnd++ = DEC2HEX[*pSrc & 0x0F];
    }
  }

  std::string sResult((char *)pStart, (char *)pEnd);
  delete [] pStart;
  return sResult;
}



/************************************************************/
/** Class interface */

cPVRClientNextPVR::cPVRClientNextPVR()
{
  m_iCurrentChannel        = -1;
  m_tcpclient              = new NextPVR::Socket(NextPVR::af_inet, NextPVR::pf_inet, NextPVR::sock_stream, NextPVR::tcp);
  m_streamingclient        = new NextPVR::Socket(NextPVR::af_inet, NextPVR::pf_inet, NextPVR::sock_stream, NextPVR::tcp);
  m_bConnected             = false;
  m_iChannelCount          = 0;
  m_currentRecordingLength = 0;

  m_supportsLiveTimeshift  = false;
  m_currentLiveLength      = 0;
  m_currentLivePosition    = 0;

  m_pLiveShiftSource       = NULL;

  m_incomingStreamBuffer.Create(188*2000);
}

cPVRClientNextPVR::~cPVRClientNextPVR()
{
  XBMC->Log(LOG_DEBUG, "->~cPVRClientNextPVR()");
  if (m_bConnected)
    Disconnect();
  SAFE_DELETE(m_tcpclient);  
}



bool cPVRClientNextPVR::Connect()
{
  string result;

  // initiate session
  CStdString response;
  if (DoRequest("/service?method=session.initiate&ver=1.0&device=xbmc", response) == HTTP_OK)
  {
    TiXmlDocument doc;
    if (doc.Parse(response) != NULL)
    {
      TiXmlElement* saltNode = doc.RootElement()->FirstChildElement("salt");
      TiXmlElement* sidNode = doc.RootElement()->FirstChildElement("sid");

      if (saltNode != NULL && sidNode != NULL)
      {
        // extract and store sid
        PVR_STRCPY(m_sid, sidNode->FirstChild()->Value());

        // extract salt
        char salt[64];
        PVR_STRCPY(salt, saltNode->FirstChild()->Value());

        // a bit of debug
        XBMC->Log(LOG_DEBUG, "session.initiate returns: sid=%s salt=%s", m_sid, salt);
        

        CStdString pinMD5 = PVRXBMC::XBMC_MD5::GetMD5(g_szPin);
        pinMD5.ToLower();

        // calculate combined MD5
        CStdString combinedMD5;
        combinedMD5.append(":");
        combinedMD5.append(pinMD5);
        combinedMD5.append(":");
        combinedMD5.append(salt);

        // get digest
        CStdString md5 = PVRXBMC::XBMC_MD5::GetMD5(combinedMD5);

        // login session
        CStdString loginResponse;
        char request[512];
        sprintf(request, "/service?method=session.login&sid=%s&md5=%s", m_sid, md5.c_str());
        if (DoRequest(request, loginResponse) == HTTP_OK)
        {
          if (strstr(loginResponse, "<rsp stat=\"ok\">"))
          {
            // check server version
            CStdString settings;
            if (DoRequest("/service?method=setting.list", settings) == HTTP_OK)
            {
              // if it's a NextPVR server, check the verions. WinTV Extend servers work a slightly different way.
              TiXmlDocument settingsDoc;
              if (settingsDoc.Parse(settings) != NULL)
              {
                TiXmlElement* versionNode = settingsDoc.RootElement()->FirstChildElement("NextPVRVersion");
                if (versionNode == NULL)
                {
                  // WinTV Extend server
                }
                else 
                {
                  // NextPVR server
                  int version = atoi(versionNode->FirstChild()->Value());
                  XBMC->Log(LOG_DEBUG, "NextPVR version: %d", version);

                  // is the server new enough
                  if (version < 20508)
                  {
                    XBMC->Log(LOG_ERROR, "Your NextPVR version '%d' is too old. Please upgrade to '%s' or higher!", version, NEXTPVRC_MIN_VERSION_STRING);
                    XBMC->QueueNotification(QUEUE_ERROR, XBMC->GetLocalizedString(30050));
                    XBMC->QueueNotification(QUEUE_ERROR, XBMC->GetLocalizedString(30051), NEXTPVRC_MIN_VERSION_STRING);
                    return false;
                  }
                }
                TiXmlElement* liveTimeshiftNode = settingsDoc.RootElement()->FirstChildElement("LiveTimeshift");
                if (liveTimeshiftNode != NULL)
                {
                  m_supportsLiveTimeshift = true;
                }

                // load padding defaults
                m_iDefaultPrePadding = 1;
                m_iDefaultPostPadding = 2;
                if ( settingsDoc.RootElement()->FirstChildElement("PrePadding") != NULL &&  settingsDoc.RootElement()->FirstChildElement("PrePadding")->FirstChild() != NULL)
                {
                  m_iDefaultPrePadding = atoi(settingsDoc.RootElement()->FirstChildElement("PrePadding")->FirstChild()->Value());
                  m_iDefaultPostPadding = atoi( settingsDoc.RootElement()->FirstChildElement("PostPadding")->FirstChild()->Value());
                }

                m_recordingDirectories = "";
                if ( settingsDoc.RootElement()->FirstChildElement("RecordingDirectories") != NULL &&  settingsDoc.RootElement()->FirstChildElement("RecordingDirectories")->FirstChild() != NULL)
                {
                  m_recordingDirectories += settingsDoc.RootElement()->FirstChildElement("RecordingDirectories")->FirstChild()->Value();
                }
              }
            }

            m_bConnected = true;
            XBMC->Log(LOG_DEBUG, "session.login successful");
            return true;
          }
          else
          {
            XBMC->Log(LOG_DEBUG, "session.login failed");
            XBMC->QueueNotification(QUEUE_ERROR, XBMC->GetLocalizedString(30052));
            m_bConnected = false;
          }
        }
      }
    }  
  }

  return false;
}

void cPVRClientNextPVR::Disconnect()
{
  string result;

  m_bConnected = false;
}

/* IsUp()
 * \brief   Check if we have a valid session to nextpvr
 * \return  True when a session is active
 */
bool cPVRClientNextPVR::IsUp()
{
  return m_bConnected;
}





/************************************************************/
/** General handling */

// Used among others for the server name string in the "Recordings" view
const char* cPVRClientNextPVR::GetBackendName(void)
{
  if (!m_tcpclient->is_valid())
  {
    return g_szHostname.c_str();
  }

  XBMC->Log(LOG_DEBUG, "->GetBackendName()");
  
  if (m_BackendName.length() == 0)
  {
    m_BackendName = "NextPVR  (";
    m_BackendName += g_szHostname.c_str();
    m_BackendName += ")";
  }

  return m_BackendName.c_str();
}

const char* cPVRClientNextPVR::GetBackendVersion(void)
{
  if (!IsUp())
    return "0.0";

  return "1.0";
}

const char* cPVRClientNextPVR::GetConnectionString(void)
{
  static CStdString strConnectionString = "connected";
  return strConnectionString.c_str();
}

PVR_ERROR cPVRClientNextPVR::GetDriveSpace(long long *iTotal, long long *iUsed)
{

  string result;
  vector<string> fields;

  *iTotal = 0;
  *iUsed = 0;

  if (!IsUp())
    return PVR_ERROR_SERVER_ERROR;

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR cPVRClientNextPVR::GetBackendTime(time_t *localTime, int *gmtOffset)
{
  if (!IsUp())
    return PVR_ERROR_SERVER_ERROR;

  return PVR_ERROR_NO_ERROR;
}

/************************************************************/
/** EPG handling */

PVR_ERROR cPVRClientNextPVR::GetEpg(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd)
{
  EPG_TAG broadcast;

  CStdString response;
  char request[512];
  sprintf(request, "/service?method=channel.listings&channel_id=%d&start=%d&end=%d", channel.iUniqueId, (int)iStart, (int)iEnd);
  if (DoRequest(request, response) == HTTP_OK)
  {
    TiXmlDocument doc;
    if (doc.Parse(response) != NULL)
    {
      TiXmlElement* listingsNode = doc.RootElement()->FirstChildElement("listings");
      TiXmlElement* pListingNode = listingsNode->FirstChildElement("l");
      for( pListingNode; pListingNode; pListingNode=pListingNode->NextSiblingElement())
      {
        memset(&broadcast, 0, sizeof(EPG_TAG));

        char title[128];
        char description[1024];

        strncpy(title, pListingNode->FirstChildElement("name")->FirstChild()->Value(), sizeof title);
        if (pListingNode->FirstChildElement("description") != NULL && pListingNode->FirstChildElement("description")->FirstChild() != NULL)
        {
          PVR_STRCPY(description, pListingNode->FirstChildElement("description")->FirstChild()->Value());
        }
        else
        {
          description[0] = '\0';
        }

        char start[32];
        strncpy(start, pListingNode->FirstChildElement("start")->FirstChild()->Value(), sizeof start);
        start[10] = '\0';

        char end[32];
        strncpy(end, pListingNode->FirstChildElement("end")->FirstChild()->Value(), sizeof end);
        end[10] = '\0';

        broadcast.iUniqueBroadcastId  = atoi(pListingNode->FirstChildElement("id")->FirstChild()->Value());
        broadcast.strTitle            = title;
        broadcast.iChannelNumber      = channel.iChannelNumber;
        broadcast.startTime           = atol(start);
        broadcast.endTime             = atol(end);        
        broadcast.strPlot             = description;
        broadcast.strIconPath         = "";

        char genre[128];
        genre[0] = '\0';
        if (pListingNode->FirstChildElement("genre") != NULL && pListingNode->FirstChildElement("genre")->FirstChild() != NULL)
        {
          broadcast.iGenreType          = EPG_GENRE_USE_STRING;
          PVR_STRCPY(genre, pListingNode->FirstChildElement("genre")->FirstChild()->Value());
          broadcast.strGenreDescription = genre;
        }
        else
        {
          // genre type
          if (pListingNode->FirstChildElement("genre_type") != NULL && pListingNode->FirstChildElement("genre_type")->FirstChild() != NULL)
          {
            broadcast.iGenreType  = atoi(pListingNode->FirstChildElement("genre_type")->FirstChild()->Value());
          }

          // genre subtype
          if (pListingNode->FirstChildElement("genre_subtype") != NULL && pListingNode->FirstChildElement("genre_subtype")->FirstChild() != NULL)
          {
            broadcast.iGenreSubType  = atoi(pListingNode->FirstChildElement("genre_subtype")->FirstChild()->Value());
          }
        }

        broadcast.bNotify             = false;

        PVR->TransferEpgEntry(handle, &broadcast);
      }
    }
  }

  return PVR_ERROR_NO_ERROR;
}


/************************************************************/
/** Channel handling */

int cPVRClientNextPVR::GetNumChannels(void)
{
  if (m_iChannelCount != 0)
    return m_iChannelCount;


  // need something more optimal, but this will do for now...
  m_iChannelCount = 0;
  CStdString response;
  if (DoRequest("/service?method=channel.list", response) == HTTP_OK)
  {
    TiXmlDocument doc;
    if (doc.Parse(response) != NULL)
    {
      TiXmlElement* channelsNode = doc.RootElement()->FirstChildElement("channels");
      TiXmlElement* pChannelNode = channelsNode->FirstChildElement("channel");
      for( pChannelNode; pChannelNode; pChannelNode=pChannelNode->NextSiblingElement())
      {
        m_iChannelCount++;
      }
    }
  }

  return m_iChannelCount;
}

CStdString cPVRClientNextPVR::GetChannelIcon(int channelID)
{
  char filename[64];
  snprintf(filename, sizeof(filename), "nextpvr-ch%d.png", channelID);

  CStdString iconFilename("special://userdata/addon_data/pvr.nextpvr/");
  iconFilename += filename;

  // do we already have the icon file?
  if (XBMC->FileExists(iconFilename, false))
  {        
    return iconFilename;    
  } 

  if (m_tcpclient->create())
  {
    if (m_tcpclient->connect(g_szHostname, g_iPort))
    {
      char line[256];
      sprintf(line, "GET /service?method=channel.icon&channel_id=%d HTTP/1.0\r\n", channelID);
      m_tcpclient->send(line, strlen(line));

      sprintf(line, "Connection: close\r\n");
      m_tcpclient->send(line, strlen(line));

      sprintf(line, "\r\n");
      m_tcpclient->send(line, strlen(line));

      char buf[1024];
      int read = m_tcpclient->receive(buf, sizeof buf, 0);
      if (read > 0)
      {
        void* fileHandle = XBMC->OpenFileForWrite(iconFilename, true);
        if (fileHandle != NULL)
        {
          int written = 0;
          // HTTP response header
          for (int i=0; i<read; i++)
          {
            if (buf[i] == '\r' && buf[i+1] == '\n' && buf[i+2] == '\r' && buf[i+3] == '\n')
            {
              XBMC->WriteFile(fileHandle, (unsigned char *)&buf[i+4], (read - (i + 4)));
            }
          }

          // rest of body
          bool connected = true;
          while (connected)
          {
            char buf[1024];
            int read = m_tcpclient->receive(buf, sizeof buf, 0);

            if (read == 0)
            {
              connected = false;
            }
            else if (read > 0)
            {
              XBMC->WriteFile(fileHandle, (unsigned char *)buf, read);
            }
            else if (read < 0 && written > 0)
            {
              connected = false;
            }
          }

          // close file
          XBMC->CloseFile(fileHandle);
        }
      }
    }
    m_tcpclient->close();
    return iconFilename;
  }


  return "";
}

PVR_ERROR cPVRClientNextPVR::GetChannels(ADDON_HANDLE handle, bool bRadio)
{
  PVR_CHANNEL     tag;
  CStdString      stream;  
  
  m_iChannelCount = 0;

  CStdString response;
  if (DoRequest("/service?method=channel.list", response) == HTTP_OK)
  {
    TiXmlDocument doc;
    if (doc.Parse(response) != NULL)
    {
      TiXmlElement* channelsNode = doc.RootElement()->FirstChildElement("channels");
      TiXmlElement* pChannelNode = channelsNode->FirstChildElement("channel");
      for( pChannelNode; pChannelNode; pChannelNode=pChannelNode->NextSiblingElement())
      {
        memset(&tag, 0, sizeof(PVR_CHANNEL));
        tag.iUniqueId = atoi(pChannelNode->FirstChildElement("id")->FirstChild()->Value());
        tag.iChannelNumber = atoi(pChannelNode->FirstChildElement("number")->FirstChild()->Value());

        PVR_STRCPY(tag.strChannelName, pChannelNode->FirstChildElement("name")->FirstChild()->Value());

        // check if we need to download a channel icon
        if (pChannelNode->FirstChildElement("icon"))
        {
          CStdString iconFile = GetChannelIcon(tag.iUniqueId);
          if (iconFile.length() > 0)
          {
            PVR_STRCPY(tag.strIconPath, iconFile);
          }
        }

        PVR_STRCPY(tag.strInputFormat, "video/x-mpegts");

        // check if it's a radio channel
        tag.bIsRadio = false;
        TiXmlElement* channelTypeNode = pChannelNode->FirstChildElement("type");
        if (strcmp(channelTypeNode->FirstChild()->Value(), "0xa") == 0)
        {
          tag.bIsRadio = true;
        }

        // transfer channel to XBMC
        if ((bRadio && tag.bIsRadio) || (!bRadio && !tag.bIsRadio))
          PVR->TransferChannelEntry(handle, &tag);
        
        m_iChannelCount ++;
      }
    }
  }
  return PVR_ERROR_NO_ERROR;
}

/************************************************************/
/** Channel group handling **/

int cPVRClientNextPVR::GetChannelGroupsAmount(void)
{
  // Not directly possible at the moment
  XBMC->Log(LOG_DEBUG, "GetChannelGroupsAmount");

  int groups = 0;

  CStdString response;
  if (DoRequest("/service?method=channel.groups", response) == HTTP_OK)
  {
    TiXmlDocument doc;
    if (doc.Parse(response) != NULL)
    {
      TiXmlElement* groupsNode = doc.RootElement()->FirstChildElement("groups");
      TiXmlElement* pGroupNode = groupsNode->FirstChildElement("group");
      for( pGroupNode; pGroupNode; pGroupNode=pGroupNode->NextSiblingElement())
      {
        groups++;
      }
    }
  }

  return groups;
}

PVR_ERROR cPVRClientNextPVR::GetChannelGroups(ADDON_HANDLE handle, bool bRadio)
{
  PVR_CHANNEL_GROUP tag;

  // nextpvr doesn't have a separate concept of radio channel groups
  if (bRadio)
    return PVR_ERROR_NO_ERROR;

  // for tv, use the groups returned by nextpvr
  CStdString response;
  if (DoRequest("/service?method=channel.groups", response) == HTTP_OK)
  {
    TiXmlDocument doc;
    if (doc.Parse(response) != NULL)
    {
      TiXmlElement* groupsNode = doc.RootElement()->FirstChildElement("groups");
      TiXmlElement* pGroupNode = groupsNode->FirstChildElement("group");
      for( pGroupNode; pGroupNode; pGroupNode=pGroupNode->NextSiblingElement())
      {
        memset(&tag, 0, sizeof(PVR_CHANNEL_GROUP));
        tag.bIsRadio = false;
        strncpy(tag.strGroupName, pGroupNode->FirstChildElement("name")->FirstChild()->Value(), sizeof tag.strGroupName);

        // tell XBMC about channel, ignoring "All Channels" since xbmc has an built in group with effectively the same function
        if (strcmp(tag.strGroupName, "All Channels") != 0)
        {
          PVR->TransferChannelGroup(handle, &tag);
        }
      }
    }
  }
  return PVR_ERROR_NO_ERROR;
}



PVR_ERROR cPVRClientNextPVR::GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group)
{
  std::string encodedGroupName = UriEncode(group.strGroupName);

  char request[512];
  sprintf(request, "/service?method=channel.list&group_id=%s", encodedGroupName.c_str());

  CStdString response;
  if (DoRequest(request, response) == HTTP_OK)
  {
    PVR_CHANNEL_GROUP_MEMBER tag;

    TiXmlDocument doc;
    if (doc.Parse(response) != NULL)
    {
      TiXmlElement* channelsNode = doc.RootElement()->FirstChildElement("channels");
      TiXmlElement* pChannelNode = channelsNode->FirstChildElement("channel");
      for( pChannelNode; pChannelNode; pChannelNode=pChannelNode->NextSiblingElement())
      {
        memset(&tag, 0, sizeof(PVR_CHANNEL_GROUP_MEMBER));
        strncpy(tag.strGroupName, group.strGroupName, sizeof(tag.strGroupName));
        tag.iChannelUniqueId = atoi(pChannelNode->FirstChildElement("id")->FirstChild()->Value());
        tag.iChannelNumber = atoi(pChannelNode->FirstChildElement("number")->FirstChild()->Value());

        PVR->TransferChannelGroupMember(handle, &tag);
      }
    }
  }

  return PVR_ERROR_NO_ERROR;
}

/************************************************************/
/** Record handling **/

int cPVRClientNextPVR::GetNumRecordings(void)
{
  // need something more optimal, but this will do for now...
  int recordingCount = 0;

  CStdString response;
  if (DoRequest("/service?method=recording.list&filter=ready", response) == HTTP_OK)
  {
    TiXmlDocument doc;
    if (doc.Parse(response) != NULL)
    {
      TiXmlElement* recordingsNode = doc.RootElement()->FirstChildElement("recordings");
      if (recordingsNode != NULL)
      {
        TiXmlElement* pRecordingNode = recordingsNode->FirstChildElement("recording");
        for( pRecordingNode; pRecordingNode; pRecordingNode=pRecordingNode->NextSiblingElement())
        {
          recordingCount++;
        }
      }
    }
  }
  
  return recordingCount;
}

PVR_ERROR cPVRClientNextPVR::GetRecordings(ADDON_HANDLE handle)
{
  // include already-completed recordings
  CStdString response;
  if (DoRequest("/service?method=recording.list&filter=ready", response) == HTTP_OK)
  {
    TiXmlDocument doc;
    if (doc.Parse(response) != NULL)
    {
      PVR_RECORDING   tag;

      TiXmlElement* recordingsNode = doc.RootElement()->FirstChildElement("recordings");
      TiXmlElement* pRecordingNode = recordingsNode->FirstChildElement("recording");
      for( pRecordingNode; pRecordingNode; pRecordingNode=pRecordingNode->NextSiblingElement())
      {
        memset(&tag, 0, sizeof(PVR_RECORDING));
        
        PVR_STRCPY(tag.strRecordingId, pRecordingNode->FirstChildElement("id")->FirstChild()->Value());
        PVR_STRCPY(tag.strTitle, pRecordingNode->FirstChildElement("name")->FirstChild()->Value());
        PVR_STRCPY(tag.strDirectory, pRecordingNode->FirstChildElement("name")->FirstChild()->Value());

        if (pRecordingNode->FirstChildElement("desc") != NULL && pRecordingNode->FirstChildElement("desc")->FirstChild() != NULL)
        {
          PVR_STRCPY(tag.strPlot, pRecordingNode->FirstChildElement("desc")->FirstChild()->Value());
        }

        if (pRecordingNode->FirstChildElement("subtitle") != NULL && pRecordingNode->FirstChildElement("subtitle")->FirstChild() != NULL)
        {
          PVR_STRCPY(tag.strTitle, pRecordingNode->FirstChildElement("subtitle")->FirstChild()->Value());
        }


        tag.recordingTime = atoi(pRecordingNode->FirstChildElement("start_time_ticks")->FirstChild()->Value());
        tag.iDuration = atoi(pRecordingNode->FirstChildElement("duration_seconds")->FirstChild()->Value());

        if (pRecordingNode->FirstChildElement("playback_position") != NULL && pRecordingNode->FirstChildElement("playback_position")->FirstChild() != NULL)
        {
          tag.iLastPlayedPosition = atoi(pRecordingNode->FirstChildElement("playback_position")->FirstChild()->Value());
        }

        CStdString strStream;
        strStream.Format("http://%s:%d/live?recording=%s", g_szHostname, g_iPort, tag.strRecordingId);
        strncpy(tag.strStreamURL, strStream.c_str(), sizeof(tag.strStreamURL)); 


        PVR->TransferRecordingEntry(handle, &tag);
      }
    }
  }

  // ...and any in-progress recordings
  if (DoRequest("/service?method=recording.list&filter=pending", response) == HTTP_OK)
  {
    TiXmlDocument doc;
    if (doc.Parse(response) != NULL)
    {
      PVR_RECORDING   tag;

      TiXmlElement* recordingsNode = doc.RootElement()->FirstChildElement("recordings");
      TiXmlElement* pRecordingNode = recordingsNode->FirstChildElement("recording");
      for( pRecordingNode; pRecordingNode; pRecordingNode=pRecordingNode->NextSiblingElement())
      {
        memset(&tag, 0, sizeof(PVR_RECORDING));
        
        PVR_STRCPY(tag.strRecordingId, pRecordingNode->FirstChildElement("id")->FirstChild()->Value());
        PVR_STRCPY(tag.strTitle, pRecordingNode->FirstChildElement("name")->FirstChild()->Value());
        PVR_STRCPY(tag.strDirectory, pRecordingNode->FirstChildElement("name")->FirstChild()->Value());

        if (pRecordingNode->FirstChildElement("desc") != NULL && pRecordingNode->FirstChildElement("desc")->FirstChild() != NULL)
        {
          PVR_STRCPY(tag.strPlot, pRecordingNode->FirstChildElement("desc")->FirstChild()->Value());
        }

        tag.recordingTime = atoi(pRecordingNode->FirstChildElement("start_time_ticks")->FirstChild()->Value());
        tag.iDuration = atoi(pRecordingNode->FirstChildElement("duration_seconds")->FirstChild()->Value());

        CStdString strStream;
        strStream.Format("http://%s:%d/live?recording=%s", g_szHostname, g_iPort, tag.strRecordingId);
        strncpy(tag.strStreamURL, strStream.c_str(), sizeof(tag.strStreamURL)); 

        if (tag.recordingTime <= time(NULL) && (tag.recordingTime + tag.iDuration) >= time(NULL))
        {
          PVR->TransferRecordingEntry(handle, &tag);
        }
      }
    }
  }
  
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR cPVRClientNextPVR::DeleteRecording(const PVR_RECORDING &recording)
{
  XBMC->Log(LOG_DEBUG, "DeleteRecording");
  char request[512];
  sprintf(request, "/service?method=recording.delete&recording_id=%s", recording.strRecordingId);

  CStdString response;
  if (DoRequest(request, response) == HTTP_OK)
  {
    if (strstr(response, "<rsp stat=\"ok\">"))
    {
      PVR->TriggerRecordingUpdate();
      XBMC->Log(LOG_DEBUG, "DeleteRecording failed. Returning PVR_ERROR_NO_ERROR");
      return PVR_ERROR_NO_ERROR;
    }
    else
    {
      XBMC->Log(LOG_DEBUG, "DeleteRecording failed");
    }
  }


  XBMC->Log(LOG_DEBUG, "DeleteRecording failed. Returning PVR_ERROR_FAILED");
  return PVR_ERROR_FAILED;
}

PVR_ERROR cPVRClientNextPVR::RenameRecording(const PVR_RECORDING &recording)
{
  if (!IsUp())
    return PVR_ERROR_SERVER_ERROR;

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR cPVRClientNextPVR::SetRecordingLastPlayedPosition(const PVR_RECORDING &recording, int lastplayedposition)
{
  XBMC->Log(LOG_DEBUG, "SetRecordingLastPlayedPosition");
  char request[512];
  sprintf(request, "/service?method=recording.watched.set&recording_id=%s&position=%d", recording.strRecordingId, lastplayedposition);

  CStdString response;
  if (DoRequest(request, response) == HTTP_OK)
  {
    if (strstr(response, "<rsp stat=\"ok\">") == NULL)
    {
      XBMC->Log(LOG_DEBUG, "SetRecordingLastPlayedPosition failed");
      return PVR_ERROR_FAILED;
    }
  }
  return PVR_ERROR_NO_ERROR;
}

int cPVRClientNextPVR::GetRecordingLastPlayedPosition(const PVR_RECORDING &recording)
{
  return recording.iLastPlayedPosition;
}

PVR_ERROR cPVRClientNextPVR::GetRecordingEdl(const PVR_RECORDING& recording, PVR_EDL_ENTRY entries[], int *size)
{
  XBMC->Log(LOG_DEBUG, "GetRecordingEdl");
  char request[512];
  sprintf(request, "/service?method=recording.edl&recording_id=%s", recording.strRecordingId);

  CStdString response;
  if (DoRequest(request, response) == HTTP_OK)
  {
    if (strstr(response, "<rsp stat=\"ok\">") != NULL)
    {
      TiXmlDocument doc;
      if (doc.Parse(response) != NULL)
      {
        int index = 0;
        TiXmlElement* commercialsNode = doc.RootElement()->FirstChildElement("commercials");
        TiXmlElement* pCommercialNode = commercialsNode->FirstChildElement("commercial");
        for( pCommercialNode; pCommercialNode; pCommercialNode=pCommercialNode->NextSiblingElement())
        {          
          PVR_EDL_ENTRY entry;
          entry.start = atoi(pCommercialNode->FirstChildElement("start")->FirstChild()->Value()) * 1000;
          entry.end = atoi(pCommercialNode->FirstChildElement("end")->FirstChild()->Value()) * 1000 ;
          entry.type = PVR_EDL_TYPE_COMBREAK;
          entries[index] = entry;
          index++;
        }
        *size = index;
        return PVR_ERROR_NO_ERROR;
      }
    }
  }
  return PVR_ERROR_FAILED;
}

/************************************************************/
/** Timer handling */

int cPVRClientNextPVR::GetNumTimers(void)
{
  int timerCount = 0;
  CStdString response;

  // get list of recurring recordings
  if (DoRequest("/service?method=recording.recurring.list", response) == HTTP_OK)
  {
    TiXmlDocument doc;
    if (doc.Parse(response) != NULL)
    {
      TiXmlElement* recordingsNode = doc.RootElement()->FirstChildElement("recurrings");
      if (recordingsNode != NULL)
      {
        TiXmlElement* pRecordingNode = recordingsNode->FirstChildElement("recurring");
        for( pRecordingNode; pRecordingNode; pRecordingNode=pRecordingNode->NextSiblingElement())
        {
          timerCount++;
        }
      }
    }
  }


  // get list of pending recordings
  response = "";
  if (DoRequest("/service?method=recording.list&filter=pending", response) == HTTP_OK)
  {
    TiXmlDocument doc;
    if (doc.Parse(response) != NULL)
    {
      TiXmlElement* recordingsNode = doc.RootElement()->FirstChildElement("recordings");
      if (recordingsNode != NULL)
      {
        TiXmlElement* pRecordingNode = recordingsNode->FirstChildElement("recording");
        for( pRecordingNode; pRecordingNode; pRecordingNode=pRecordingNode->NextSiblingElement())
        {
          timerCount++;
        }
      }
    }
  }
  
  return timerCount;
}

PVR_ERROR cPVRClientNextPVR::GetTimers(ADDON_HANDLE handle)
{
  CStdString response;

  // first add the recurring recordings
  if (DoRequest("/service?method=recording.recurring.list&filter=pending", response) == HTTP_OK)
  {
    TiXmlDocument doc;
    if (doc.Parse(response) != NULL)
    {
      PVR_TIMER tag;
      TiXmlElement* recurringsNode = doc.RootElement()->FirstChildElement("recurrings");
      TiXmlElement* pRecurringNode = recurringsNode->FirstChildElement("recurring");
      for( pRecurringNode; pRecurringNode; pRecurringNode=pRecurringNode->NextSiblingElement())
      {
        memset(&tag, 0, sizeof(tag));
        tag.iClientIndex = 0xF000000 + atoi(pRecurringNode->FirstChildElement("id")->FirstChild()->Value());

        tag.iClientChannelUid = 8101;

        char strTitle[PVR_ADDON_NAME_STRING_LENGTH];
        strncpy(strTitle, pRecurringNode->FirstChildElement("name")->FirstChild()->Value(), sizeof(strTitle)-1);
        strncat(tag.strTitle, XBMC->GetLocalizedString(30054), sizeof(tag.strTitle) - 1);
        strncat(tag.strTitle, " ", sizeof(tag.strTitle) - 1);
        strncat(tag.strTitle, strTitle, sizeof(tag.strTitle) - 1);

        //PVR_STRCPY(tag.strTitle, pRecurringNode->FirstChildElement("name")->FirstChild()->Value());
        //tag.iClientChannelUid = atoi(pRecurringNode->FirstChildElement("channel_id")->FirstChild()->Value());
        tag.state = PVR_TIMER_STATE_SCHEDULED;

        tag.startTime = time(NULL) - 96000;
        tag.endTime = time(NULL) - 86000;
        
        PVR_STRCPY(tag.strSummary, "summary");

        tag.bIsRepeating = true;

        // pass timer to xbmc
        PVR->TransferTimerEntry(handle, &tag);
      }
    }
  }

  // next add the one-off recordings.
  response = "";
  if (DoRequest("/service?method=recording.list&filter=pending", response) == HTTP_OK)
  {
    TiXmlDocument doc;
    if (doc.Parse(response) != NULL)
    {
      PVR_TIMER tag;

      TiXmlElement* recordingsNode = doc.RootElement()->FirstChildElement("recordings");
      TiXmlElement* pRecordingNode = recordingsNode->FirstChildElement("recording");
      for( pRecordingNode; pRecordingNode; pRecordingNode=pRecordingNode->NextSiblingElement())
      {
        memset(&tag, 0, sizeof(tag));


        tag.iClientIndex = atoi(pRecordingNode->FirstChildElement("id")->FirstChild()->Value());
        tag.iClientChannelUid = atoi(pRecordingNode->FirstChildElement("channel_id")->FirstChild()->Value());

        // name
        PVR_STRCPY(tag.strTitle, pRecordingNode->FirstChildElement("name")->FirstChild()->Value());

        // description
        if (pRecordingNode->FirstChildElement("desc") != NULL && pRecordingNode->FirstChildElement("desc")->FirstChild() != NULL)
        {
          PVR_STRCPY(tag.strSummary, pRecordingNode->FirstChildElement("desc")->FirstChild()->Value());
        }

        tag.state = PVR_TIMER_STATE_SCHEDULED;
        if (pRecordingNode->FirstChildElement("status") != NULL && pRecordingNode->FirstChildElement("status")->FirstChild() != NULL)
        {
          char status[32];
          PVR_STRCPY(status, pRecordingNode->FirstChildElement("status")->FirstChild()->Value());
          if (strcmp(status, "Recording") == 0)
          {
            tag.state = PVR_TIMER_STATE_RECORDING;
          }
        }


        // start/end time
        char start[32];
        strncpy(start, pRecordingNode->FirstChildElement("start_time_ticks")->FirstChild()->Value(), sizeof start);
        start[10] = '\0';
        tag.startTime           = atol(start);
        tag.endTime             = tag.startTime + atoi(pRecordingNode->FirstChildElement("duration_seconds")->FirstChild()->Value());

        // recurring recordings
        if (pRecordingNode->FirstChildElement("recurring") != NULL && pRecordingNode->FirstChildElement("recurring")->FirstChild() != NULL)
        {
          if (strcmp(pRecordingNode->FirstChildElement("recurring")->FirstChild()->Value(), "true") == 0)
          {
            tag.bIsRepeating = true;
          }
        }

        // pass timer to xbmc
        PVR->TransferTimerEntry(handle, &tag);
      }
    }
  }

  return PVR_ERROR_NO_ERROR;  
}

PVR_ERROR cPVRClientNextPVR::GetTimerInfo(unsigned int timernumber, PVR_TIMER &timerinfo)
{
  // not supported 
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR cPVRClientNextPVR::AddTimer(const PVR_TIMER &timerinfo)
{
  // editing recording is not supported
  if (timerinfo.iClientIndex != -1)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }

  std::string encodedName = UriEncode(timerinfo.strTitle);

  // manual recording (iEpgUid == -1) or instant recording (timerinfo.startTime == 0)
  if (timerinfo.startTime == 0 || timerinfo.iEpgUid == -1)
  {
    // build request
    char request[1024]; 
    snprintf(request, sizeof(request), "/service?method=recording.save&name=%s&channel=%d&time_t=%d&duration=%d", 
          encodedName.c_str(),
          timerinfo.iClientChannelUid, 
          (int)timerinfo.startTime, 
          (int)(timerinfo.endTime - timerinfo.startTime));

    // send request to NextPVR
    CStdString response;
    if (DoRequest(request, response) == HTTP_OK)
    {
      if (strstr(response, "<rsp stat=\"ok\">"))
      {
        PVR->TriggerTimerUpdate();
        return PVR_ERROR_NO_ERROR;
      }
    }
  }
  else
  {
    CDialogRecordPref vWindow(timerinfo.strTitle, timerinfo.strSummary, m_iDefaultPrePadding, m_iDefaultPostPadding, m_recordingDirectories);
    if (vWindow.DoModal() == 1) // user hit ok
    {
      // build request string
      char request[1024];
      if (vWindow.RecordingType == 0)
      {
        // build one-off recording request
        snprintf(request, sizeof(request), "/service?method=recording.save&name=%s&channel=%d&time_t=%d&duration=%d&pre_padding=%d&post_padding=%d&directory_id=%s", 
          encodedName.c_str(),
          timerinfo.iClientChannelUid, 
          (int)timerinfo.startTime, 
          (int)(timerinfo.endTime - timerinfo.startTime),
          vWindow.PrePadding,
          vWindow.PostPadding, 
          vWindow.RecordingDirectory.c_str()
          );
      }
      else
      {
        // build recurring recording request
        snprintf(request, sizeof(request), "/service?method=recording.recurring.save&event_id=%d&recurring_type=%d&keep=%d&pre_padding=%d&post_padding=%d&directory_id=%s", 
          timerinfo.iEpgUid,
          vWindow.RecordingType,
          vWindow.Keep,
          vWindow.PrePadding,
          vWindow.PostPadding, 
          vWindow.RecordingDirectory.c_str()
          );
      }

      // send request to NextPVR
      CStdString response;
      if (DoRequest(request, response) == HTTP_OK)
      {
        if (strstr(response, "<rsp stat=\"ok\">"))
        {
          PVR->TriggerTimerUpdate();
          return PVR_ERROR_NO_ERROR;
        }
      }
    }
    else
    {
      // cancel
      return PVR_ERROR_NO_ERROR;
    }
  }

  return PVR_ERROR_FAILED;
}

PVR_ERROR cPVRClientNextPVR::DeleteTimer(const PVR_TIMER &timer, bool bForceDelete)
{
  char request[512];
  sprintf(request, "/service?method=recording.delete&recording_id=%d", timer.iClientIndex);

  // handle special-case for recurring recordings
  if (timer.iClientIndex > 0xF000000)
  {
    sprintf(request, "/service?method=recording.recurring.delete&recurring_id=%d", (timer.iClientIndex - 0xF000000));
  }

  CStdString response;
  if (DoRequest(request, response) == HTTP_OK)
  {
    if (strstr(response, "<rsp stat=\"ok\">"))
    {
      PVR->TriggerTimerUpdate();
      return PVR_ERROR_NO_ERROR;
    }
  }

  return PVR_ERROR_FAILED;
}

PVR_ERROR cPVRClientNextPVR::UpdateTimer(const PVR_TIMER &timerinfo)
{
  // not supported
  return PVR_ERROR_NO_ERROR;
}


/************************************************************/
/** Live stream handling */

bool cPVRClientNextPVR::OpenLiveStream(const PVR_CHANNEL &channelinfo)
{
  m_PlaybackURL = "";

  XBMC->Log(LOG_DEBUG, "OpenLiveStream(%d:%s) (oid=%d)", channelinfo.iChannelNumber, channelinfo.strChannelName, channelinfo.iUniqueId);
  if (strstr(channelinfo.strStreamURL, "live?channel") == NULL)
  {
    if (m_pLiveShiftSource != NULL)
    {
      XBMC->Log(LOG_DEBUG, "OpenLiveStream() informing NextPVR of existing channel stream closing");

      char request[512];
      sprintf(request, "/service?method=channel.stop");
      CStdString response;
      DoRequest(request, response);

      m_pLiveShiftSource->Close();
      delete m_pLiveShiftSource;
      m_pLiveShiftSource = NULL;
    }

    if (!m_streamingclient->create())
    {
      XBMC->Log(LOG_ERROR, "Could not connect create streaming socket");
      return false;
    }

    m_incomingStreamBuffer.Clear();

    if (!m_streamingclient->connect(g_szHostname, g_iPort))
    {
      XBMC->Log(LOG_ERROR, "Could not connect to NextPVR backend for streaming");
      return false;
    }

    if (m_pLiveShiftSource != NULL)
    {
      delete m_pLiveShiftSource;
      m_pLiveShiftSource = NULL;
    }
  
    char line[256];
    sprintf(line, "GET /live?channel=%d&client=XBMC-%s HTTP/1.0\r\n", channelinfo.iChannelNumber, m_sid); 
    if (m_supportsLiveTimeshift && g_bUseTimeshift)
      sprintf(line, "GET /live?channel=%d&mode=liveshift&client=XBMC-%s HTTP/1.0\r\n", channelinfo.iChannelNumber, m_sid); 
    m_streamingclient->send(line, strlen(line));

    sprintf(line, "Connection: close\r\n");
    m_streamingclient->send(line, strlen(line));

    sprintf(line, "\r\n");
    m_streamingclient->send(line, strlen(line));

    m_currentLivePosition = 0;
  
    XBMC->Log(LOG_DEBUG, "OpenLiveStream()@1");

    char buf[1024];
    int read = m_streamingclient->receive(buf, sizeof buf, 0);

    XBMC->Log(LOG_DEBUG, "OpenLiveStream()@2");

    for (int i=0; i<read; i++)
    {
      if (buf[i] == '\r' && buf[i+1] == '\n' && buf[i+2] == '\r' && buf[i+3] == '\n')
      {
        int remainder = read - (i+4);
        if (remainder > 0)
        {
          m_incomingStreamBuffer.WriteData((char *)&buf[i+4], remainder);
        }

        char header[256];
        if (i < sizeof(header))
        {
          memset(header, 0, sizeof(header));
          memcpy(header, buf, i);
          XBMC->Log(LOG_DEBUG, "%s", header);

          if (strstr(header, "HTTP/1.1 404") != NULL)
          {
            XBMC->Log(LOG_DEBUG, "Unable to start channel. 404");
            XBMC->QueueNotification(QUEUE_INFO, "Tuner not available");
            return false;
          }

        }

        // long blocking from now on
        m_streamingclient->set_non_blocking(1);

        snprintf(line, sizeof(line), "http://%s:%d/live?channel=%d&client=XBMC", g_szHostname.c_str(), g_iPort, channelinfo.iChannelNumber);
        m_PlaybackURL = line;


        if (channelinfo.bIsRadio == false && m_supportsLiveTimeshift && g_bUseTimeshift)
        {
          m_streamingclient->set_non_blocking(0); 
          m_pLiveShiftSource = new LiveShiftSource(m_streamingclient);
        }

        XBMC->Log(LOG_DEBUG, "OpenLiveStream()@exit");
        return true;
      }
    }
  }

  XBMC->Log(LOG_DEBUG, "OpenLiveStream()@exit (failed)");
  return false;
}

int cPVRClientNextPVR::ReadLiveStream(unsigned char *pBuffer, unsigned int iBufferSize)
{
  //XBMC->Log(LOG_DEBUG, "ReadLiveStream");

  int read = iBufferSize;

  if (m_supportsLiveTimeshift && m_pLiveShiftSource != NULL)
  {
    int rc = m_pLiveShiftSource->Read(pBuffer, iBufferSize);
    if (rc < 0)
    {
      m_streamingclient->close();
      XBMC->QueueNotification(QUEUE_ERROR, XBMC->GetLocalizedString(30053));
    }

    static int total = 0;
    total += rc;
    //XBMC->Log(LOG_DEBUG, "ReadLiveStream read %d bytes. (total %d)", rc, total);

    return rc;
  }
  else
  {
    bool bufferWasEmpty = (m_incomingStreamBuffer.getMaxReadSize() == 0);

    // do we have enough data to fill this buffer? 
    int read_timeouts = 0;
    bool bufferMore = true;
    unsigned char buf[188*100];
    while (bufferMore)
    {
      if (m_incomingStreamBuffer.getMaxWriteSize() < sizeof(buf))
        bufferMore = false;

      if (bufferMore)
      {
        bool available = m_streamingclient->read_ready();
        int read = m_streamingclient->receive((char *)buf, sizeof buf, 0);
        if (read > 0)
        {
          //XBMC->Log(LOG_DEBUG, "ReadLiveStream() added %d bytes to buffer. (now at %d bytes)", read, m_incomingStreamBuffer.getMaxReadSize());
          // write it to incoming ring buffer
          m_incomingStreamBuffer.WriteData((char *)buf, read);
        }
        else
        {
          if (bufferWasEmpty && m_incomingStreamBuffer.getMaxReadSize() < (188*400))
          {
            usleep(50000);
            read_timeouts++;
          }
          else if (m_incomingStreamBuffer.getMaxReadSize() >= iBufferSize)
          {
            // got enough data to continue
            bufferMore = false;
          }
          else
          {
            usleep(50000);
            read_timeouts++;
          }
        }
      }

      // is it taking too long?
      if (read_timeouts > 100)
      {
        m_streamingclient->close();
        bufferMore = false;
        XBMC->QueueNotification(QUEUE_ERROR, XBMC->GetLocalizedString(30053));
        return -1;
      }
    }

    // read from buffer to return for XBMC
    if (m_incomingStreamBuffer.getMaxReadSize() < read)
      read = m_incomingStreamBuffer.getMaxReadSize();
    m_incomingStreamBuffer.ReadData((char *)pBuffer, read);

    m_currentLivePosition += read;
  }

  //XBMC->Log(LOG_DEBUG, "ReadLiveStream return %d bytes (%d bytes remaining in buffer)", read, m_incomingStreamBuffer.getMaxReadSize());

  return read;
}

void cPVRClientNextPVR::CloseLiveStream(void)
{
  XBMC->Log(LOG_DEBUG, "CloseLiveStream");

  if (m_pLiveShiftSource)
  {
    XBMC->Log(LOG_DEBUG, "Telling backend of live session closure");

    char request[512];
    sprintf(request, "/service?method=channel.stop");
    CStdString response;
    DoRequest(request, response);
    
    m_pLiveShiftSource->Close();
    delete m_pLiveShiftSource;
    m_pLiveShiftSource = NULL;
  }

  // Socket no longer required. Server will clean up when socket is closed.
  m_streamingclient->close(); 
  XBMC->Log(LOG_DEBUG, "CloseLiveStream@exit");
}


long long cPVRClientNextPVR::SeekLiveStream(long long iPosition, int iWhence)
{
  PLATFORM::CLockObject lock(m_mutex);

  if (m_pLiveShiftSource != NULL)
  {
    m_pLiveShiftSource->Seek(iPosition);
    return iPosition;
  }

  // not supported
  return -1;
}


long long cPVRClientNextPVR::LengthLiveStream(void)
{
  if (m_pLiveShiftSource != NULL)
  {
    return m_pLiveShiftSource->GetLength();
  }

  // not supported
  return -1;
}

long long cPVRClientNextPVR::PositionLiveStream(void)
{
  if (m_pLiveShiftSource != NULL)
  {
    return m_pLiveShiftSource->GetPosition();
  }

  return m_currentLivePosition;
}

bool cPVRClientNextPVR::SwitchChannel(const PVR_CHANNEL &channel)
{
  XBMC->Log(LOG_DEBUG, "SwitchChannel(%d:%s)", channel.iChannelNumber, channel.strChannelName);

  // if we're already on the correct channel, then dont do anything
  if (((int)channel.iUniqueId) == m_iCurrentChannel)
    return true;

  // open new stream
  bool result = OpenLiveStream(channel);  
  return result;
}


int cPVRClientNextPVR::GetCurrentClientChannel()
{
  XBMC->Log(LOG_DEBUG, "GetCurrentClientChannel: uid=%i", m_iCurrentChannel);
  return m_iCurrentChannel;
}

PVR_ERROR cPVRClientNextPVR::SignalStatus(PVR_SIGNAL_STATUS &signalStatus)
{
  // Not supported yet
  return PVR_ERROR_NO_ERROR;
}


bool cPVRClientNextPVR::CanPauseStream(void)
{
  return (m_supportsLiveTimeshift && g_bUseTimeshift);
}

bool cPVRClientNextPVR::CanSeekStream(void)
{
  return (m_supportsLiveTimeshift && g_bUseTimeshift);
}

void Tokenize(const string& str, vector<string>& tokens, const string& delimiters = " ")
{
  string::size_type start_pos = 0;
  string::size_type delim_pos = 0;

  while (string::npos != delim_pos)
  {
    delim_pos = str.find_first_of(delimiters, start_pos);
    tokens.push_back(str.substr(start_pos, delim_pos - start_pos));
    start_pos = delim_pos + 1;
    // Find next "non-delimiter"
  }
}


/************************************************************/
/** Record stream handling */


bool cPVRClientNextPVR::OpenRecordingInternal(long long seekOffset)
{
  if (!m_streamingclient->create())
  {
    XBMC->Log(LOG_ERROR, "Could not connect create streaming socket");
    return false;
  }

  if (!m_streamingclient->connect(g_szHostname, g_iPort))
  {
    XBMC->Log(LOG_ERROR, "Could not connect to NextPVR backend for streaming");
    return false;
  }

  
  char line[256];
  sprintf(line, "GET /live?recording=%s&client=XBMC HTTP/1.0\r\n", m_currentRecordingID);
  m_streamingclient->send(line, strlen(line));

  if (seekOffset != 0)
  {
    sprintf(line, "Range: bytes=%d-\r\n", seekOffset);
    m_streamingclient->send(line, strlen(line));
  }

  sprintf(line, "Connection: close\r\n");
  m_streamingclient->send(line, strlen(line));

  sprintf(line, "\r\n");
  m_streamingclient->send(line, strlen(line));
  
  char buf[1024];
  int read = m_streamingclient->receive(buf, sizeof buf, 0);

  for (int i=0; i<read; i++)
  {
    if (buf[i] == '\r' && buf[i+1] == '\n' && buf[i+2] == '\r' && buf[i+3] == '\n')
    {
      int remainder = read - (i+4);
      if (remainder > 0)
      {
        m_incomingStreamBuffer.WriteData((char *)&buf[i+4], remainder);
      }

      // extract recording length from HTTP headers
      if (seekOffset == 0 && m_currentRecordingLength == 0)
      {
        char tempHeader[256];
        if (i < sizeof(tempHeader))
        {
          memset(tempHeader, 0, sizeof(tempHeader));
          memcpy(tempHeader, buf, i);
          XBMC->Log(LOG_DEBUG, "%s", tempHeader);

          std::string header(tempHeader);
          std::vector<string> lines;
          Tokenize(header, lines, "\r\n");
          for (vector<string>::iterator it = lines.begin(); it < lines.end(); ++it)
          {
            std::string& line(*it);
            if (line.find("Content-Length") != std::string::npos)
            {
              m_currentRecordingLength = atoll(&line[line.find(":")] + 2);
              break;
            }
          }
        }
      }

      // long blocking from now on
      m_streamingclient->set_non_blocking(1);
        
      XBMC->Log(LOG_DEBUG, "OpenRecordingInternal returning 'true'");
      return true;
    }
  }

  XBMC->Log(LOG_DEBUG, "OpenRecordingInternal returning 'false'");
  return false;
}

bool cPVRClientNextPVR::OpenRecordedStream(const PVR_RECORDING &recording)
{
  XBMC->Log(LOG_DEBUG, "OpenRecordedStream(%s:%s)", recording.strRecordingId, recording.strTitle);
  
  m_currentRecordingLength = 0;
  m_currentRecordingPosition = 0;
  PVR_STRCLR(m_currentRecordingID);

  if (strstr(recording.strStreamURL, "live?recording") == NULL)
  {
    PVR_STRCPY(m_currentRecordingID, recording.strRecordingId);
    return OpenRecordingInternal(0);
  }

  return true;
}

void cPVRClientNextPVR::CloseRecordedStream(void)
{
  m_streamingclient->close();

  m_currentRecordingLength = 0;
  m_currentRecordingPosition = 0;
}

int cPVRClientNextPVR::ReadRecordedStream(unsigned char *pBuffer, unsigned int iBufferSize)
{
  PLATFORM::CLockObject lock(m_mutex);
  XBMC->Log(LOG_DEBUG, "ReadRecordedStream(%d bytes from offset %d)", iBufferSize, (int)m_currentRecordingPosition);

  // do we have enough data to fill this buffer? 
  unsigned char buf[188*100];
  while (m_incomingStreamBuffer.getMaxReadSize() < iBufferSize)
  {
    // no, then read more
    int read = m_streamingclient->receive((char *)buf, sizeof buf, 0);
    if (read > 0)
    {
      // write it to incoming ring buffer
      m_incomingStreamBuffer.WriteData((char *)buf, read);
    }
  }

  // read from buffer to return for XBMC
  m_incomingStreamBuffer.ReadData((char *)pBuffer, iBufferSize);
  m_currentRecordingPosition += iBufferSize;
  XBMC->Log(LOG_DEBUG, "ReadRecordedStream return %d bytes", iBufferSize);
  return iBufferSize;
}

long long cPVRClientNextPVR::SeekRecordedStream(long long iPosition, int iWhence)
{
  PLATFORM::CLockObject lock(m_mutex);

  if (m_currentRecordingLength != 0)
  {
    m_streamingclient->close();
    if (iWhence == SEEK_END)
      iPosition = m_currentRecordingPosition - iPosition;

    XBMC->Log(LOG_DEBUG, "SeekRecordedStream(%d, %d)", (int)iPosition, iWhence);

    OpenRecordingInternal(iPosition);
    m_currentRecordingPosition = iPosition;
    return iPosition;
  }

  XBMC->Log(LOG_DEBUG, "SeekRecordedStream returning -1");

  // not supported
  return -1;  
}

long long cPVRClientNextPVR::PositionRecordedStream(void)
{
  if (m_currentRecordingLength != 0)
  {
    XBMC->Log(LOG_DEBUG, "PositionRecordedStream returning %d", (int)m_currentRecordingPosition);
    return m_currentRecordingPosition;
  }

  XBMC->Log(LOG_DEBUG, "PositionRecordedStream returning -1");

  // not supported
  return -1;
}

long long  cPVRClientNextPVR::LengthRecordedStream(void)
{
  if (m_currentRecordingLength != 0)
  {
    XBMC->Log(LOG_DEBUG, "LengthRecordedStream returning %d", (int)m_currentRecordingLength);
    return m_currentRecordingLength;
  }

  XBMC->Log(LOG_DEBUG, "LengthRecordedStream returning -1");

  // not supported
  return -1;
}

const char* cPVRClientNextPVR::GetLiveStreamURL(const PVR_CHANNEL &channelinfo)
{
  string result;
  XBMC->Log(LOG_DEBUG, "GetLiveStreamURL(uid=%i)", channelinfo.iUniqueId);
  if (!OpenLiveStream(channelinfo))
  {
    return "";
  }
  else
  {
    return m_PlaybackURL.c_str();
  }
}



/************************************************************/
/** http handling */
int cPVRClientNextPVR::DoRequest(const char *resource, CStdString &response)
{
  PLATFORM::CLockObject lock(m_mutex);

  // build request string, adding SID if requred
  CStdString strURL;
  if (strstr(resource, "method=session") == NULL)
    strURL.Format("http://%s:%d%s&sid=%s", g_szHostname, g_iPort, resource, m_sid);
  else
    strURL.Format("http://%s:%d%s", g_szHostname, g_iPort, resource);

  // ask XBMC to read the URL for us
  int resultCode = HTTP_NOTFOUND;
  void* fileHandle = XBMC->OpenFile(strURL.c_str(), 0);
  if (fileHandle)
  {
    char buffer[1024];
    while (XBMC->ReadFileString(fileHandle, buffer, 1024))
      response.append(buffer);
    XBMC->CloseFile(fileHandle);
    resultCode = HTTP_OK;
  }
  
  return resultCode;
}
