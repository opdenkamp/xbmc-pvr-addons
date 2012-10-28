#include "DvbData.h"

#include "client.h" 

using namespace ADDON;
using namespace PLATFORM;

void Dvb::TimerUpdates()
{
  std::vector<DvbTimer> newtimer = LoadTimers();

  for (unsigned int i=0; i<m_timers.size(); i++)
  {
    m_timers[i].iUpdateState = DVB_UPDATE_STATE_NONE;
  }
  
  unsigned int iUpdated=0;
  unsigned int iUnchanged=0; 

  for (unsigned int j=0;j<newtimer.size(); j++) {
    for (unsigned int i=0; i<m_timers.size(); i++) 
    {
      if (m_timers[i].like(newtimer[j]))
      {
        if(m_timers[i] == newtimer[j])
        {
          m_timers[i].iUpdateState = DVB_UPDATE_STATE_FOUND;
          newtimer[j].iUpdateState = DVB_UPDATE_STATE_FOUND;
          iUnchanged++;
        }
        else
        {
          newtimer[j].iUpdateState = DVB_UPDATE_STATE_UPDATED;
          m_timers[i].iUpdateState = DVB_UPDATE_STATE_UPDATED;
          m_timers[i].strTitle = newtimer[j].strTitle;
          m_timers[i].strPlot = newtimer[j].strPlot;
          m_timers[i].iChannelId = newtimer[j].iChannelId;
          m_timers[i].startTime = newtimer[j].startTime;
          m_timers[i].endTime = newtimer[j].endTime;
          m_timers[i].bRepeating = newtimer[j].bRepeating;
          m_timers[i].iWeekdays = newtimer[j].iWeekdays;
          m_timers[i].iEpgID = newtimer[j].iEpgID;
          m_timers[i].iTimerID = newtimer[j].iTimerID;
          m_timers[i].iPriority = newtimer[j].iPriority;
          m_timers[i].iFirstDay = newtimer[j].iFirstDay;
          m_timers[i].state = newtimer[j].state;

          iUpdated++;

        }
      }
    }
  }

  unsigned int iRemoved = 0;

  for (unsigned int i=0; i<m_timers.size(); i++)
  {
    if (m_timers.at(i).iUpdateState == DVB_UPDATE_STATE_NONE)
    {
      XBMC->Log(LOG_INFO, "%s Removed timer: '%s', ClientIndex: '%d'", __FUNCTION__, m_timers.at(i).strTitle.c_str(), m_timers.at(i).iClientIndex);
      m_timers.erase(m_timers.begin()+i);
      i=0;
      iRemoved++;
    }
  }
  unsigned int iNew=0;

  for (unsigned int i=0; i<newtimer.size();i++)
  { 
    if(newtimer.at(i).iUpdateState == DVB_UPDATE_STATE_NEW)
    {  
      DvbTimer &timer = newtimer.at(i);
      timer.iClientIndex = m_iClientIndexCounter;
      XBMC->Log(LOG_INFO, "%s New timer: '%s', ClientIndex: '%d'", __FUNCTION__, timer.strTitle.c_str(), m_iClientIndexCounter);
      m_timers.push_back(timer);
      m_iClientIndexCounter++;
      iNew++;
    } 
  }
 
  XBMC->Log(LOG_INFO, "%s No of timers: removed [%d], untouched [%d], updated '%d', new '%d'", __FUNCTION__, iRemoved, iUnchanged, iUpdated, iNew); 

  if (iRemoved != 0 || iUpdated != 0 || iNew != 0) 
  {
    XBMC->Log(LOG_INFO, "%s Changes in timerlist detected, trigger an update!", __FUNCTION__);
    PVR->TriggerTimerUpdate();
  }
}

Dvb::Dvb() 
{
  m_bIsConnected = false;
  m_strServerName = "DVBViewer";
  CStdString strURL = "", strURLStream = "", strURLRecording = "";

  // simply add user@pass in front of the URL if username/password is set
  if ((g_strUsername.length() > 0) && (g_strPassword.length() > 0))
    strURL.Format("%s:%s@", g_strUsername.c_str(), g_strPassword.c_str());
  strURL.Format("http://%s%s:%u/", strURL.c_str(), g_strHostname.c_str(), g_iPortWeb);
  m_strURL = strURL.c_str();

  if ((g_strUsername.length() > 0) && (g_strPassword.length() > 0))
    strURLRecording.Format("%s:%s@", g_strUsername.c_str(), g_strPassword.c_str());
  strURLRecording.Format("http://%s%s:%u/", strURLRecording.c_str(), g_strHostname.c_str(), g_iPortRecording);
  m_strURLRecording = strURLRecording.c_str();

  if ((g_strUsername.length() > 0) && (g_strPassword.length() > 0))
    strURLStream.Format("%s:%s@", g_strUsername.c_str(), g_strPassword.c_str());
  strURLStream.Format("http://%s%s:%u/", strURLStream.c_str(), g_strHostname.c_str(), g_iPortStream);
  m_strURLStream = strURLStream.c_str();

  m_iNumRecordings = 0;
  m_iNumChannelGroups = 0;
  m_iCurrentChannel = -1;
  m_iClientIndexCounter = 1;

  m_iUpdateTimer = 0;
  m_bUpdateTimers = false;
  m_bUpdateEPG = false;
}

