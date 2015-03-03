#include "PctvData.h"
#include "client.h"
#include <iostream>
#include <fstream>
#include <string>
#include <map>

#include "md5.h"



#define URI_REST_CONFIG         "/TVC/free/data/config"
#define URI_REST_CHANNELS       "/TVC/user/data/tv/channels"
#define URI_REST_CHANNELLISTS   "/TVC/user/data/tv/channellists"
#define URI_REST_RECORDINGS     "/TVC/user/data/gallery/video"
#define URI_REST_TIMER          "/TVC/user/data/recordingtasks"
#define URI_REST_EPG            "/TVC/user/data/epg"
#define URI_REST_STORAGE        "/TVC/user/data/storage"
#define URI_REST_FOLDER	        "/TVC/user/data/folder"

#define DEFAULT_TV_PIN          "0000"

#define URI_INDEX_HTML			"/TVC/common/Login.html"

#define DEFAULT_PREVIEW_MODE    "m2ts"
#define DEFAULT_PROFILE         "m2ts.Native.NR"
#define DEFAULT_REC_PROFILE     "m2ts.4000k.HR"

using namespace std;
using namespace ADDON;
using namespace PLATFORM;



/************************************************************/
/** Class interface */

Pctv::Pctv() :m_strBaseUrl(""), m_strStid(""), m_strPreviewMode(DEFAULT_PREVIEW_MODE), m_config({ "", "", "", 0, "" })
{   
  m_bIsConnected = false;      
  m_bUpdating = false;  
  m_iNumChannels = 0;
  m_iNumRecordings = 0;  
  m_iNumGroups = 0;  
  m_iBitrate = g_iBitrate;
  m_bTranscode = g_bTranscode;
  m_bUsePIN = g_bUsePIN;
  m_iPortWeb = g_iPortWeb;     
  m_strBackendUrlNoAuth.Format("http://%s:%u", g_strHostname.c_str(), m_iPortWeb);
}

void  *Pctv::Process()
{
  XBMC->Log(LOG_DEBUG, "%s - starting", __FUNCTION__);

  CLockObject lock(m_mutex);
  m_started.Broadcast();

  return NULL;
}

Pctv::~Pctv()
{
  CLockObject lock(m_mutex);
  XBMC->Log(LOG_DEBUG, "%s Stopping update thread...", __FUNCTION__);
  StopThread();

  XBMC->Log(LOG_DEBUG, "%s Removing internal channels list...", __FUNCTION__);
  m_channels.clear();
  m_groups.clear();
  m_epg.clear();
  m_recordings.clear();
  m_timer.clear();
  m_partitions.clear();
  m_bIsConnected = false;
  
}

bool Pctv::Open()
{
  CLockObject lock(m_mutex);

  XBMC->Log(LOG_NOTICE, "%s - PCTV Systems Addon Configuration options", __FUNCTION__);
  XBMC->Log(LOG_NOTICE, "%s - Hostname: '%s'", __FUNCTION__, g_strHostname.c_str());
  XBMC->Log(LOG_NOTICE, "%s - WebPort: '%d'", __FUNCTION__, m_iPortWeb);
    
  m_bIsConnected = GetFreeConfig();

  if (!m_bIsConnected)
  {
    XBMC->Log(LOG_ERROR, "%s It seem's that pctv cannot be reached. Make sure that you set the correct configuration options in the addon settings!", __FUNCTION__);
    return false;
  }
 
  // add user:pin in front of the URL if PIN is set  
  CStdString strURL = "";
  std::string strAuth = "";
  if (m_bUsePIN)
  {
	  CStdString pinMD5 = XBMCPVR::XBMC_MD5::GetMD5(g_strPin);
	  pinMD5.ToLower();

	  strURL.Format("User:%s@", pinMD5.c_str());

	  if (IsSupported("broadway"))
	  {
		  strAuth = "/basicauth";
	  }
  }

  strURL.Format("http://%s%s:%u%s", strURL.c_str(), g_strHostname.c_str(), m_iPortWeb, strAuth);
  m_strBaseUrl = strURL;

  // request index.html to force wake-up from standby
  if (IsSupported("broadway")) {
	  int retval;
	  cRest rest;
	  Json::Value response;  

	  std::string strUrl = m_strBaseUrl + URI_INDEX_HTML;
	  retval = rest.Get(strUrl, "", response);
  }

  if (m_channels.size() == 0)
  {
    // Load the TV channels
	LoadChannels();    
  }  
    
  XBMC->Log(LOG_INFO, "%s Starting separate client update thread...", __FUNCTION__);
  CreateThread();

  return IsRunning();
}

