#include "DvbData.h"
#include "client.h"
#include "platform/util/util.h"

using namespace ADDON;
using namespace PLATFORM;

void Dvb::TimerUpdates()
{
  for (unsigned int i = 0; i < m_timers.size(); i++)
    m_timers[i].iUpdateState = DVB_UPDATE_STATE_NONE;

  std::vector<DvbTimer> newtimer = LoadTimers();
  unsigned int iUpdated = 0;
  unsigned int iUnchanged = 0;
  for (unsigned int j = 0; j < newtimer.size(); j++)
  {
    for (unsigned int i = 0; i < m_timers.size(); i++)
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
  for (unsigned int i = 0; i < m_timers.size(); i++)
  {
    if (m_timers.at(i).iUpdateState == DVB_UPDATE_STATE_NONE)
    {
      XBMC->Log(LOG_DEBUG, "%s Removed timer: '%s', ClientIndex: '%d'",
          __FUNCTION__, m_timers.at(i).strTitle.c_str(), m_timers.at(i).iClientIndex);
      m_timers.erase(m_timers.begin() + i);
      i = 0;
      iRemoved++;
    }
  }

  unsigned int iNew = 0;
  for (unsigned int i = 0; i < newtimer.size(); i++)
  {
    if(newtimer.at(i).iUpdateState == DVB_UPDATE_STATE_NEW)
    {
      DvbTimer &timer = newtimer.at(i);
      timer.iClientIndex = m_iClientIndexCounter;
      XBMC->Log(LOG_DEBUG, "%s New timer: '%s', ClientIndex: '%d'",
          __FUNCTION__, timer.strTitle.c_str(), m_iClientIndexCounter);
      m_timers.push_back(timer);
      m_iClientIndexCounter++;
      iNew++;
    }
  }

  XBMC->Log(LOG_DEBUG, "%s No of timers: removed: %d, untouched: %d, updated: %d, new: %d",
      __FUNCTION__, iRemoved, iUnchanged, iUpdated, iNew);

  if (iRemoved || iUpdated || iNew)
  {
    XBMC->Log(LOG_INFO, "%s Changes in timerlist detected, trigger an update!", __FUNCTION__);
    PVR->TriggerTimerUpdate();
  }
}

Dvb::Dvb()
{
  m_bIsConnected = false;
  m_strServerName = "DVBViewer";
  CStdString strAuth("");

  // simply add user@pass in front of the URL if username/password is set
  if (!g_strUsername.empty() && !g_strPassword.empty())
    strAuth.Format("%s:%s@", g_strUsername, g_strPassword);

  m_strURL.Format("http://%s%s:%u/", strAuth, g_strHostname, g_iPortWeb);
  m_strURLRecording.Format("http://%s%s:%u/", strAuth, g_strHostname, g_iPortRecording);
  m_strURLStream.Format("http://%s%s:%u/", strAuth, g_strHostname, g_iPortStream);

  m_iCurrentChannel     = -1;
  m_iClientIndexCounter = 1;

  m_iUpdateTimer  = 0;
  m_bUpdateTimers = false;
  m_bUpdateEPG    = false;
  m_tsBuffer      = NULL;
}

bool Dvb::Open()
{
  CLockObject lock(m_mutex);

  m_bIsConnected = GetDeviceInfo();
  if (!m_bIsConnected)
    return false;

  if (!LoadChannels())
    return false;

  GetPreferredLanguage();
  GetTimeZone();

  TimerUpdates();

  XBMC->Log(LOG_INFO, "Starting separate client update thread...");
  CreateThread();

  return IsRunning();
}

void  *Dvb::Process()
{
  XBMC->Log(LOG_DEBUG, "%s - starting", __FUNCTION__);

  while (!IsStopped())
  {
    Sleep(1000);
    m_iUpdateTimer++;

    if (m_bUpdateEPG)
    {
      Sleep(8000); /* Sleep enough time to let the recording service grab the EPG data */
      PVR->TriggerEpgUpdate(m_iCurrentChannel);
      m_bUpdateEPG = false;
    }

    if (m_iUpdateTimer > 60 || m_bUpdateTimers)
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
  m_groups.clear();

  CStdString url;
  url.Format("%sapi/getchannelsdat.html", m_strURL);
  CStdString strBIN;
  strBIN = GetHttpXML(url);
  if (strBIN.IsEmpty())
  {
    XBMC->Log(LOG_ERROR, "Unable to load channels.dat");
    return false;
  }
  strBIN.erase(0, CHANNELDAT_HEADER_SIZE);
  ChannelsDat *channel = (ChannelsDat*)strBIN.data();
  int num_channels = strBIN.size() / sizeof(ChannelsDat);

  std::vector<DvbChannel> channels;
  std::vector<DvbChannelGroup> groups;
  DvbChannelGroup channelGroup("");
  int channel_pos = 0;

  for (int i = 0; i < num_channels; i++, channel++)
  {
    if (channel->Flags & ADDITIONAL_AUDIO_TRACK_FLAG)
    {
      if (!g_bUseFavourites || !(channel->Flags & VIDEO_FLAG))
        continue;
    }
    else
      channel_pos++;

    CStdString curGroup(channel->Category, channel->Category_len);
    if (channelGroup.getName() != curGroup)
    {
      char *strGroupNameUtf8 = XBMC->UnknownToUTF8(curGroup);
      channelGroup.setName(strGroupNameUtf8);
      groups.push_back(channelGroup);
      XBMC->FreeString(strGroupNameUtf8);
    }

    DvbChannel dvbChannel;
    /* EPGChannelID = (TunerType + 1)*2^48 + NID*2^32 + TID*2^16 + SID */
    dvbChannel.llEpgId        = ((uint64_t)(channel->TunerType + 1) << 48) + ((uint64_t)channel->OriginalNetwork_ID << 32) + ((uint64_t)channel->TransportStream_ID << 16) + channel->Service_ID;
    dvbChannel.Encrypted      = channel->Flags & ENCRYPTED_FLAG;
    /* ChannelID = (tunertype + 1) * 536870912 + APID * 65536 + SID */
    dvbChannel.iChannelId     = ((channel->TunerType + 1) << 29) + (channel->Audio_PID << 16) + channel->Service_ID;
    dvbChannel.bRadio         = (channel->Flags & VIDEO_FLAG) == 0;
    dvbChannel.group          = channelGroup;
    dvbChannel.iUniqueId      = channels.size() + 1;
    dvbChannel.iChannelNumber = channels.size() + 1;

    CStdString strChannelName(channel->ChannelName, channel->ChannelName_len);
    /* large channels names are distributed at the end of the root + category fields
     * format of root/category: field_data|len of channel substring|channel substring
     */
    if (channel->ChannelName_len == 25)
    {
      if (channel->Root_len < 25)
        strChannelName.append(channel->Root + channel->Root_len + 1, channel->Root[channel->Root_len]);
      if (channel->Category_len < 25)
        strChannelName.append(channel->Category + channel->Category_len + 1, channel->Category[channel->Category_len]);
    }

    char *strChannelNameUtf8 = XBMC->UnknownToUTF8(strChannelName.c_str());
    dvbChannel.strChannelName = strChannelNameUtf8;
    XBMC->FreeString(strChannelNameUtf8);

    CStdString strTmp;
    strTmp.Format("%supnp/channelstream/%d.ts", m_strURLStream, channel_pos - 1);
    dvbChannel.strStreamURL = strTmp;

    strTmp.Format("%sLogos/%s.png", m_strURL, URLEncodeInline(dvbChannel.strChannelName));
    dvbChannel.strIconPath = strTmp;

    channels.push_back(dvbChannel);
  }

  if (g_bUseFavourites)
  {
    CStdString urlFav(g_strFavouritesPath);
    if (!XBMC->FileExists(urlFav, false))
      urlFav.Format("%sapi/getfavourites.html", m_strURL);

    CStdString strXML(GetHttpXML(urlFav));
    if (strXML.empty())
    {
      XBMC->Log(LOG_ERROR, "Unable to load favourites.xml.");
      return false;
    }

    RemoveNullChars(strXML);
    XMLResults xe;
    XMLNode xMainNode = XMLNode::parseString(strXML, NULL, &xe);
    if (xe.error != 0)
    {
      XBMC->Log(LOG_ERROR, "Unable to parse favourites.xml. Error: %s", XMLNode::getError(xe.error));
      return false;
    }

    XMLNode xNode = xMainNode.getChildNode("settings");
    int m = xNode.nChildNode("section");

    std::vector<DvbChannel> channelsfav;
    std::vector<DvbChannelGroup> groupsfav;
    DvbChannel favChannel;
    DvbChannelGroup favGroup("");

    for (int i = 0; i < m; i++)
    {
      XMLNode xTmp = xNode.getChildNode("section", i);
      int n = xTmp.nChildNode("entry");
      for (int j = 0; j < n; j++)
      {
        CStdString strTmp(xTmp.getChildNode("entry", j).getText());
        uint64_t llFavId;
        if (sscanf(strTmp, "%lu|", &llFavId))
        {
          int iChannelId = llFavId & 0xFFFFFFFF;
          for (unsigned int i = 0; i < channels.size(); i++)
          {
            if (channels[i].iChannelId == iChannelId)
            {
              favChannel.llEpgId        = channels[i].llEpgId;
              favChannel.Encrypted      = channels[i].Encrypted;
              favChannel.iChannelId     = channels[i].iChannelId;
              favChannel.bRadio         = channels[i].bRadio;
              favChannel.group          = favGroup;
              favChannel.iUniqueId      = channelsfav.size() + 1;
              favChannel.iChannelNumber = channelsfav.size() + 1;
              favChannel.strChannelName = channels[i].strChannelName;
              favChannel.strStreamURL   = channels[i].strStreamURL;
              favChannel.strIconPath    = channels[i].strIconPath;
              channelsfav.push_back(favChannel);
              break;
            }
          }
        }
        else
        {
          /* it's a group */
          char *strGroupNameUtf8 = XBMC->UnknownToUTF8(strTmp.c_str());
          favGroup.setName(strGroupNameUtf8);
          groupsfav.push_back(favGroup);
          XBMC->FreeString(strGroupNameUtf8);
        }
      }
    }
    m_channels = channelsfav;
    m_groups = groupsfav;
  }
  else
  {
    m_channels = channels;
    m_groups = groups;
  }

  XBMC->Log(LOG_INFO, "Loaded %d channels in %d groups", m_channels.size(), m_groups.size());
  return true;
}

bool Dvb::IsConnected()
{
  return m_bIsConnected;
}

CStdString Dvb::GetHttpXML(CStdString& url)
{
  CStdString strResult;
  void* fileHandle = XBMC->OpenFile(url, 0);
  if (fileHandle)
  {
    char buffer[1024];
    while (int bytesRead = XBMC->ReadFile(fileHandle, buffer, 1024))
      strResult.append(buffer, bytesRead);
    XBMC->CloseFile(fileHandle);
  }
  return strResult;
}

const char *Dvb::GetServerName()
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

unsigned int Dvb::GetRecordingsAmount()
{
  return m_recordings.size();
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
      xbmcChannel.iEncryptionSystem = channel.Encrypted;
      xbmcChannel.bIsHidden         = false;
      PVR_STRCPY(xbmcChannel.strChannelName, channel.strChannelName.c_str());
      PVR_STRCPY(xbmcChannel.strInputFormat, ""); // unused
      PVR_STRCPY(xbmcChannel.strIconPath,    channel.strIconPath.c_str());

      if (!g_bUseTimeshift)
      {
        CStdString strStream;
        strStream.Format("pvr://stream/tv/%i.ts", channel.iUniqueId);
        PVR_STRCPY(xbmcChannel.strStreamURL, strStream.c_str()); //channel.strStreamURL.c_str();
      }

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
  if (m_tsBuffer)
    SAFE_DELETE(m_tsBuffer);
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
  url.Format("%sapi/epg.html?lvl=2&channel=%lld&start=%f&end=%f", m_strURL, myChannel.llEpgId, iStart/86400.0 + DELPHI_DATE, iEnd/86400.0 + DELPHI_DATE);

  CStdString strXML(GetHttpXML(url));

  XMLResults xe;
  XMLNode xMainNode = XMLNode::parseString(strXML, NULL, &xe);
  if (xe.error != 0)
  {
    XBMC->Log(LOG_ERROR, "Unable to parse EPG. Invalid XML. Error: %s", XMLNode::getError(xe.error));
    return PVR_ERROR_SERVER_ERROR;
  }

  XMLNode xNode = xMainNode.getChildNode("epg");
  int n = xNode.nChildNode("programme");

  XBMC->Log(LOG_DEBUG, "%s Number of elements: '%d'", __FUNCTION__, n);

  int iNumEPG = 0;
  for (int i = 0; i < n; i++)
  {
    CStdString strTmp;
    int iTmpStart;
    int iTmpEnd;

    XMLNode xTmp = xNode.getChildNode("programme", i);
    iTmpStart = ParseDateTime(xNode.getChildNode("programme", i).getAttribute("start"));
    iTmpEnd = ParseDateTime(xNode.getChildNode("programme", i).getAttribute("stop"));

    if (iEnd > 1 && iEnd < iTmpEnd)
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
    broadcast.iGenreType          = 0;  // unused
    broadcast.iGenreSubType       = 0;  // unused
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

    XBMC->Log(LOG_DEBUG, "%s loaded EPG entry '%d:%s' channel '%d' start '%d' end '%d'",
        __FUNCTION__, broadcast.iUniqueBroadcastId, broadcast.strTitle, entry.iChannelId, entry.startTime, entry.endTime);
  }

  XBMC->Log(LOG_DEBUG, "%s Loaded %u EPG Entries for channel '%s'", __FUNCTION__, iNumEPG, channel.strChannelName);
  return PVR_ERROR_NO_ERROR;
}

int Dvb::GetChannelNumber(CStdString strChannelId)
{
  int channel;
  sscanf(strChannelId.c_str(), "%d|", &channel);
  for (unsigned int i = 0; i < m_channels.size();  i++)
  {
    if (m_channels[i].iChannelId == channel)
      return i + 1;
  }
  return -1;
}

PVR_ERROR Dvb::GetTimers(ADDON_HANDLE handle)
{
  XBMC->Log(LOG_INFO, "%s - timers available '%d'", __FUNCTION__, m_timers.size());
  for (unsigned int i = 0; i < m_timers.size(); i++)
  {
    DvbTimer &timer = m_timers.at(i);
    XBMC->Log(LOG_INFO, "%s - Transfer timer '%s', ClientIndex '%d'",
        __FUNCTION__, timer.strTitle.c_str(), timer.iClientIndex);
    PVR_TIMER tag;
    memset(&tag, 0, sizeof(PVR_TIMER));
    tag.iClientChannelUid = timer.iChannelId;
    tag.startTime         = timer.startTime;
    tag.endTime           = timer.endTime;
    PVR_STRCPY(tag.strTitle,     timer.strTitle.c_str());
    PVR_STRCPY(tag.strDirectory, "/");   // unused
    PVR_STRCPY(tag.strSummary,   timer.strPlot.c_str());
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
  std::vector<DvbTimer> timers;

  CStdString url;
  url.Format("%s%s", m_strURL, "api/timerlist.html?utf8");

  CStdString strXML(GetHttpXML(url));
  RemoveNullChars(strXML);

  XMLResults xe;
  XMLNode xMainNode = XMLNode::parseString(strXML, NULL, &xe);
  if (xe.error != 0)
  {
    XBMC->Log(LOG_ERROR, "Unable to parse timers. Invalid XML. Error: %s", XMLNode::getError(xe.error));
    return timers;
  }

  XMLNode xNode = xMainNode.getChildNode("Timers");
  int n = xNode.nChildNode("Timer");

  XBMC->Log(LOG_INFO, "%s Number of elements: '%d'", __FUNCTION__, n);

  while (n > 0)
  {
    int i = n - 1;
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
    for (unsigned int i = 0; i < Weekdays.size(); i++)
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
      if (iTmp == -1)
        timer.state = PVR_TIMER_STATE_RECORDING;
    }

    if (GetInt(xTmp, "ID", iTmp))
      timer.iTimerID = iTmp;

    timers.push_back(timer);

    XBMC->Log(LOG_INFO, "%s fetched Timer entry '%s', begin '%d', end '%d'",
        __FUNCTION__, timer.strTitle.c_str(), timer.startTime, timer.endTime);
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

  struct tm *timeinfo;
  time_t startTime = timer.startTime, endTime = timer.endTime;
  if (!startTime)
    time(&startTime);
  else
  {
    startTime -= timer.iMarginStart*60;
    endTime += timer.iMarginEnd*60;
  }

  int dor = ((startTime + m_iTimezone*60) / DAY_SECS) + DELPHI_DATE;
  timeinfo = localtime(&startTime);
  int start = timeinfo->tm_hour*60 + timeinfo->tm_min;
  timeinfo = localtime(&endTime);
  int stop = timeinfo->tm_hour*60 + timeinfo->tm_min;

  char strWeek[8] = "-------";
  for (int i = 0; i < 7; i++)
  {
    if (timer.iWeekdays & (1 << i))
      strWeek[i] = 'T';
  }

  int iChannelId = m_channels.at(timer.iClientChannelUid-1).iChannelId;
  CStdString strTmp;
  if (bNewTimer)
    strTmp.Format("api/timeradd.html?ch=%d&dor=%d&enable=1&start=%d&stop=%d&prio=%d&days=%s&title=%s&encoding=255",
        iChannelId, dor, start, stop, timer.iPriority, strWeek, URLEncodeInline(timer.strTitle));
  else
  {
    int enabled = 1;
    if (timer.state == PVR_TIMER_STATE_CANCELLED)
      enabled = 0;
    strTmp.Format("api/timeredit.html?id=%d&ch=%d&dor=%d&enable=%d&start=%d&stop=%d&prio=%d&days=%s&title=%s&encoding=255",
        GetTimerID(timer), iChannelId, dor, enabled, start, stop, timer.iPriority, strWeek, URLEncodeInline(timer.strTitle));
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
  std::vector<DvbRecording> recthumbs;
  recthumbs = m_recordings;
  m_recordings.clear();

  CStdString url;
  url.Format("%s%s", m_strURL, "api/recordings.html?utf8");

  CStdString strXML(GetHttpXML(url));
  RemoveNullChars(strXML);

  XMLResults xe;
  XMLNode xMainNode = XMLNode::parseString(strXML, NULL, &xe);
  if (xe.error != 0)
  {
    XBMC->Log(LOG_ERROR, "Unable to parse recordings. Invalid XML. Error: %s", XMLNode::getError(xe.error));
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

    strTmp.Format("%supnp/recordings/%d.ts", m_strURLRecording, atoi(recording.strRecordingId.c_str()));
    recording.strStreamURL = strTmp;

    recording.startTime = ParseDateTime(xNode.getChildNode("recording", i).getAttribute("start"));

    int hours, mins, secs;
    sscanf(xNode.getChildNode("recording", i).getAttribute("duration"), "%02d%02d%02d", &hours, &mins, &secs);
    recording.iDuration = hours*60*60 + mins*60 + secs;

    bool bGetThumbnails = true;
    if (iGetRecordingsCount == 0 && j > MAX_RECORDING_THUMBS - 1)
      bGetThumbnails = false;

    for (unsigned int i = 0; i < recthumbs.size(); i++)
    {
      if (recthumbs[i].strRecordingId == recording.strRecordingId
          && recthumbs[i].strThumbnailPath.size() > 20
          && recthumbs[i].strThumbnailPath.size() < 100)
      {
        recording.strThumbnailPath = recthumbs[i].strThumbnailPath;
        bGetThumbnails = false;
        break;
      }
    }

    if (bGetThumbnails)
    {
      url.Format("%sepg_details.html?aktion=epg_details&recID=%s", m_strURL, recording.strRecordingId);
      CStdString strThumb;
      strThumb = GetHttpXML(url);

      unsigned int iThumbnailPos;
      iThumbnailPos = strThumb.find_first_of('_', RECORDING_THUMB_POS);
      strTmp.Format("%sthumbnails/video/%s_SM.jpg", m_strURL, strThumb.substr(RECORDING_THUMB_POS, iThumbnailPos - RECORDING_THUMB_POS));
      recording.strThumbnailPath = strTmp;
    }

    PVR_RECORDING tag;
    memset(&tag, 0, sizeof(PVR_RECORDING));
    PVR_STRCPY(tag.strRecordingId,   recording.strRecordingId.c_str());
    PVR_STRCPY(tag.strTitle,         recording.strTitle.c_str());
    PVR_STRCPY(tag.strStreamURL,     recording.strStreamURL.c_str());
    PVR_STRCPY(tag.strPlotOutline,   recording.strPlotOutline.c_str());
    PVR_STRCPY(tag.strPlot,          recording.strPlot.c_str());
    PVR_STRCPY(tag.strChannelName,   recording.strChannelName.c_str());
    PVR_STRCPY(tag.strThumbnailPath, recording.strThumbnailPath.c_str());
    tag.recordingTime = recording.startTime;
    tag.iDuration     = recording.iDuration;
    PVR_STRCPY(tag.strDirectory, "/"); // unused

    PVR->TransferRecordingEntry(handle, &tag);

    iNumRecording++;
    m_recordings.push_back(recording);

    XBMC->Log(LOG_DEBUG, "%s loaded Recording entry '%s', start '%d', length '%d'", __FUNCTION__, tag.strTitle, recording.startTime, recording.iDuration);
  }
  iGetRecordingsCount++;

  XBMC->Log(LOG_INFO, "Loaded %u Recording Entries", iNumRecording);

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
  url.Format("%s%s", m_strURL, strCommandURL);
  GetHttpXML(url);
}

CStdString Dvb::URLEncodeInline(const CStdString& strData)
{
  /* Copied from xbmc/URL.cpp */
  CStdString strResult;

  /* wonder what a good value is here is, depends on how often it occurs */
  strResult.reserve(strData.length() * 2);

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
    for (int i = 0; i < n; i++)
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

void Dvb::GetPreferredLanguage()
{
  CStdString strXML, url;
  url.Format("%s%s", m_strURL, "config.html?aktion=config");
  strXML = GetHttpXML(url);
  unsigned int iLanguagePos = strXML.find("EPGLanguage");
  iLanguagePos = strXML.find(" selected>", iLanguagePos);
  m_strEPGLanguage = strXML.substr(iLanguagePos-4, 3);
}

void Dvb::GetTimeZone()
{
  CStdString url;
  url.Format("%s%s", m_strURL, "api/status.html");

  CStdString strXML(GetHttpXML(url));

  XMLResults xe;
  XMLNode xMainNode = XMLNode::parseString(strXML, NULL, &xe);
  if (xe.error != 0)
    XBMC->Log(LOG_ERROR, "Unable to get recording service status. Invalid XML. Error: %s", XMLNode::getError(xe.error));
  else
  {
    XMLNode xNode = xMainNode.getChildNode("status");
    GetInt(xNode, "timezone", m_iTimezone);
  }
}

void Dvb::RemoveNullChars(CStdString &String)
{
  /* favourites.xml and timers.xml sometimes have null chars that screw the xml */
  for (unsigned int i = 0; i < String.size()-1; i++)
  {
    if (String.data()[i] == '\0')
    {
      String.erase(i, 1);
      i--;
    }
  }
}

PVR_ERROR Dvb::GetChannelGroups(ADDON_HANDLE handle)
{
  for (unsigned int i = 0; i < m_groups.size(); i++)
  {
    PVR_CHANNEL_GROUP tag;
    memset(&tag, 0, sizeof(PVR_CHANNEL_GROUP));
    tag.bIsRadio = false;
    PVR_STRCPY(tag.strGroupName, m_groups[i].getName().c_str());

    PVR->TransferChannelGroup(handle, &tag);
  }

  return PVR_ERROR_NO_ERROR;
}

unsigned int Dvb::GetNumChannelGroups()
{
  return m_groups.size();
}

PVR_ERROR Dvb::GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group)
{
  CStdString groupName(group.strGroupName);
  for (unsigned int i = 0; i < m_channels.size(); i++)
  {
    DvbChannel &channel = m_channels.at(i);
    if (channel.group == groupName)
    {
      PVR_CHANNEL_GROUP_MEMBER tag;
      memset(&tag, 0, sizeof(PVR_CHANNEL_GROUP_MEMBER));
      PVR_STRCPY(tag.strGroupName, group.strGroupName);
      tag.iChannelUniqueId = channel.iUniqueId;
      tag.iChannelNumber   = channel.iChannelNumber;

      XBMC->Log(LOG_DEBUG, "%s - add channel %s (%d) to group '%s' channel number %d",
          __FUNCTION__, channel.strChannelName.c_str(), tag.iChannelUniqueId, group.strGroupName, channel.iChannelNumber);

      PVR->TransferChannelGroupMember(handle, &tag);
    }
  }
  return PVR_ERROR_NO_ERROR;
}

int Dvb::GetCurrentClientChannel(void)
{
  return m_iCurrentChannel;
}

const char *Dvb::GetLiveStreamURL(const PVR_CHANNEL &channelinfo)
{
  SwitchChannel(channelinfo);

  return m_channels.at(channelinfo.iUniqueId-1).strStreamURL.c_str();
}

bool Dvb::OpenLiveStream(const PVR_CHANNEL &channelinfo)
{
  XBMC->Log(LOG_INFO, "%s channel '%u'", __FUNCTION__, channelinfo.iUniqueId);

  if ((int)channelinfo.iUniqueId == m_iCurrentChannel)
    return true;

  if (!g_bUseTimeshift)
    return SwitchChannel(channelinfo);

  if (m_tsBuffer)
    SAFE_DELETE(m_tsBuffer);
  XBMC->Log(LOG_INFO, "%s ts buffer starts url=%s", __FUNCTION__, GetLiveStreamURL(channelinfo));
  m_tsBuffer = new TimeshiftBuffer(GetLiveStreamURL(channelinfo), g_strTimeshiftBufferPath);
  return m_tsBuffer->IsValid();
}

void Dvb::CloseLiveStream(void)
{
  m_iCurrentChannel = -1;
  if (m_tsBuffer)
    SAFE_DELETE(m_tsBuffer);
}

bool Dvb::SwitchChannel(const PVR_CHANNEL &channel)
{
  m_iCurrentChannel = channel.iUniqueId;
  m_bUpdateEPG = true;
  return true;
}

int Dvb::ReadLiveStream(unsigned char *pBuffer, unsigned int iBufferSize)
{
  if (!m_tsBuffer)
    return 0;
  return m_tsBuffer->ReadData(pBuffer, iBufferSize);
}

long long Dvb::SeekLiveStream(long long iPosition, int iWhence /* = SEEK_SET */)
{
  if (!m_tsBuffer)
    return 0;
  return m_tsBuffer->Seek(iPosition, iWhence);
}

long long Dvb::PositionLiveStream(void)
{
  if (!m_tsBuffer)
    return 0;
  return m_tsBuffer->Position();
}

long long Dvb::LengthLiveStream(void)
{
  if (!m_tsBuffer)
    return 0;
  return m_tsBuffer->Length();
}

bool Dvb::GetDeviceInfo()
{
  CStdString url;
  url.Format("%s%s", m_strURL, "api/version.html");

  CStdString strXML(GetHttpXML(url));

  XMLResults xe;
  XMLNode xMainNode = XMLNode::parseString(strXML, NULL, &xe);
  if (xe.error != 0)
  {
    XBMC->Log(LOG_ERROR, "Unable to connect to the Recording Service");
    XBMC->QueueNotification(QUEUE_ERROR, XBMC->GetLocalizedString(30500));
    Sleep(10000);
    return false;
  }

  // Get Version
  XBMC->Log(LOG_NOTICE, "Fetching deviceInfo...");
  XMLNode xNode = xMainNode.getChildNode("version");
  if (!xNode.getText()) {
    XBMC->Log(LOG_ERROR, "Could not parse version from result!");
    return false;
  }
  m_strDVBViewerVersion = xNode.getText();
  CStdString strVersion = m_strDVBViewerVersion.substr(30, 2);
  if (atoi(strVersion) < RS_MIN_VERSION)
  {
    XBMC->Log(LOG_ERROR, "Recording Service version 1.%d or higher required", RS_MIN_VERSION);
    XBMC->QueueNotification(QUEUE_ERROR, XBMC->GetLocalizedString(30501), RS_MIN_VERSION);
    Sleep(10000);
    return false;
  }
  XBMC->Log(LOG_NOTICE, "Version: %s", m_strDVBViewerVersion.c_str());

  return true;
}

PVR_ERROR Dvb::SignalStatus(PVR_SIGNAL_STATUS &signalStatus)
{
  CStdString strXML, url;
  url.Format("%s%s", m_strURL, "status.html?aktion=status"); 
  strXML = GetHttpXML(url);
  unsigned int iSignalStartPos, iSignalEndPos;
  iSignalEndPos = strXML.find("%</th>");

  unsigned int iAdapterStartPos, iAdapterEndPos;
  iAdapterStartPos = strXML.rfind("3\">", iSignalEndPos) + 3;
  iAdapterEndPos = strXML.find("<", iAdapterStartPos);
  PVR_STRCPY(signalStatus.strAdapterName, strXML.substr(iAdapterStartPos, iAdapterEndPos - iAdapterStartPos).c_str());

  iSignalStartPos = strXML.find_last_of(">", iSignalEndPos) + 1;
  if (iSignalEndPos < strXML.size())
    signalStatus.iSignal = (int)(atoi(strXML.substr(iSignalStartPos, iSignalEndPos - iSignalStartPos).c_str()) * 655.35);
  PVR_STRCPY(signalStatus.strAdapterStatus, "OK");

  return PVR_ERROR_NO_ERROR;
}