bool Dvb::Open()
{
  CLockObject lock(m_mutex);

  XBMC->Log(LOG_NOTICE, "%s - DVBViewer Addon Configuration options", __FUNCTION__);
  XBMC->Log(LOG_NOTICE, "%s - Hostname: '%s'", __FUNCTION__, g_strHostname.c_str());
  XBMC->Log(LOG_NOTICE, "%s - WebPort: '%d'", __FUNCTION__, g_iPortWeb);
  XBMC->Log(LOG_NOTICE, "%s - StreamPort: '%d'", __FUNCTION__, g_iPortStream);
  
  m_bIsConnected = GetDeviceInfo();

  if (!m_bIsConnected)
    return false;

  if (!LoadChannels())
    return false;

  m_strEPGLanguage = GetPreferredLanguage();

  CStdString url; 
  url.Format("%s%s", m_strURL.c_str(), "api/status.html"); 

  CStdString strXML;
  strXML = GetHttpXML(url);

  XMLResults xe;
  XMLNode xMainNode = XMLNode::parseString(strXML.c_str(), NULL, &xe);
  
  if(xe.error != 0)  {
    XBMC->Log(LOG_ERROR, "%s Unable to parse XML. Error: '%s' ", __FUNCTION__, XMLNode::getError(xe.error));
  }
  else {
    XMLNode xNode = xMainNode.getChildNode("status");
    GetInt(xNode, "timezone", m_iTimezone);
  }

  TimerUpdates();

  XBMC->Log(LOG_INFO, "%s Starting separate client update thread...", __FUNCTION__);
  CreateThread(); 
  
  return IsRunning(); 
}

void  *Dvb::Process()
{
  XBMC->Log(LOG_DEBUG, "%s - starting", __FUNCTION__);

  while(!IsStopped())
  {
    Sleep(1000);
    m_iUpdateTimer++;

    if (m_bUpdateEPG)
    {
      Sleep(8000); /* Sleep enough time to let the recording service grab the EPG data */
      PVR->TriggerEpgUpdate(m_iCurrentChannel);
      m_bUpdateEPG = false;
    }

    if ((m_iUpdateTimer > 60) || m_bUpdateTimers)
    {
      m_iUpdateTimer = 0;
 
      // Trigger Timer and Recording updates acording to the addon settings
      CLockObject lock(m_mutex);
      XBMC->Log(LOG_INFO, "%s Perform Updates!", __FUNCTION__);

      if (m_bUpdateTimers)
      {
        Sleep(500);
        m_bUpdateTimers = false;
      }
      TimerUpdates();
      PVR->TriggerRecordingUpdate();
    }
  }

  CLockObject lock(m_mutex);
  m_started.Broadcast();
  //XBMC->Log(LOG_DEBUG, "%s - exiting", __FUNCTION__);

  return NULL;
}

bool Dvb::LoadChannels() 
{
  ChannelsDat *channel;
  int num_channels, channel_pos = 0;

  m_groups.clear();
  m_iNumChannelGroups = 0;

  CStdString url;
  url.Format("%sapi/getchannelsdat.html", m_strURL.c_str()); 
  CStdString strBIN;
  strBIN = GetHttpXML(url);
  if (strBIN.IsEmpty())
  {
    XBMC->Log(LOG_ERROR, "%s Unable to load channels.dat", __FUNCTION__);
    return false;
  }
  strBIN.erase(0, CHANNELDAT_HEADER_SIZE);
  channel = (ChannelsDat*)strBIN.data();
  num_channels = strBIN.size() / sizeof(ChannelsDat);

  std::vector<DvbChannel> channelsdat;
  std::vector<DvbChannelGroup> groupsdat;
  DvbChannel datChannel;
  DvbChannelGroup datGroup;

  for (int i=0; i<num_channels; i++)
  {
    if (i) channel++;
    if (channel->Flags & ADDITIONAL_AUDIO_TRACK_FLAG) continue;
    char channel_group[26] = "";
    if ((strcmp(channel_group, channel->Category) != 0) && (channel->Flags & VIDEO_FLAG))
      {
        datGroup.strGroupName = strncpy(channel_group, channel->Category, channel->Category_len);
#ifdef WIN32
        AnsiToUtf8(datGroup.strGroupName);
#else
        XBMC->UnknownToUTF8(datGroup.strGroupName);
#endif
        groupsdat.push_back(datGroup);
        m_iNumChannelGroups++; 
      }
    channel_pos++;

    /* EPGChannelID = (TunerType + 1)*2^48 + NID*2^32 + TID*2^16 + SID */
    datChannel.llEpgId = ((uint64_t)(channel->TunerType + 1)<<48) + ((uint64_t)channel->OriginalNetwork_ID<<32) + ((uint64_t)channel->TransportStream_ID<<16) + channel->Service_ID;
    datChannel.Encrypted = channel->Flags & ENCRYPTED_FLAG;
    /* ChannelID = (tunertype + 1) * 536870912 + APID * 65536 + SID */
    datChannel.iChannelId = ((channel->TunerType + 1) << 29) + (channel->Audio_PID << 16) + channel->Service_ID;
    datChannel.bRadio = (channel->Flags & VIDEO_FLAG) == 0;
    datChannel.strGroupName = datGroup.strGroupName;
    datChannel.iUniqueId = channelsdat.size()+1;
    datChannel.iChannelNumber = channelsdat.size()+1;

    char channel_name[26] = "";
    CStdString strChannelName = strncpy(channel_name, channel->ChannelName, channel->ChannelName_len);
    if (channel->ChannelName_len == 25)
    {
      if (channel->Root[channel->Root_len] > 0)
      {
        char channel_root[26] = "";
        CStdString strRoot = strncpy(channel_root, channel->Root, 25);
        strChannelName.append(strRoot.substr(channel->Root_len + 1, channel->Root[channel->Root_len]));
        if (channel->Category[channel->Category_len] > 0)
        {
          char channel_category[26] = "";
          CStdString strCategory = strncpy(channel_category, channel->Category, 25);
          strChannelName.append(strCategory.substr(channel->Category_len + 1, channel->Category[channel->Category_len]));
        }
      }
    }
    datChannel.strChannelName = strChannelName;
#ifdef WIN32
    AnsiToUtf8(datChannel.strChannelName);
#else
    XBMC->UnknownToUTF8(datChannel.strChannelName);
#endif
  
    CStdString strTmp;
    strTmp.Format("%supnp/channelstream/%d.ts", m_strURLStream.c_str(), channel_pos-1);
    datChannel.strStreamURL = strTmp;

    strTmp.Format("%sLogos/%s.png", m_strURL.c_str(), URLEncodeInline(datChannel.strChannelName.c_str())); 
    datChannel.strIconPath = strTmp;

    channelsdat.push_back(datChannel);
  }

  if (g_bUseFavourites)
  {
    CStdString urlFav = g_strFavouritesPath;
    if (!XBMC->FileExists(urlFav.c_str(), false))
      urlFav.Format("%sapi/getfavourites.html", m_strURL.c_str());

    CStdString strXML;
    strXML = GetHttpXML(urlFav);

    if(!strXML.size())
    {
      XBMC->Log(LOG_ERROR, "%s Unable to load favourites.xml.", __FUNCTION__);
      return false;
    }

    RemoveNullChars(strXML);
    XMLResults xe;
    XMLNode xMainNode = XMLNode::parseString(strXML.c_str(), NULL, &xe);

    if(xe.error != 0)  {
      XBMC->Log(LOG_ERROR, "%s Unable to parse favourites.xml. Error: '%s' ", __FUNCTION__, XMLNode::getError(xe.error));
      return false;
    }

    XMLNode xNode = xMainNode.getChildNode("settings");
    int m = xNode.nChildNode("section");

    std::vector<DvbChannel> channelsfav;
    std::vector<DvbChannelGroup> groupsfav;
    DvbChannel favChannel;
    DvbChannelGroup favGroup;

    for (int i = 0; i<m; i++)
    {
      CStdString groupName;
      XMLNode xTmp = xNode.getChildNode("section", i);
      int n = xTmp.nChildNode("entry");
      for (int j = 0; j<n; j++)
      {
        CStdString strTmp;
        XMLNode xTmp2 = xTmp.getChildNode("entry", j);
        strTmp = xTmp2.getText();
        uint64_t llFavId;
        if (sscanf(strTmp, "%lld|", &llFavId))
        {
          int iChannelId = llFavId & 0xFFFFFFFF;
          if (n == 1)
            groupName = "";
          for (unsigned int i = 0; i<channelsdat.size(); i++)
          {
            if (channelsdat[i].iChannelId == iChannelId)
            {
              favChannel.llEpgId = channelsdat[i].llEpgId;
              favChannel.Encrypted = channelsdat[i].Encrypted;
              favChannel.iChannelId = channelsdat[i].iChannelId;
              favChannel.bRadio = channelsdat[i].bRadio;
              favChannel.strGroupName = groupName;
              favChannel.iUniqueId = channelsfav.size()+1;
              favChannel.iChannelNumber = channelsfav.size()+1;
              favChannel.strChannelName = channelsdat[i].strChannelName;
              favChannel.strStreamURL = channelsdat[i].strStreamURL;
              favChannel.strIconPath = channelsdat[i].strIconPath;
              channelsfav.push_back(favChannel);
              break;
            }
          }
        }
        else
        {
          groupName = strTmp;
          favGroup.strGroupName = groupName;
#ifdef WIN32
          AnsiToUtf8(favGroup.strGroupName);
#else
          XBMC->UnknownToUTF8(favGroup.strGroupName);
#endif
          groupsfav.push_back(favGroup);
        }
      }
    }
    m_channels = channelsfav;
    m_groups = groupsfav;
  }
  else
  {
    m_channels = channelsdat;
    m_groups = groupsdat;
  }

  XBMC->Log(LOG_INFO, "%s Loaded %d Channelsgroups", __FUNCTION__, m_iNumChannelGroups);
  return true;
}