void Pctv::CloseLiveStream(void)
{
  XBMC->Log(LOG_DEBUG, "CloseLiveStream");  
}

/************************************************************/
/** Channels  */
PVR_ERROR Pctv::GetChannels(ADDON_HANDLE handle, bool bRadio) 
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);
  m_iNumChannels = 0;
  m_channels.clear();

  Json::Value data;
  int retval;
  retval = RESTGetChannelList(0, data); // all channels

  if (retval < 0)
  {
    XBMC->Log(LOG_ERROR, "No channels available.");
    return PVR_ERROR_SERVER_ERROR;
  }
  
  for (unsigned int index = 0; index < data.size(); ++index)
  {
    PctvChannel channel;
    Json::Value entry;

    entry = data[index];
    
    channel.iUniqueId = entry["Id"].asInt();
    channel.strChannelName = entry["DisplayName"].asString();    
	if (entry["MajorChannelNo"] != Json::nullValue)
	{
		channel.iChannelNumber = entry["MajorChannelNo"].asInt();
	}	
	else 
	{
		channel.iChannelNumber = entry["Id"].asInt();
	}
	if (entry["MinorChannelNo"] != Json::nullValue)
	{
		channel.iSubChannelNumber = entry["MinorChannelNo"].asInt();
	}
	else {
		channel.iSubChannelNumber = 0;
	}
	channel.iEncryptionSystem = 0;	
    CStdString params;    
    params = GetPreviewParams(handle, entry);
    channel.strStreamURL = GetPreviewUrl(params);    
    channel.strLogoPath = GetChannelLogo(entry);
    m_iNumChannels++;
    m_channels.push_back(channel);
    
    XBMC->Log(LOG_DEBUG, "%s loaded Channel entry '%s'", __FUNCTION__, channel.strChannelName.c_str());
  }
  
  if (m_channels.size() > 0) {
	std::sort(m_channels.begin(), m_channels.end());
  }
	  
  XBMC->QueueNotification(QUEUE_INFO, "%d channels loaded.", m_channels.size());
  
  TransferChannels(handle);

  return PVR_ERROR_NO_ERROR;
}


void Pctv::TransferChannels(ADDON_HANDLE handle)
{
  for (unsigned int i = 0; i < m_channels.size(); i++)
  {
    CStdString strTmp;
    PctvChannel &channel = m_channels.at(i);
    PVR_CHANNEL tag;
    memset(&tag, 0, sizeof(PVR_CHANNEL));
    tag.iUniqueId = channel.iUniqueId;
	tag.iChannelNumber = channel.iChannelNumber;
	tag.iSubChannelNumber = channel.iSubChannelNumber;
	tag.iEncryptionSystem = channel.iEncryptionSystem;	
    strncpy(tag.strChannelName, channel.strChannelName.c_str(), sizeof(tag.strChannelName));
    strncpy(tag.strInputFormat, m_strPreviewMode.c_str(), sizeof(tag.strInputFormat));
    strncpy(tag.strStreamURL, channel.strStreamURL.c_str(), sizeof(tag.strStreamURL));
    strncpy(tag.strIconPath, channel.strLogoPath.c_str(), sizeof(tag.strIconPath));
    PVR->TransferChannelEntry(handle, &tag);
  }
}


bool Pctv::LoadChannels()
{
  PVR->TriggerChannelGroupsUpdate();
  PVR->TriggerChannelUpdate();
  return true;  
}

/************************************************************/
/** Groups  */

unsigned int Pctv::GetChannelGroupsAmount()
{
  return m_iNumGroups;
}

PVR_ERROR Pctv::GetChannelGroups(ADDON_HANDLE handle, bool bRadio) 
{
  m_iNumGroups = 0;
  m_groups.clear();

  Json::Value data;
  int retval;
  retval = RESTGetChannelLists(data);

  if (retval < 0)
  {
    XBMC->Log(LOG_ERROR, "No channels available.");
    return PVR_ERROR_SERVER_ERROR;
  }

  for (unsigned int index = 0; index < data.size(); ++index)
  { 
    PctvChannelGroup group;
    Json::Value entry;

    entry = data[index];
    int iChannelListId = entry["Id"].asInt();

    Json::Value channellistData;    
    retval = RESTGetChannelList(iChannelListId, channellistData);
	if (retval > 0) {
		Json::Value channels = channellistData["Channels"];

		for (unsigned int i = 0; i < channels.size(); ++i) {

			Json::Value channel;

			channel = channels[i];
			group.members.push_back(channel["Id"].asInt());
		}
	}
	
    group.iGroupId = iChannelListId;
    group.strGroupName = entry["DisplayName"].asString();
    group.bRadio = false;
        
    m_groups.push_back(group);
    m_iNumGroups++;

    XBMC->Log(LOG_DEBUG, "%s loaded channelist entry '%s'", __FUNCTION__, group.strGroupName.c_str());
  }

  XBMC->QueueNotification(QUEUE_INFO, "%d groups loaded.", m_groups.size());

  TransferGroups(handle);

  return PVR_ERROR_NO_ERROR;
}

void Pctv::TransferGroups(ADDON_HANDLE handle)
{
  for (unsigned int i = 0; i<m_groups.size(); i++)
  {
    CStdString strTmp;
    PctvChannelGroup &group = m_groups.at(i);
  
    PVR_CHANNEL_GROUP tag;
    memset(&tag, 0, sizeof(PVR_CHANNEL_GROUP));
    tag.bIsRadio = false;
    strncpy(tag.strGroupName, group.strGroupName.c_str(), sizeof(tag.strGroupName));
    
    PVR->TransferChannelGroup(handle, &tag);
  }
}

PVR_ERROR Pctv::GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group)
{
  CStdString strTmp = group.strGroupName;
  for (unsigned int i = 0; i < m_groups.size(); i++)
  {
    PctvChannelGroup &g = m_groups.at(i);
    if (!strTmp.compare(g.strGroupName)) 
    {
      for (unsigned int i = 0; i<g.members.size(); i++) 
      {
        PVR_CHANNEL_GROUP_MEMBER tag;
        memset(&tag, 0, sizeof(PVR_CHANNEL_GROUP_MEMBER));

        tag.iChannelUniqueId = g.members[i];
        strncpy(tag.strGroupName, g.strGroupName.c_str(), sizeof(tag.strGroupName));
        
        PVR->TransferChannelGroupMember(handle, &tag);
      }      
    }      
  }

  return PVR_ERROR_NO_ERROR;
}

int Pctv::RESTGetChannelLists(Json::Value& response)
{
  int retval;
  cRest rest;

  std::string strUrl = m_strBaseUrl + URI_REST_CHANNELLISTS;
  retval = rest.Get(strUrl, "", response);

  if (retval >= 0)
  {
    if (response.type() == Json::arrayValue)
    {
      int size = response.size();
      return size;
    }
    else
    {
      XBMC->Log(LOG_DEBUG, "Unknown response format. Expected Json::arrayValue\n");
      return -1;
    }
  }
  else
  {
    XBMC->Log(LOG_DEBUG, "Request Recordings failed. Return value: %i\n", retval);
  }

  return retval;
}

/************************************************************/
/** Recordings  */

PVR_ERROR Pctv::GetRecordings(ADDON_HANDLE handle)
{  
  m_iNumRecordings = 0;
  m_recordings.clear();

  Json::Value data;
  int retval = RESTGetRecordings(data);  
  if (retval > 0) {
	for (unsigned int index = 0; index < data["video"].size(); ++index)
	{
		PctvRecording recording;
		//Json::Value entry;

		//entry = data["video"][index];
		Json::Value entry(data["video"][index]);
		recording.strRecordingId = index;
		recording.strTitle = entry["DisplayName"].asString();
		recording.startTime = static_cast<time_t>(entry["RecDate"].asDouble() / 1000); // in seconds
		recording.iDuration = static_cast<time_t>(entry["Duration"].asDouble() / 1000); // in seconds
		recording.iLastPlayedPosition = static_cast<int>(entry["Resume"].asDouble() / 1000); // in seconds
		
		CStdString params = GetPreviewParams(handle, entry);
		recording.strStreamURL = GetPreviewUrl(params);
		m_iNumRecordings++;
		m_recordings.push_back(recording);

		XBMC->Log(LOG_DEBUG, "%s loaded Recording entry '%s'", __FUNCTION__, recording.strTitle.c_str());
	}
  }
  
  XBMC->QueueNotification(QUEUE_INFO, "%d recordings loaded.", m_recordings.size());
  
  TransferRecordings(handle);

  return PVR_ERROR_NO_ERROR;
}