bool Dvb::IsConnected() 
{
  return m_bIsConnected;
}

CStdString Dvb::GetHttpXML(CStdString& url)
{
  CStdString strResult;
  void* fileHandle = XBMC->OpenFile(url.c_str(), 0);
  if (fileHandle)
  {
    char buffer[1024];
    while (int bytesRead = XBMC->ReadFile(fileHandle, buffer, 1024))
      strResult.append(buffer, bytesRead);
    XBMC->CloseFile(fileHandle);
  }
  return strResult;
}

const char * Dvb::GetServerName() 
{
  return m_strServerName.c_str();  
}

int Dvb::GetChannelsAmount()
{
  return m_channels.size();
}

int Dvb::GetTimersAmount()
{
  return m_timers.size();
}

unsigned int Dvb::GetRecordingsAmount() {
  return m_iNumRecordings;
}

PVR_ERROR Dvb::GetChannels(ADDON_HANDLE handle, bool bRadio)
{
    for (unsigned int iChannelPtr = 0; iChannelPtr < m_channels.size(); iChannelPtr++)
  {
    DvbChannel &channel = m_channels.at(iChannelPtr);
    if (channel.bRadio == bRadio)
    {
      PVR_CHANNEL xbmcChannel;
      memset(&xbmcChannel, 0, sizeof(PVR_CHANNEL));

      xbmcChannel.iUniqueId         = channel.iUniqueId;
      xbmcChannel.bIsRadio          = channel.bRadio;
      xbmcChannel.iChannelNumber    = channel.iChannelNumber;
      strncpy(xbmcChannel.strChannelName, channel.strChannelName.c_str(), sizeof(xbmcChannel.strChannelName));
      strncpy(xbmcChannel.strInputFormat, "", sizeof(xbmcChannel.strInputFormat)); // unused

      CStdString strStream;
      strStream.Format("pvr://stream/tv/%i.ts", channel.iUniqueId);
      strncpy(xbmcChannel.strStreamURL, strStream.c_str(), sizeof(xbmcChannel.strStreamURL)); //channel.strStreamURL.c_str();
      xbmcChannel.iEncryptionSystem = channel.Encrypted;
      
      strncpy(xbmcChannel.strIconPath, channel.strIconPath.c_str(), sizeof(xbmcChannel.strIconPath));
      xbmcChannel.bIsHidden         = false;

      PVR->TransferChannelEntry(handle, &xbmcChannel);
    }
  }

  return PVR_ERROR_NO_ERROR;
}

Dvb::~Dvb() 
{
  StopThread();
  
  m_channels.clear();  
  m_timers.clear();
  m_recordings.clear();
  m_groups.clear();
  m_bIsConnected = false;
}