void Pctv::TransferRecordings(ADDON_HANDLE handle)
{
  for (unsigned int i = 0; i<m_recordings.size(); i++)
  {    
    PctvRecording &recording = m_recordings.at(i);
    PVR_RECORDING tag;
    memset(&tag, 0, sizeof(PVR_RECORDING));
    strncpy(tag.strRecordingId, recording.strRecordingId.c_str(), sizeof(tag.strRecordingId) -1);
    strncpy(tag.strTitle, recording.strTitle.c_str(), sizeof(tag.strTitle) -1);
    strncpy(tag.strStreamURL, recording.strStreamURL.c_str(), sizeof(tag.strStreamURL) -1);
    strncpy(tag.strPlotOutline, recording.strPlotOutline.c_str(), sizeof(tag.strPlotOutline) -1);
    strncpy(tag.strPlot, recording.strPlot.c_str(), sizeof(tag.strPlot) -1);
    strncpy(tag.strChannelName, recording.strChannelName.c_str(), sizeof(tag.strChannelName) -1);
    strncpy(tag.strIconPath, recording.strIconPath.c_str(), sizeof(tag.strIconPath) -1);
	recording.strDirectory = "";
    strncpy(tag.strDirectory, recording.strDirectory.c_str(), sizeof(tag.strDirectory) -1);
    tag.recordingTime = recording.startTime;
    tag.iDuration = recording.iDuration;

    PVR->TransferRecordingEntry(handle, &tag);
  }
}

int Pctv::RESTGetRecordings(Json::Value& response)
{
  cRest rest;
  std::string strUrl = m_strBaseUrl + URI_REST_RECORDINGS;
  int retval = rest.Get(strUrl, "", response);
  if (retval >= 0)
  {
    if (response.type() == Json::objectValue)
    {
		return response["TotalCount"].asInt();		
    }
    else
    {
      XBMC->Log(LOG_DEBUG, "Unknown response format. Expected Json::objectValue\n");
      return -1;
    }
  }
  else
  {
    XBMC->Log(LOG_DEBUG, "Request Recordings failed. Return value: %i\n", retval);
  }

  return retval;
}

unsigned int Pctv::GetRecordingsAmount() {
  return m_iNumRecordings;
}

/************************************************************/
/** Timer */

unsigned int Pctv::GetTimersAmount(void)
{  
	return m_timer.size();
}

PVR_ERROR Pctv::GetTimers(ADDON_HANDLE handle)
{  
  m_timer.clear();

  Json::Value data;
  int retval = RESTGetTimer(data);  
  if (retval < 0)
  {
    XBMC->Log(LOG_ERROR, "No timer available.");
    return PVR_ERROR_SERVER_ERROR;
  }

  for (unsigned int index = 0; index < data.size(); ++index)
  {
    PctvTimer timer;
    Json::Value entry = data[index];

    timer.iId = entry["Id"].asInt();
    timer.strTitle = entry["DisplayName"].asString();
    timer.iChannelId = entry["ChannelId"].asInt();
    timer.startTime = static_cast<time_t>(entry["RealStartTime"].asDouble() / 1000);
    timer.endTime = static_cast<time_t>(entry["RealEndTime"].asDouble() / 1000);
    timer.iStartOffset = entry["StartOffset"].asInt();
    timer.iEndOffset = entry["EndOffset"].asInt();      
    
    CStdString strState = entry["State"].asString();        
    if (strState == "Idle" || strState == "Prepared")
    {
      timer.state = PVR_TIMER_STATE_SCHEDULED;
    }
    else if (strState == "Running")
    {
      timer.state = PVR_TIMER_STATE_RECORDING;
    }
    else if (strState == "Done")
    {
      timer.state = PVR_TIMER_STATE_COMPLETED;
    }     
    else
    {
      timer.state = PVR_TIMER_STATE_NEW;  // default
    }
	
    m_timer.push_back(timer);

    XBMC->Log(LOG_DEBUG, "%s loaded Timer entry '%s'", __FUNCTION__, timer.strTitle.c_str());
  }
  
  XBMC->QueueNotification(QUEUE_INFO, "%d timer loaded.", m_timer.size());
  
  TransferTimer(handle);

  return PVR_ERROR_NO_ERROR;
}

void Pctv::TransferTimer(ADDON_HANDLE handle)
{
  for (unsigned int i = 0; i<m_timer.size(); i++)
  {
    CStdString strTmp;
    PctvTimer &timer = m_timer.at(i);
    PVR_TIMER tag;
    memset(&tag, 0, sizeof(PVR_TIMER));
    tag.iClientIndex = timer.iId;
    tag.iClientChannelUid = timer.iChannelId;
    strncpy(tag.strTitle, timer.strTitle.c_str(), sizeof(tag.strTitle));
    tag.startTime = timer.startTime;
    tag.endTime = timer.endTime;
    tag.state = timer.state;
    tag.strDirectory[0] = '\0';
    tag.iPriority = 0;
    tag.iLifetime = 0;
    tag.bIsRepeating = false;
    tag.iEpgUid = 0;

    PVR->TransferTimerEntry(handle, &tag);
  }
}

int Pctv::RESTGetTimer(Json::Value& response)
{
  cRest rest;
  std::string strUrl = m_strBaseUrl + URI_REST_TIMER;
  int retval = rest.Get(strUrl, "", response);

  if (retval >= 0)
  {
    if (response.type() == Json::arrayValue)
    {
      return response.size();
    }
    else
    {
      XBMC->Log(LOG_DEBUG, "Unknown response format. Expected Json::arrayValue\n");
      return -1;
    }
  }
  else
  {
    XBMC->Log(LOG_DEBUG, "Request Timer failed. Return value: %i\n", retval);
  }

  return retval;
}


int Pctv::RESTAddTimer(const PVR_TIMER &timer, Json::Value& response)
{	
  CStdString strQueryString;
  strQueryString.Format("{\"Id\":0,\"ChannelId\":%i,\"State\":\"%s\",\"RealStartTime\":%llu,\"RealEndTime\":%llu,\"StartOffset\":%llu,\"EndOffset\":%llu,\"DisplayName\":\"%s\",\"Recurrence\":%i,\"ChannelListId\":%i,\"Profile\":\"%s\"}",
	  timer.iClientChannelUid, "Idle", static_cast<unsigned long long>(timer.startTime) * 1000, static_cast<unsigned long long>(timer.endTime) * 1000, static_cast<unsigned long long>(timer.iMarginStart) * 1000, static_cast<unsigned long long>(timer.iMarginEnd) * 1000, timer.strTitle, 0, 0, DEFAULT_REC_PROFILE);

  cRest rest;
  std::string strUrl = m_strBaseUrl + URI_REST_TIMER;
  int retval = rest.Post(strUrl, strQueryString, response);

  if (retval >= 0)
  {
    if (response.type() == Json::objectValue)
    {
		retval = 0;
    }
    else
    {
      XBMC->Log(LOG_DEBUG, "Unknown response format. Expected Json::arrayValue\n");
	  return -1;
    }
  }
  else
  {
    XBMC->Log(LOG_DEBUG, "Request Timer failed. Return value: %i\n", retval);
	return -1;
  }
    
  // Trigger a timer update to receive new timer from Broadway
  PVR->TriggerTimerUpdate();
  if (timer.startTime <= 0)
  {
    // Refresh the recordings
    usleep(100000);
    PVR->TriggerRecordingUpdate();
  }

  return retval;
}

PVR_ERROR Pctv::AddTimer(const PVR_TIMER &timer) 
{
  XBMC->Log(LOG_DEBUG, "AddTimer iClientChannelUid: %i\n", timer.iClientChannelUid);
  
  Json::Value data;  
  int retval = RESTAddTimer(timer, data);
  if (retval == 0) {
	  return PVR_ERROR_NO_ERROR;
  }
  
  return PVR_ERROR_SERVER_ERROR;
}



/************************************************************/
/** EPG  */