int Dvb::ParseDateTime(CStdString strDate, bool iDateFormat)
{
  struct tm timeinfo;

  memset(&timeinfo, 0, sizeof(tm));
  if (iDateFormat)
    sscanf(strDate, "%04d%02d%02d%02d%02d%02d", &timeinfo.tm_year, &timeinfo.tm_mon, &timeinfo.tm_mday, &timeinfo.tm_hour, &timeinfo.tm_min, &timeinfo.tm_sec);
  else
    sscanf(strDate, "%02d.%02d.%04d%02d:%02d:%02d", &timeinfo.tm_mday, &timeinfo.tm_mon, &timeinfo.tm_year, &timeinfo.tm_hour, &timeinfo.tm_min, &timeinfo.tm_sec);
  timeinfo.tm_mon  -= 1;
  timeinfo.tm_year -= 1900;
  timeinfo.tm_isdst = -1;

  return mktime(&timeinfo);
}

PVR_ERROR Dvb::GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd)
{
  DvbChannel &myChannel = m_channels.at(channel.iUniqueId-1);

  CStdString url;
  url.Format("%sapi/epg.html?lvl=2&channel=%lld&start=%f&end=%f",  m_strURL.c_str(), myChannel.llEpgId, iStart/86400.0 + DELPHI_DATE, iEnd/86400.0 + DELPHI_DATE); 
 
  CStdString strXML;
  strXML = GetHttpXML(url);

  int iNumEPG = 0;

  XMLResults xe;
  XMLNode xMainNode = XMLNode::parseString(strXML.c_str(), NULL, &xe);

  if(xe.error != 0)  {
    XBMC->Log(LOG_ERROR, "%s Unable to parse XML. Error: '%s' ", __FUNCTION__, XMLNode::getError(xe.error));
    return PVR_ERROR_SERVER_ERROR;
  }

  XMLNode xNode = xMainNode.getChildNode("epg");
  int n = xNode.nChildNode("programme");

  XBMC->Log(LOG_INFO, "%s Number of elements: '%d'", __FUNCTION__, n);

  for (int i = 0; i<n; i++)
  {
    CStdString strTmp;
    int iTmpStart;
    int iTmpEnd;

    XMLNode xTmp = xNode.getChildNode("programme", i);
    iTmpStart = ParseDateTime(xNode.getChildNode("programme", i).getAttribute("start"));
    iTmpEnd = ParseDateTime(xNode.getChildNode("programme", i).getAttribute("stop"));

    if ((iEnd > 1) && (iEnd < iTmpEnd))
       continue;
    
    DvbEPGEntry entry;
    entry.startTime = iTmpStart;
    entry.endTime = iTmpEnd;

    if (!GetInt(xTmp, "eventid", entry.iEventId))  
      continue;

    entry.iChannelId = channel.iUniqueId;
    
    if(!GetStringLng(xTmp.getChildNode("titles"), "title", strTmp))
      continue;

    entry.strTitle = strTmp;
    
    CStdString strTmp2;
    GetStringLng(xTmp.getChildNode("descriptions"), "description", strTmp);
    GetStringLng(xTmp.getChildNode("events"), "event", strTmp2);
    if (strTmp.size() > strTmp2.size())
      entry.strPlot = strTmp;
    else
      entry.strPlot = strTmp2;

    EPG_TAG broadcast;
    memset(&broadcast, 0, sizeof(EPG_TAG));

    broadcast.iUniqueBroadcastId  = entry.iEventId;
    broadcast.strTitle            = entry.strTitle.c_str();
    broadcast.iChannelNumber      = channel.iChannelNumber;
    broadcast.startTime           = entry.startTime;
    broadcast.endTime             = entry.endTime;
    broadcast.strPlotOutline      = entry.strPlotOutline.c_str();
    broadcast.strPlot             = entry.strPlot.c_str();
    broadcast.strIconPath         = ""; // unused
    broadcast.iGenreType          = 0; // unused
    broadcast.iGenreSubType       = 0; // unused
    broadcast.strGenreDescription = "";
    broadcast.firstAired          = 0;  // unused
    broadcast.iParentalRating     = 0;  // unused
    broadcast.iStarRating         = 0;  // unused
    broadcast.bNotify             = false;
    broadcast.iSeriesNumber       = 0;  // unused
    broadcast.iEpisodeNumber      = 0;  // unused
    broadcast.iEpisodePartNumber  = 0;  // unused
    broadcast.strEpisodeName      = ""; // unused

    PVR->TransferEpgEntry(handle, &broadcast);

    iNumEPG++; 

    XBMC->Log(LOG_INFO, "%s loaded EPG entry '%d:%s' channel '%d' start '%d' end '%d'", __FUNCTION__, broadcast.iUniqueBroadcastId, broadcast.strTitle, entry.iChannelId, entry.startTime, entry.endTime);
  }

  XBMC->Log(LOG_INFO, "%s Loaded %u EPG Entries for channel '%s'", __FUNCTION__, iNumEPG, channel.strChannelName);
  return PVR_ERROR_NO_ERROR;
}

int Dvb::GetChannelNumber(CStdString strChannelId)  
{
  int channel;
  sscanf(strChannelId.c_str(), "%d|", &channel);
  for (unsigned int i = 0;i<m_channels.size();  i++)
  {
    if (m_channels[i].iChannelId == channel)
    return i+1;
  }
  return -1;
}