PVR_ERROR Pctv::GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd)
{
  XBMC->Log(LOG_DEBUG, "%s - Channel: %s\n", __FUNCTION__, channel.strChannelName);
  
  Json::Value data;
  for (vector<PctvChannel>::iterator myChannel = m_channels.begin(); myChannel < m_channels.end(); ++myChannel)
  {
    if (myChannel->iUniqueId != (int)channel.iUniqueId) continue;
	  if (!GetEPG((int)channel.iUniqueId, iStart, iEnd, data)) continue;
    if (data.size() <= 0) continue;

    for (unsigned int index = 0; index < data.size(); ++index) 
    {
      Json::Value buffer = data[index];
      int iChannelId = buffer["Id"].asInt();
      Json::Value entries = buffer["Entries"];
      EPG_TAG epg;
      
      for (unsigned int i = 0; i < entries.size(); ++i)
      {
        Json::Value entry = entries[i];
        memset(&epg, 0, sizeof(EPG_TAG));
	      if (IsSupported("broadway"))
        {
				epg.iUniqueBroadcastId = entry["Id"].asUInt();
			}
			else 
			{
				epg.iUniqueBroadcastId = GetEventId((long long)entry["Id"].asDouble());
			}
            epg.iChannelNumber = iChannelId;                        
            epg.strTitle = entry["Title"].asCString();            
			epg.strPlot = entry["ShortDescription"].asCString();            
			epg.strPlotOutline = entry["LongDescription"].asCString();
            epg.startTime = static_cast<time_t>(entry["StartTime"].asDouble() / 1000);
            epg.endTime = static_cast<time_t>(entry["EndTime"].asDouble() / 1000);
            epg.strEpisodeName = "";
            epg.strIconPath = "";
        PVR->TransferEpgEntry(handle, &epg);
      }
    }

	  return PVR_ERROR_NO_ERROR;
  }
    
  return PVR_ERROR_SERVER_ERROR;
}

unsigned int Pctv::GetEventId(long long EntryId) 
{
	return (unsigned int)((EntryId >> 32) & 0xFFFFFFFFL);
}

bool Pctv::GetEPG(int id, time_t iStart, time_t iEnd, Json::Value& data)
{   
  int retval = RESTGetEpg(id, iStart, iEnd, data);
  if (retval < 0)
  {
    XBMC->Log(LOG_ERROR, "No EPG data retrieved.");
    return false;
  }
  
  XBMC->Log(LOG_NOTICE, "EPG Loaded.");
  return true;
}


int Pctv::RESTGetEpg(int id, time_t iStart, time_t iEnd, Json::Value& response)
{
  CStdString strParams;
  //strParams.Format("?ids=%d&extended=1&start=%d&end=%d", id, iStart * 1000, iEnd * 1000);
  strParams.Format("?ids=%d&extended=1&start=%llu&end=%llu", id, static_cast<unsigned long long>(iStart) * 1000, static_cast<unsigned long long>(iEnd) * 1000);  
  
  cRest rest;
  std::string strUrl = m_strBaseUrl + URI_REST_EPG;
  int retval = rest.Get(strUrl, strParams, response);
  if (retval >= 0)
  {
    if (response.type() == Json::arrayValue)
    {
      return response.size();
    }
    else
    {
      XBMC->Log(LOG_DEBUG, "Unknown response format. Expected Json::arrayValue\n");
      return -1;
    }
  }
  else
  {
    XBMC->Log(LOG_DEBUG, "Request EPG failed. Return value: %i\n", retval);
  }

  return retval;
}


CStdString Pctv::GetPreviewParams(ADDON_HANDLE handle, Json::Value entry) 
{ 
  CStdString strStid = GetStid(handle->dataIdentifier);
  CStdString strTmp;  
  if (entry["File"].isString())
  {  // Gallery entry
    strTmp.Format("stid=%s&galleryid=%.0f&file=%s&profile=%s", strStid, entry["Id"].asDouble(), URLEncodeInline(entry["File"].asString()), GetTranscodeProfileValue());
    return strTmp;
  }  
  
  // channel entry
  strTmp.Format("channel=%i&mode=%s&profile=%s&stid=%s", entry["Id"].asInt(), m_strPreviewMode, GetTranscodeProfileValue(), strStid);
  return strTmp;
  
}

CStdString Pctv::GetTranscodeProfileValue()
{
  CStdString strProfile;
  if (!m_bTranscode)
  {
    strProfile.Format("%s.Native.NR", m_strPreviewMode);
  }
  else
  {
    strProfile.Format("%s.%ik.HR", m_strPreviewMode, m_iBitrate);
  }

  return strProfile;
}

CStdString Pctv::GetPreviewUrl(CStdString params) 
{
  CStdString strTmp;
  strTmp.Format("%s/TVC/Preview?%s", m_strBaseUrl.c_str(), params);
  return strTmp;
}

CStdString Pctv::GetStid(int defaultStid)
{ 
  if (m_strStid == "")
  {    
    m_strStid.Format("_xbmc%i", defaultStid);
  }
    
  return m_strStid;
}

CStdString Pctv::GetChannelLogo(Json::Value entry)
{
  CStdString strNameParam;
  strNameParam.Format("%s/TVC/Resource?type=1&default=emptyChannelLogo&name=%s", m_strBaseUrl.c_str(), URLEncodeInline(GetShortName(entry)));
  return strNameParam;
}

CStdString Pctv::GetShortName(Json::Value entry)
{
  CStdString strShortName;
  if (entry["shortName"].isNull()) 
  {
    strShortName = entry["DisplayName"].asString();
    if (strShortName == "") { strShortName = entry["Name"].asString(); }
    strShortName.Replace(" ", "_");
  }
  
  return strShortName;
}

bool Pctv::IsConnected()
{
  return m_bIsConnected;
}

bool Pctv::GetFreeConfig()
{  	
  CStdString strConfig = "";

  cRest rest;
  Json::Value response;
  std::string strUrl = m_strBackendUrlNoAuth + URI_REST_CONFIG;
  int retval = rest.Get(strUrl, "", response);
  if (retval != E_FAILED)
  {
    if (response.type() == Json::objectValue)
    {      
      m_config.init(response);
    }
    return true;
  }
  
  return false;
}

const char* Pctv::GetBackendName()
{
  return m_strBackendName.c_str();
}

const char* Pctv::GetBackendVersion()
{
  return m_strBackendVersion.c_str();
}

bool Pctv::GetChannel(const PVR_CHANNEL &channel, PctvChannel &myChannel)
{
  for (unsigned int iChannelPtr = 0; iChannelPtr < m_channels.size(); iChannelPtr++)
  {    
    PctvChannel &thisChannel = m_channels.at(iChannelPtr);
    if (thisChannel.iUniqueId == (int)channel.iUniqueId)
    {
      myChannel.iUniqueId = thisChannel.iUniqueId;
      myChannel.bRadio = thisChannel.bRadio;
      myChannel.iChannelNumber = thisChannel.iChannelNumber;
      myChannel.iEncryptionSystem = thisChannel.iEncryptionSystem;
      myChannel.strChannelName = thisChannel.strChannelName;
      myChannel.strLogoPath = thisChannel.strLogoPath;
      myChannel.strStreamURL = thisChannel.strStreamURL;
      return true;
    }
  }

  return false;
}

unsigned int Pctv::GetChannelsAmount()
{
  return m_channels.size();
}



bool Pctv::IsRecordFolderSet(CStdString& partitionId)
{
	Json::Value data;
	int retval = RESTGetFolder(data); // get folder config
	if (retval <= 0) return false;
	
	for (unsigned int i = 0; i < data.size(); i++)
	{
		Json::Value folder = data[i];
		if (folder["Type"].asString() == "record") { 
			partitionId = folder["DevicePartitionId"].asString();
			return true;
		}
	}

	return false;
}


int Pctv::RESTGetFolder(Json::Value& response)
{
	XBMC->Log(LOG_DEBUG, "%s - get folder config via REST interface", __FUNCTION__);

  cRest rest;
	std::string strUrl = m_strBaseUrl + URI_REST_FOLDER;
	int retval = rest.Get(strUrl, "", response);
	if (retval >= 0)
	{
		if (response.type() == Json::arrayValue)
		{
			return response.size();
		}
		else
		{
			XBMC->Log(LOG_DEBUG, "Unknown response format. Expected Json::arrayValue\n");
			return -1;
		}
	}
	else
	{
		XBMC->Log(LOG_DEBUG, "Request folder data failed. Return value: %i\n", retval);
	}

	return retval;
}

int Pctv::RESTGetStorage(Json::Value& response)
{
	XBMC->Log(LOG_DEBUG, "%s - get storage data via REST interface", __FUNCTION__);

	cRest rest;
	std::string strUrl = m_strBaseUrl + URI_REST_STORAGE;
	int retval = rest.Get(strUrl, "", response);
	if (retval >= 0)
	{
		if (response.type() == Json::arrayValue)
		{
			return response.size();
		}
		else
		{
			XBMC->Log(LOG_DEBUG, "Unknown response format. Expected Json::arrayValue\n");
			return -1;
		}
	}
	else
	{
		XBMC->Log(LOG_DEBUG, "Request storage data failed. Return value: %i\n", retval);
	}
	
	return retval;
}