PVR_ERROR Dvb::GetTimers(ADDON_HANDLE handle)
{
  XBMC->Log(LOG_INFO, "%s - timers available '%d'", __FUNCTION__, m_timers.size());
  for (unsigned int i=0; i<m_timers.size(); i++)
  {
    DvbTimer &timer = m_timers.at(i);
    XBMC->Log(LOG_INFO, "%s - Transfer timer '%s', ClientIndex '%d'", __FUNCTION__, timer.strTitle.c_str(), timer.iClientIndex);
    PVR_TIMER tag;
    memset(&tag, 0, sizeof(PVR_TIMER));
    tag.iClientChannelUid = timer.iChannelId;
    tag.startTime         = timer.startTime;
    tag.endTime           = timer.endTime;
    strncpy(tag.strTitle, timer.strTitle.c_str(), sizeof(tag.strTitle));
    strncpy(tag.strDirectory, "/", sizeof(tag.strDirectory));   // unused
    strncpy(tag.strSummary, timer.strPlot.c_str(), sizeof(tag.strSummary));
    tag.state             = timer.state;
    tag.iPriority         = timer.iPriority;
    tag.iLifetime         = 0;     // unused
    tag.bIsRepeating      = timer.bRepeating;
    tag.firstDay          = timer.iFirstDay;
    tag.iWeekdays         = timer.iWeekdays;
    tag.iEpgUid           = timer.iEpgID;
    tag.iMarginStart      = 0;     // unused
    tag.iMarginEnd        = 0;     // unused
    tag.iGenreType        = 0;     // unused
    tag.iGenreSubType     = 0;     // unused
    tag.iClientIndex = timer.iClientIndex;

    PVR->TransferTimerEntry(handle, &tag);
  }
  return PVR_ERROR_NO_ERROR;
}

std::vector<DvbTimer> Dvb::LoadTimers()
{
  CStdString url; 
  url.Format("%s%s", m_strURL.c_str(), "api/timerlist.html"); 

  CStdString strXML;
  strXML = GetHttpXML(url);
  RemoveNullChars(strXML);

  XMLResults xe;
  XMLNode xMainNode = XMLNode::parseString(strXML.c_str(), NULL, &xe);
  
  std::vector<DvbTimer> timers;

  if(xe.error != 0)  {
    XBMC->Log(LOG_ERROR, "%s Unable to parse XML. Error: '%s' ", __FUNCTION__, XMLNode::getError(xe.error));
    return timers;
  }

  XMLNode xNode = xMainNode.getChildNode("Timers");
  int n = xNode.nChildNode("Timer");

  XBMC->Log(LOG_INFO, "%s Number of elements: '%d'", __FUNCTION__, n);
  

  while(n>0)
  {
    int i = n-1;
    n--;
    XMLNode xTmp = xNode.getChildNode("Timer", i);

    CStdString strTmp;
    int iTmp;
    
    if (GetString(xTmp, "Descr", strTmp)) 
      XBMC->Log(LOG_DEBUG, "%s Processing timer '%s'", __FUNCTION__, strTmp.c_str());
 
    DvbTimer timer;
    
    timer.strTitle = strTmp;
    timer.iChannelId = GetChannelNumber(xTmp.getChildNode("Channel").getAttribute("ID"));
    timer.state = PVR_TIMER_STATE_SCHEDULED;

    CStdString DateTime;
    DateTime = xNode.getChildNode("timer", i).getAttribute("Date");
    DateTime.append(xNode.getChildNode("timer", i).getAttribute("Start"));
    timer.startTime = ParseDateTime(DateTime, false);
    timer.endTime = timer.startTime + atoi(xNode.getChildNode("timer", i).getAttribute("Dur"))*60;

    CStdString Weekdays;
    Weekdays = xNode.getChildNode("timer", i).getAttribute("Days");
    timer.iWeekdays = 0;
    for (unsigned int i = 0; i<Weekdays.size(); i++)
    {
      if (Weekdays.data()[i] != '-')
        timer.iWeekdays += (1 << i);
    }

    if (timer.iWeekdays != 0)
      {
        timer.iFirstDay = timer.startTime;
        timer.bRepeating = true; 
      }
    else
      timer.bRepeating = false;

    timer.iPriority = atoi(xNode.getChildNode("Timer", i).getAttribute("Priority"));

    if (xNode.getChildNode("Timer", i).getAttribute("EPGEventID"))
      timer.iEpgID = atoi(xNode.getChildNode("Timer", i).getAttribute("EPGEventID"));

    if (xNode.getChildNode("Timer", i).getAttribute("Enabled")[0] == '0')
      timer.state = PVR_TIMER_STATE_CANCELLED;

    if (GetInt(xTmp, "Recording", iTmp))
    {
      if (iTmp == -1) timer.state = PVR_TIMER_STATE_RECORDING;
    }

    if (GetInt(xTmp, "ID", iTmp))
      timer.iTimerID = iTmp;

    timers.push_back(timer);

    XBMC->Log(LOG_INFO, "%s fetched Timer entry '%s', begin '%d', end '%d'", __FUNCTION__, timer.strTitle.c_str(), timer.startTime, timer.endTime);
  }

  XBMC->Log(LOG_INFO, "%s fetched %u Timer Entries", __FUNCTION__, timers.size());
  return timers; 
}

int Dvb::GetTimerID(const PVR_TIMER &timer)
{
  unsigned int i=0;
  while (i<m_timers.size())
  {
    if (m_timers.at(i).iClientIndex == timer.iClientIndex)
      break;
    else
      i++;
  }
  DvbTimer &Timer = m_timers.at(i);
  return Timer.iTimerID;
}

void Dvb::GenerateTimer(const PVR_TIMER &timer, bool bNewTimer)
{
  XBMC->Log(LOG_DEBUG, "%s - channelUid=%d title=%s epgid=%d", __FUNCTION__, timer.iClientChannelUid, timer.strTitle, timer.iEpgUid);

  struct tm * timeinfo;
  time_t startTime = timer.startTime;
  if (startTime == 0)
    startTime = time(NULL);
  int dor = ((startTime + m_iTimezone*60 - timer.iMarginStart) / DAY_SECS) + DELPHI_DATE;
  timeinfo = gmtime (&startTime);
  int start = (timeinfo->tm_hour*60 + m_iTimezone + timeinfo->tm_min - timer.iMarginStart) % DAY_MINS;
  timeinfo = gmtime (&timer.endTime);
  int stop = (timeinfo->tm_hour*60 + m_iTimezone + timeinfo->tm_min + timer.iMarginEnd) % DAY_MINS;

  char strWeek[8] = "-------";
  for (int i = 0; i<7; i++)
  {
    if (timer.iWeekdays & (1 << i)) strWeek[i] = 'T';
  }

  int iChannelId = m_channels.at(timer.iClientChannelUid-1).iChannelId;
  CStdString strTmp;
  if (bNewTimer)
  {
    if (timer.startTime)
      strTmp.Format("api/timeradd.html?ch=%d&dor=%d&enable=1&start=%d&stop=%d&prio=%d&days=%s&title=%s", iChannelId, dor, start, stop, timer.iPriority, strWeek, URLEncodeInline(timer.strTitle));
    else
      strTmp.Format("api/timeradd.html?ch=%d&dor=%d&enable=1&start=%d&stop=%d&prio=%d&title=%s", iChannelId, dor, start+timer.iMarginStart, stop-timer.iMarginEnd, timer.iPriority, URLEncodeInline(timer.strTitle));
  }
  else
  {
    int enabled = 1;
    if (timer.state == PVR_TIMER_STATE_CANCELLED)
      enabled = 0;
    strTmp.Format("api/timeredit.html?id=%d&ch=%d&dor=%d&enable=%d&start=%d&stop=%d&prio=%d&days=%s&title=%s", GetTimerID(timer), iChannelId, dor, enabled, start, stop, timer.iPriority, strWeek, URLEncodeInline(timer.strTitle));
  }
  
  SendSimpleCommand(strTmp);
  m_bUpdateTimers = true;
}