PVR_ERROR Pctv::GetStorageInfo(long long *total, long long *used)
{
	m_partitions.clear();
	CStdString strPartitionId = "";
	
	bool isRecordFolder = IsRecordFolderSet(strPartitionId);
	
	if (isRecordFolder) {
		Json::Value data;

		int retval = RESTGetStorage(data); // get storage data
		if (retval <= 0)
		{
			XBMC->Log(LOG_ERROR, "No storage available.");
			return PVR_ERROR_SERVER_ERROR;
		}

		for (unsigned int i = 0; i < data.size(); i++)
		{
			Json::Value storage = data[i];
			std::string deviceId = storage["Id"].asString();
			Json::Value devicePartitions = storage["Partitions"];
			
			int iCount = devicePartitions.size();
			if (iCount > 0) {
				for (int p = 0; p < iCount; p++)
				{
					Json::Value partition;
					partition = devicePartitions[p];

					CStdString strDevicePartitionId;
					strDevicePartitionId.Format("%s.%s", deviceId, partition["Id"].asString());

					if (strDevicePartitionId == strPartitionId)
          {
						uint32_t size = partition["Size"].asUInt();
						uint32_t available = partition["Available"].asUInt();						

						*total = size;
						*used = (size - available);
						
						/* Convert from kBytes to Bytes */
						*total *= 1024;
						*used *= 1024;
						return PVR_ERROR_NO_ERROR;
					}
				}
			}
		}		
	}

	return PVR_ERROR_SERVER_ERROR;
}


bool Pctv::replace(std::string& str, const std::string& from, const std::string& to) {
  size_t start_pos = str.find(from);
  if (start_pos == std::string::npos)
    return false;

  str.replace(start_pos, from.length(), to);
  return true;
}


int Pctv::ReadLiveStream(unsigned char *pBuffer, unsigned int iBufferSize)
{
  return 0;
}

long long Pctv::SeekLiveStream(long long iPosition, int iWhence /* = SEEK_SET */)
{
  return 0;
}

long long Pctv::PositionLiveStream(void)
{
  return 0;
}

long long Pctv::LengthLiveStream(void)
{
  return 0;
}

/* ################ misc ################ */

/*
* \brief Get a channel list from PCTV Device via REST interface
* \param id The channel list id
*/
int Pctv::RESTGetChannelList(int id, Json::Value& response)
{
  XBMC->Log(LOG_DEBUG, "%s - get channel list entries via REST interface", __FUNCTION__);
  int retval = -1;
  cRest rest;  

  if (id == 0) // all channels
  {
	  std::string strUrl = m_strBaseUrl + URI_REST_CHANNELS;
	  retval = rest.Get(strUrl, "?available=1", response);
    if (retval >= 0)
    {
      if (response.type() == Json::arrayValue)
      {
        return response.size();
      }
      else
      {
        XBMC->Log(LOG_DEBUG, "Unknown response format. Expected Json::arrayValue\n");
        return -1;
      }
    }
    else
    {
      XBMC->Log(LOG_DEBUG, "Request Channel List failed. Return value: %i\n", retval);
    }
  }
  else if (id > 0) 
  {
    char url[255];
	sprintf(url, "%s%s/%i", m_strBaseUrl.c_str(), URI_REST_CHANNELLISTS, id);	
	
	retval = rest.Get(url, "?available=1", response);
    if (retval >= 0)
    {
      if (response.type() == Json::objectValue)
      {
        return response.size();
      }
      else
      {
        XBMC->Log(LOG_DEBUG, "Unknown response format. Expected Json::objectValue\n");
        return -1;
      }
    }
    else
    {
      XBMC->Log(LOG_DEBUG, "Request Channel List failed. Return value: %i\n", retval);
    }
  }
  
  return retval;
}

bool Pctv::IsSupported(const std::string& cap)
{
	return m_config.hasCapability(cap);
}


const char SAFE[256] =
{
  /*      0 1 2 3  4 5 6 7  8 9 A B  C D E F */
  /* 0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  /* 1 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  /* 2 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  /* 3 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,

  /* 4 */ 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  /* 5 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,
  /* 6 */ 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  /* 7 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,

  /* 8 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  /* 9 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  /* A */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  /* B */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

  /* C */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  /* D */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  /* E */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  /* F */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};


CStdString Pctv::URLEncodeInline(const CStdString& sSrc)
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
      *pEnd++ = *pSrc;
    else
    {
      // escape this char
      *pEnd++ = '%';
      *pEnd++ = DEC2HEX[*pSrc >> 4];
      *pEnd++ = DEC2HEX[*pSrc & 0x0F];
    }
  }

  std::string sResult((char *)pStart, (char *)pEnd);
  delete[] pStart;
  return sResult;
}