PVR_ERROR Dvb::AddTimer(const PVR_TIMER &timer)
{
  GenerateTimer(timer);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR Dvb::UpdateTimer(const PVR_TIMER &timer)
{
  GenerateTimer(timer, false);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR Dvb::DeleteTimer(const PVR_TIMER &timer) 
{
  CStdString strTmp;
  strTmp.Format("api/timerdelete.html?id=%d", GetTimerID(timer));
  SendSimpleCommand(strTmp);

  if (timer.state == PVR_TIMER_STATE_RECORDING)
    PVR->TriggerRecordingUpdate();
  
  m_bUpdateTimers = true;

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR Dvb::GetRecordings(ADDON_HANDLE handle)
{
  m_iNumRecordings = 0;
  std::vector<DvbRecording> recthumbs;
  recthumbs = m_recordings;
  m_recordings.clear();

  CStdString url;
  url.Format("%s%s", m_strURL.c_str(), "api/recordings.html?utf8=");
 
  CStdString strXML;
  strXML = GetHttpXML(url);
  RemoveNullChars(strXML);

  XMLResults xe;
  XMLNode xMainNode = XMLNode::parseString(strXML.c_str(), NULL, &xe);

  if(xe.error != 0)  {
    XBMC->Log(LOG_ERROR, "%s Unable to parse XML. Error: '%s' ", __FUNCTION__, XMLNode::getError(xe.error));
    return PVR_ERROR_SERVER_ERROR;
  }

  XMLNode xNode = xMainNode.getChildNode("recordings");
  int n = xNode.nChildNode("recording");

  XBMC->Log(LOG_INFO, "%s Number of elements: '%d'", __FUNCTION__, n);
 
  int iNumRecording = 0;
  static int iGetRecordingsCount = 0;
  int j = n;

  while(n>0)
  {
    int i = n-1;
    n--;
    XMLNode xTmp = xNode.getChildNode("recording", i);
    CStdString strTmp;

    DvbRecording recording;
    if (xNode.getChildNode("recording", i).getAttribute("id"))
      recording.strRecordingId = xNode.getChildNode("recording", i).getAttribute("id");

    if (GetString(xTmp, "title", strTmp))
      recording.strTitle = strTmp;
    
    CStdString strTmp2;
    GetString(xTmp, "desc", strTmp);
    GetString(xTmp, "info", strTmp2);
    if (strTmp.size() > strTmp2.size())
      recording.strPlot = strTmp;
    else
      recording.strPlot = strTmp2;
    
    if (GetString(xTmp, "channel", strTmp))
      recording.strChannelName = strTmp;

    strTmp.Format("%supnp/recordings/%d.ts", m_strURLRecording.c_str(), atoi(recording.strRecordingId.c_str()));
    recording.strStreamURL = strTmp;

    recording.startTime = ParseDateTime(xNode.getChildNode("recording", i).getAttribute("start"));

    int hours, mins, secs;
    sscanf(xNode.getChildNode("recording", i).getAttribute("duration"), "%02d%02d%02d", &hours, &mins, &secs);
    recording.iDuration = hours*60*60 + mins*60 + secs;

    bool bGetThumbnails = true;
    if ((iGetRecordingsCount == 0) && (j > MAX_RECORDING_THUMBS - 1))
      bGetThumbnails = false;

    for (unsigned int i=0; i<recthumbs.size(); i++)
    {
      if ((recthumbs[i].strRecordingId == recording.strRecordingId) && (recthumbs[i].strThumbnailPath.size() > 20) && (recthumbs[i].strThumbnailPath.size() < 100))
      {
        recording.strThumbnailPath = recthumbs[i].strThumbnailPath;
        bGetThumbnails = false;
        break;
      }
    } 

    if (bGetThumbnails)
    {
      url.Format("%sepg_details.html?aktion=epg_details&recID=%s", m_strURL.c_str(), recording.strRecordingId.c_str());
      CStdString strThumb;
      strThumb = GetHttpXML(url);

      unsigned int iThumbnailPos;
      iThumbnailPos = strThumb.find_first_of('_', RECORDING_THUMB_POS);
      strTmp.Format("%sthumbnails/video/%s_SM.jpg", m_strURL.c_str(), strThumb.substr(RECORDING_THUMB_POS, iThumbnailPos - RECORDING_THUMB_POS).c_str());
      recording.strThumbnailPath = strTmp;
    }
   
    PVR_RECORDING tag;
    memset(&tag, 0, sizeof(PVR_RECORDING));
    strncpy(tag.strRecordingId, recording.strRecordingId.c_str(), sizeof(tag.strRecordingId));
    strncpy(tag.strTitle, recording.strTitle.c_str(), sizeof(tag.strTitle));
    strncpy(tag.strStreamURL, recording.strStreamURL.c_str(), sizeof(tag.strStreamURL));
    strncpy(tag.strPlotOutline, recording.strPlotOutline.c_str(), sizeof(tag.strPlotOutline));
    strncpy(tag.strPlot, recording.strPlot.c_str(), sizeof(tag.strPlot));
    strncpy(tag.strChannelName, recording.strChannelName.c_str(), sizeof(tag.strChannelName));
    strncpy(tag.strThumbnailPath, recording.strThumbnailPath.c_str(), sizeof(tag.strThumbnailPath));
    tag.recordingTime     = recording.startTime;
    tag.iDuration         = recording.iDuration;
    strncpy(tag.strDirectory, "/", sizeof(tag.strDirectory));   // unused

    PVR->TransferRecordingEntry(handle, &tag);

    m_iNumRecordings++;
    iNumRecording++;
    m_recordings.push_back(recording);

    XBMC->Log(LOG_INFO, "%s loaded Recording entry '%s', start '%d', length '%d'", __FUNCTION__, tag.strTitle, recording.startTime, recording.iDuration);
  }
  iGetRecordingsCount++;

  XBMC->Log(LOG_INFO, "%s Loaded %u Recording Entries", __FUNCTION__, iNumRecording);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR Dvb::DeleteRecording(const PVR_RECORDING &recinfo) 
{
  CStdString strTmp;
  strTmp.Format("rec_list.html?aktion=delete_rec&recid=%s", recinfo.strRecordingId);
  SendSimpleCommand(strTmp);

  PVR->TriggerRecordingUpdate();

  return PVR_ERROR_NO_ERROR;
}

void Dvb::SendSimpleCommand(const CStdString& strCommandURL)
{
  CStdString url; 
  url.Format("%s%s", m_strURL.c_str(), strCommandURL.c_str()); 
  GetHttpXML(url);
}

CStdString Dvb::URLEncodeInline(const CStdString& strData)
{
  /* Copied from xbmc/URL.cpp */

  CStdString strResult;

  /* wonder what a good value is here is, depends on how often it occurs */
  strResult.reserve( strData.length() * 2 );

  for (int i = 0; i < (int)strData.size(); ++i)
  {
    int kar = (unsigned char)strData[i];
    //if (kar == ' ') strResult += '+'; // obsolete
    if (isalnum(kar) || strchr("-_.!()" , kar) ) // Don't URL encode these according to RFC1738
    {
      strResult += kar;
    }
    else
    {
      CStdString strTmp;
      strTmp.Format("%%%02.2x", kar);
      strResult += strTmp;
    }
  }
  return strResult;
}

bool Dvb::GetInt(XMLNode xRootNode, const char* strTag, int& iIntValue)
{
  XMLNode xNode = xRootNode.getChildNode(strTag );
  if (xNode.isEmpty())
     return false;
  iIntValue = atoi(xNode.getText());
  return true;
}

bool Dvb::GetBoolean(XMLNode xRootNode, const char* strTag, bool& bBoolValue)
{
  XMLNode xNode = xRootNode.getChildNode(strTag );
  if (xNode.isEmpty()) 
    return false;

  CStdString strEnabled = xNode.getText();

  strEnabled.ToLower();
  if (strEnabled == "off" || strEnabled == "no" || strEnabled == "disabled" || strEnabled == "false" || strEnabled == "0" )
    bBoolValue = false;
  else
  {
    bBoolValue = true;
    if (strEnabled != "on" && strEnabled != "yes" && strEnabled != "enabled" && strEnabled != "true")
      return false; // invalid bool switch - it's probably some other string.
  }
  return true;
}

bool Dvb::GetString(XMLNode xRootNode, const char* strTag, CStdString& strStringValue)
{
  XMLNode xNode = xRootNode.getChildNode(strTag );
  if (!xNode.isEmpty())
  {
    strStringValue = xNode.getText();
    return true;
  }
  strStringValue.Empty();
  return false;
}

bool Dvb::GetStringLng(XMLNode xRootNode, const char* strTag, CStdString& strStringValue)
{
  XMLNode xNode;
  bool found = false;
  int n = xRootNode.nChildNode(strTag);

  if (n > 1)
  {
    for (int i = 0; i<n; i++)
    {
      xNode = xRootNode.getChildNode(strTag, i);
      CStdString strTmp;
      strTmp = xNode.getAttribute("lng");
      if (strTmp == m_strEPGLanguage)
      {
        found = true;
        break;
      }
    }
  }

  if (!found) xNode = xRootNode.getChildNode(strTag);
  if (!xNode.isEmpty())
  {
    strStringValue = xNode.getText();
    return true;
  }
  strStringValue.Empty();
  return false;
}

CStdString Dvb::GetPreferredLanguage()
{
  CStdString strXML, url;
  url.Format("%s%s",  m_strURL.c_str(), "config.html?aktion=config"); 
  strXML = GetHttpXML(url);
  unsigned int iLanguagePos = strXML.find("EPGLanguage");
  iLanguagePos = strXML.find(" selected>", iLanguagePos);
  return (strXML.substr(iLanguagePos-4, 3));
}

void Dvb::RemoveNullChars(CStdString &String)
{
  /* favourites.xml and timers.xml sometimes have null chars that screw the xml */
  for (unsigned int i = 0; i<String.size()-1; i++)
  {
    if (String.data()[i] == '\0')
    {
      String.erase(i, 1);
      i--;
    }
  }
}

#ifdef WIN32
void Dvb::AnsiToUtf8(std::string &String)
{
  const int iLenW = MultiByteToWideChar(CP_ACP, 0, String.c_str(), -1, 0, 0);
  wchar_t *wcTmp = new wchar_t[iLenW + 1];
  memset(wcTmp, 0, sizeof(wchar_t)*(iLenW + 1));
  if (MultiByteToWideChar(CP_ACP, 0, String.c_str(), -1, wcTmp, iLenW + 1))
    {
      const int iLenU = WideCharToMultiByte(CP_UTF8, 0, wcTmp, -1, NULL, 0, 0, FALSE);
      char *cTmp = new char[iLenU + 1];
      memset(cTmp, 0, sizeof(char)*(iLenU + 1));
      WideCharToMultiByte(CP_UTF8, 0, wcTmp, -1, cTmp, iLenU + 1, 0, FALSE);
      String = cTmp;
      delete[] cTmp;
    }
  delete[] wcTmp;
}
#endif

PVR_ERROR Dvb::GetChannelGroups(ADDON_HANDLE handle)
{
  for(unsigned int iTagPtr = 0; iTagPtr < m_groups.size(); iTagPtr++)
  {
    PVR_CHANNEL_GROUP tag;
    memset(&tag, 0 , sizeof(PVR_CHANNEL_GROUP));

    tag.bIsRadio     = false;
    strncpy(tag.strGroupName, m_groups[iTagPtr].strGroupName.c_str(), sizeof(tag.strGroupName));

    PVR->TransferChannelGroup(handle, &tag);
  }

  return PVR_ERROR_NO_ERROR;
}


unsigned int Dvb::GetNumChannelGroups() {
  return m_iNumChannelGroups;
}

PVR_ERROR Dvb::GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group)
{
  XBMC->Log(LOG_DEBUG, "%s - group '%s'", __FUNCTION__, group.strGroupName);
  CStdString strTmp = group.strGroupName;
  for (unsigned int i = 0;i<m_channels.size();  i++) 
  {
    DvbChannel &myChannel = m_channels.at(i);
    if (!strTmp.compare(myChannel.strGroupName)) 
    {
      PVR_CHANNEL_GROUP_MEMBER tag;
      memset(&tag,0 , sizeof(PVR_CHANNEL_GROUP_MEMBER));

      strncpy(tag.strGroupName, group.strGroupName, sizeof(tag.strGroupName));
      tag.iChannelUniqueId = myChannel.iUniqueId;
      tag.iChannelNumber   = myChannel.iChannelNumber;

      XBMC->Log(LOG_DEBUG, "%s - add channel %s (%d) to group '%s' channel number %d",
          __FUNCTION__, myChannel.strChannelName.c_str(), tag.iChannelUniqueId, group.strGroupName, myChannel.iChannelNumber);

      PVR->TransferChannelGroupMember(handle, &tag);
    }
  }
  return PVR_ERROR_NO_ERROR;
}

int Dvb::GetCurrentClientChannel(void) 
{
  return m_iCurrentChannel;
}

const char* Dvb::GetLiveStreamURL(const PVR_CHANNEL &channelinfo)
{
  SwitchChannel(channelinfo);

  return m_channels.at(channelinfo.iUniqueId-1).strStreamURL.c_str();
}

bool Dvb::OpenLiveStream(const PVR_CHANNEL &channelinfo)
{
  XBMC->Log(LOG_INFO, "%s channel '%u'", __FUNCTION__, channelinfo.iUniqueId);

  if ((int)channelinfo.iUniqueId == m_iCurrentChannel)
    return true;

  return SwitchChannel(channelinfo);
}

void Dvb::CloseLiveStream(void) 
{
  m_iCurrentChannel = -1;
}

bool Dvb::SwitchChannel(const PVR_CHANNEL &channel)
{
  m_iCurrentChannel = channel.iUniqueId;
  m_bUpdateEPG = true;
  return true;
}

bool Dvb::GetDeviceInfo()
{
  CStdString url; 
  url.Format("%s%s", m_strURL.c_str(), "api/version.html"); 

  CStdString strXML;
  strXML = GetHttpXML(url);

  XMLResults xe;
  XMLNode xMainNode = XMLNode::parseString(strXML.c_str(), NULL, &xe);
  
  if(xe.error != 0)
  {
    XBMC->Log(LOG_ERROR, "%s Can't connect to the Recording Service", __FUNCTION__);
    XBMC->QueueNotification(QUEUE_ERROR, XBMC->GetLocalizedString(30500));
    Sleep(10000);
    return false;
  }

  XMLNode xNode = xMainNode.getChildNode("version");

  CStdString strTmp;;

  XBMC->Log(LOG_NOTICE, "%s - DeviceInfo", __FUNCTION__);

  // Get Version
  if (!xNode.getText()) {
    XBMC->Log(LOG_ERROR, "%s Could not parse version from result!", __FUNCTION__);
    return false;
  }
  m_strDVBViewerVersion = xNode.getText();
  CStdString strVersion = m_strDVBViewerVersion.substr(30, 2);
  if (atoi(strVersion) < RS_MIN_VERSION)
  {
    XBMC->Log(LOG_ERROR, "%s - Recording Service version 1.%d or higher required", __FUNCTION__, RS_MIN_VERSION);
    XBMC->QueueNotification(QUEUE_ERROR, XBMC->GetLocalizedString(30501), RS_MIN_VERSION);
    Sleep(10000);
    return false;
  }
  XBMC->Log(LOG_NOTICE, "%s - Version: %s", __FUNCTION__, m_strDVBViewerVersion.c_str());

  return true;
}

PVR_ERROR Dvb::SignalStatus(PVR_SIGNAL_STATUS &signalStatus)
{
  CStdString strXML, url;
  url.Format("%s%s",  m_strURL.c_str(), "status.html?aktion=status"); 
  strXML = GetHttpXML(url);
  unsigned int iSignalStartPos, iSignalEndPos;
  iSignalEndPos = strXML.find("%</th>");

  unsigned int iAdapterStartPos, iAdapterEndPos;
  iAdapterStartPos = strXML.rfind("3\">", iSignalEndPos) + 3;
  iAdapterEndPos = strXML.find("<", iAdapterStartPos);
  strncpy(signalStatus.strAdapterName, strXML.substr(iAdapterStartPos, iAdapterEndPos - iAdapterStartPos).c_str(), sizeof(signalStatus.strAdapterName));

  iSignalStartPos = strXML.find_last_of(">", iSignalEndPos) + 1;
  if (iSignalEndPos < strXML.size())
    signalStatus.iSignal = (int)(atoi(strXML.substr(iSignalStartPos, iSignalEndPos - iSignalStartPos).c_str()) * 655.35);
  strncpy(signalStatus.strAdapterStatus, "OK", sizeof(signalStatus.strAdapterStatus));

  return PVR_ERROR_NO_ERROR;
}
