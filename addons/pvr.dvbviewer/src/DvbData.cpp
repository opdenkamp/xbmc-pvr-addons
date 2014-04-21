#include "DvbData.h"
#include "client.h"
#include "platform/util/util.h"
#include "tinyxml/tinyxml.h"
#include "tinyxml/XMLUtils.h"
#include <inttypes.h>
#include <set>
#include <iterator>
#include <sstream>
#include <algorithm>

using namespace ADDON;
using namespace PLATFORM;

template <class ContainerT>
void tokenize(const CStdString& str, ContainerT& tokens,
    const CStdString& delimiters = " ", const bool trimEmpty = false)
{
  CStdString::size_type pos, lastPos = 0;
  while (true)
  {
    pos = str.find_first_of(delimiters, lastPos);
    if (pos == CStdString::npos)
    {
      pos = str.length();

      if (pos != lastPos || !trimEmpty)
        tokens.push_back(typename ContainerT::value_type(str.data() + lastPos, pos - lastPos));
      break;
    }
    else
    {
      if (pos != lastPos || !trimEmpty)
        tokens.push_back(typename ContainerT::value_type(str.data() + lastPos, pos - lastPos));
    }
    lastPos = pos + 1;
  }
};


Dvb::Dvb()
  : m_connected(false), m_backendVersion(0)
{
  // simply add user@pass in front of the URL if username/password is set
  CStdString auth("");
  if (!g_username.empty() && !g_password.empty())
    auth.Format("%s:%s@", g_username.c_str(), g_password.c_str());
  m_url.Format("http://%s%s:%u/", auth.c_str(), g_hostname.c_str(), g_webPort);

  m_currentChannel = 0;
  m_newTimerIndex  = 1;

  m_updateTimers = false;
  m_updateEPG    = false;
  m_tsBuffer     = NULL;
}

Dvb::~Dvb()
{
  StopThread();
  if (m_tsBuffer)
    SAFE_DELETE(m_tsBuffer);

  for (DvbChannels_t::iterator channel = m_channels.begin();
      channel != m_channels.end(); ++channel)
    delete *channel;
}

bool Dvb::Open()
{
  CLockObject lock(m_mutex);

  m_connected = CheckBackendVersion();
  if (!m_connected)
    return false;

  if (!UpdateBackendStatus(true))
    return false;

  if (!LoadChannels())
    return false;

  TimerUpdates();
  // force recording sync as XBMC won't update recordings on PVR restart
  PVR->TriggerRecordingUpdate();

  XBMC->Log(LOG_INFO, "Starting separate polling thread...");
  CreateThread();

  return IsRunning();
}

bool Dvb::IsConnected()
{
  return m_connected;
}

CStdString Dvb::GetBackendName()
{
  // RS api doesn't provide a reliable way to extract the server name
  return "DVBViewer";
}

CStdString Dvb::GetBackendVersion()
{
  CStdString version;
  version.Format("%u.%u.%u.%u",
      m_backendVersion >> 24 & 0xFF,
      m_backendVersion >> 16 & 0xFF,
      m_backendVersion >> 8 & 0xFF,
      m_backendVersion & 0xFF);
  return version;
}

PVR_ERROR Dvb::GetDriveSpace(long long *total, long long *used)
{
  if (!UpdateBackendStatus())
    return PVR_ERROR_SERVER_ERROR;
  *total = m_diskspace.total;
  *used  = m_diskspace.used;
  return PVR_ERROR_NO_ERROR;
}

bool Dvb::SwitchChannel(const PVR_CHANNEL& channel)
{
  m_currentChannel = channel.iUniqueId;
  m_updateEPG = true;
  return true;
}

unsigned int Dvb::GetCurrentClientChannel(void)
{
  return m_currentChannel;
}

PVR_ERROR Dvb::GetChannels(ADDON_HANDLE handle, bool bRadio)
{
  for (DvbChannels_t::iterator it = m_channels.begin();
      it != m_channels.end(); ++it)
  {
    DvbChannel *channel = *it;
    if (channel->hidden)
      continue;
    if (channel->radio != bRadio)
      continue;

    PVR_CHANNEL xbmcChannel;
    memset(&xbmcChannel, 0, sizeof(PVR_CHANNEL));
    xbmcChannel.iUniqueId         = channel->id;
    xbmcChannel.bIsRadio          = channel->radio;
    xbmcChannel.iChannelNumber    = channel->frontendNr;
    xbmcChannel.iEncryptionSystem = channel->encrypted;
    xbmcChannel.bIsHidden         = false;
    PVR_STRCPY(xbmcChannel.strChannelName, channel->name.c_str());
    PVR_STRCPY(xbmcChannel.strIconPath,    channel->logoURL.c_str());

    if (!channel->radio && !g_useRTSP)
      PVR_STRCPY(xbmcChannel.strInputFormat, "video/x-mpegts");
    else
      PVR_STRCPY(xbmcChannel.strInputFormat, "");

    if (!g_useTimeshift)
    {
      // self referencing so GetLiveStreamURL() gets triggered
      CStdString streamURL;
      streamURL.Format("pvr://stream/tv/%u.ts", channel->backendNr);
      PVR_STRCPY(xbmcChannel.strStreamURL, streamURL.c_str());
    }

    PVR->TransferChannelEntry(handle, &xbmcChannel);
  }

  return PVR_ERROR_NO_ERROR;
}

//TODO: rewrite
//TODO: missing epg v2 - there's no documention
PVR_ERROR Dvb::GetEPGForChannel(ADDON_HANDLE handle,
  const PVR_CHANNEL& channel, time_t iStart, time_t iEnd)
{
  DvbChannel *myChannel = m_channels[channel.iUniqueId - 1];

  CStdString url = BuildURL("api/epg.html?lvl=2&channel=%"PRIu64"&start=%f&end=%f",
      myChannel->epgId, iStart/86400.0 + DELPHI_DATE, iEnd/86400.0 + DELPHI_DATE);
  CStdString req = GetHttpXML(url);

  TiXmlDocument doc;
  doc.Parse(req);
  if (doc.Error())
  {
    XBMC->Log(LOG_ERROR, "Unable to parse EPG. Error: %s",
        doc.ErrorDesc());
    return PVR_ERROR_SERVER_ERROR;
  }

  unsigned iNumEPG = 0;
  for (TiXmlElement *xEntry = doc.RootElement()->FirstChildElement("programme");
      xEntry; xEntry = xEntry->NextSiblingElement("programme"))
  {
    DvbEPGEntry entry;
    entry.iChannelUid = channel.iUniqueId;
    entry.startTime = ParseDateTime(xEntry->Attribute("start"));
    entry.endTime = ParseDateTime(xEntry->Attribute("stop"));

    if (iEnd > 1 && iEnd < entry.endTime)
       continue;

    if (!XMLUtils::GetInt(xEntry, "eventid", entry.iEventId))
      continue;

    // since RS 1.26.0 the correct language is already merged into the elements
    TiXmlNode *xTitles = xEntry->FirstChild("titles");
    if (!xTitles || !XMLUtils::GetString(xTitles, "title", entry.strTitle))
      continue;

    TiXmlNode *xDescriptions = xEntry->FirstChild("descriptions");
    if (xDescriptions)
      XMLUtils::GetString(xDescriptions, "description", entry.strPlot);

    TiXmlNode *xEvents = xEntry->FirstChild("events");
    if (xEvents)
    {
      XMLUtils::GetString(xEvents, "event", entry.strPlotOutline);
      if (!entry.strPlotOutline.empty() && entry.strPlot.empty())
        entry.strPlot = entry.strPlotOutline;
    }

    XMLUtils::GetUInt(xEntry, "content", entry.genre);

    EPG_TAG broadcast;
    memset(&broadcast, 0, sizeof(EPG_TAG));
    broadcast.iUniqueBroadcastId  = entry.iEventId;
    broadcast.strTitle            = entry.strTitle.c_str();
    broadcast.iChannelNumber      = channel.iChannelNumber;
    broadcast.startTime           = entry.startTime;
    broadcast.endTime             = entry.endTime;
    broadcast.strPlotOutline      = entry.strPlotOutline.c_str();
    broadcast.strPlot             = entry.strPlot.c_str();
    broadcast.iGenreType          = entry.genre & 0xF0;
    broadcast.iGenreSubType       = entry.genre & 0x0F;

    PVR->TransferEpgEntry(handle, &broadcast);
    ++iNumEPG;

    XBMC->Log(LOG_DEBUG, "%s loaded EPG entry '%u:%s': start=%u, end=%u",
        __FUNCTION__, broadcast.iUniqueBroadcastId, broadcast.strTitle,
        entry.startTime, entry.endTime);
  }

  XBMC->Log(LOG_INFO, "Loaded %u EPG entries for channel '%s'",
      iNumEPG, channel.strChannelName);
  return PVR_ERROR_NO_ERROR;
}

unsigned int Dvb::GetChannelsAmount()
{
  return m_channelAmount;
}


PVR_ERROR Dvb::GetChannelGroups(ADDON_HANDLE handle, bool bRadio)
{
  for (DvbGroups_t::iterator group = m_groups.begin();
      group != m_groups.end(); ++group)
  {
    if (group->hidden)
      continue;
    if (group->radio != bRadio)
      continue;

    PVR_CHANNEL_GROUP tag;
    memset(&tag, 0, sizeof(PVR_CHANNEL_GROUP));
    tag.bIsRadio = group->radio;
    PVR_STRCPY(tag.strGroupName, group->name.c_str());

    PVR->TransferChannelGroup(handle, &tag);
  }

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR Dvb::GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP& pvrGroup)
{
  unsigned int channelNumberInGroup = 1;

  for (DvbGroups_t::iterator group = m_groups.begin();
      group != m_groups.end(); ++group)
  {
    if (group->name != pvrGroup.strGroupName)
      continue;

    for (std::list<DvbChannel *>::iterator it = group->channels.begin();
        it != group->channels.end(); ++it)
    {
      DvbChannel *channel = *it;

      PVR_CHANNEL_GROUP_MEMBER tag;
      memset(&tag, 0, sizeof(PVR_CHANNEL_GROUP_MEMBER));
      PVR_STRCPY(tag.strGroupName, pvrGroup.strGroupName);
      tag.iChannelUniqueId = channel->id;
      tag.iChannelNumber   = channelNumberInGroup++;

      PVR->TransferChannelGroupMember(handle, &tag);

      XBMC->Log(LOG_DEBUG, "%s add channel '%s' (%u) to group '%s'",
          __FUNCTION__, channel->name.c_str(), channel->backendNr,
          group->name.c_str());
    }
  }
  return PVR_ERROR_NO_ERROR;
}

unsigned int Dvb::GetChannelGroupsAmount()
{
  return m_groupAmount;
}


PVR_ERROR Dvb::GetTimers(ADDON_HANDLE handle)
{
  for (DvbTimers_t::iterator timer = m_timers.begin();
      timer != m_timers.end(); ++timer)
  {
    PVR_TIMER tag;
    memset(&tag, 0, sizeof(PVR_TIMER));
    PVR_STRCPY(tag.strTitle,   timer->strTitle.c_str());
    PVR_STRCPY(tag.strSummary, timer->strPlot.c_str());
    tag.iClientChannelUid = timer->iChannelUid;
    tag.startTime         = timer->startTime;
    tag.endTime           = timer->endTime;
    tag.state             = timer->state;
    tag.iPriority         = timer->iPriority;
    tag.bIsRepeating      = timer->bRepeating;
    tag.firstDay          = timer->iFirstDay;
    tag.iWeekdays         = timer->iWeekdays;
    tag.iEpgUid           = timer->iEpgId;
    tag.iClientIndex      = timer->iClientIndex;

    PVR->TransferTimerEntry(handle, &tag);
  }
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR Dvb::AddTimer(const PVR_TIMER& timer)
{
  GenerateTimer(timer);
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR Dvb::UpdateTimer(const PVR_TIMER& timer)
{
  GenerateTimer(timer, false);
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR Dvb::DeleteTimer(const PVR_TIMER& timer)
{
  GetHttpXML(BuildURL("api/timerdelete.html?id=%d", GetTimerId(timer)));

  if (timer.state == PVR_TIMER_STATE_RECORDING)
    PVR->TriggerRecordingUpdate();

  m_updateTimers = true;
  return PVR_ERROR_NO_ERROR;
}

unsigned int Dvb::GetTimersAmount()
{
  return m_timers.size();
}


PVR_ERROR Dvb::GetRecordings(ADDON_HANDLE handle)
{
  CStdString url = BuildURL("api/recordings.html?images=1");
  CStdString req = GetHttpXML(url);
  RemoveNullChars(req);

  TiXmlDocument doc;
  doc.Parse(req);
  if (doc.Error())
  {
    XBMC->Log(LOG_ERROR, "Unable to parse recordings. Error: %s",
        doc.ErrorDesc());
    return PVR_ERROR_SERVER_ERROR;
  }

  CStdString streamURL, imageURL;
  TiXmlElement *root = doc.RootElement();
  XMLUtils::GetString(root, "serverURL", streamURL);
  XMLUtils::GetString(root, "imageURL",  imageURL);

  // there's no need to merge new recordings in older ones as XBMC does this
  // already for us (using strRecordingId). so just parse all recordings again
  m_recordingAmount = 0;

  // insert recordings in reverse order
  for (TiXmlNode *xNode = root->LastChild("recording");
      xNode; xNode = xNode->PreviousSibling("recording"))
  {
    if (!xNode->ToElement())
      continue;

    TiXmlElement *xRecording = xNode->ToElement();

    DvbRecording recording;
    recording.id = xRecording->Attribute("id");
    xRecording->QueryUnsignedAttribute("content", &recording.genre);
    XMLUtils::GetString(xRecording, "channel", recording.channelName);
    XMLUtils::GetString(xRecording, "title",   recording.title);
    XMLUtils::GetString(xRecording, "info",    recording.plotOutline);
    XMLUtils::GetString(xRecording, "desc",    recording.plot);
    if (recording.plot.empty())
      recording.plot = recording.plotOutline;

    recording.streamURL = BuildExtURL(streamURL, "%s.ts", recording.id.c_str());

    CStdString thumbnail;
    if (!g_lowPerformance && XMLUtils::GetString(xRecording, "image", thumbnail))
      recording.thumbnailPath = BuildExtURL(imageURL, "%s", thumbnail.c_str());

    CStdString startTime = xRecording->Attribute("start");
    recording.startTime = ParseDateTime(startTime);

    int hours, mins, secs;
    sscanf(xRecording->Attribute("duration"), "%02d%02d%02d", &hours, &mins, &secs);
    recording.duration = hours*60*60 + mins*60 + secs;

    // generate a more unique id
    recording.id += "_" + startTime;

    PVR_RECORDING tag;
    memset(&tag, 0, sizeof(PVR_RECORDING));
    PVR_STRCPY(tag.strRecordingId,   recording.id.c_str());
    PVR_STRCPY(tag.strTitle,         recording.title.c_str());
    PVR_STRCPY(tag.strStreamURL,     recording.streamURL.c_str());
    PVR_STRCPY(tag.strPlotOutline,   recording.plotOutline.c_str());
    PVR_STRCPY(tag.strPlot,          recording.plot.c_str());
    PVR_STRCPY(tag.strChannelName,   recording.channelName.c_str());
    PVR_STRCPY(tag.strThumbnailPath, recording.thumbnailPath.c_str());
    tag.recordingTime = recording.startTime;
    tag.iDuration     = recording.duration;
    tag.iGenreType    = recording.genre & 0xF0;
    tag.iGenreSubType = recording.genre & 0x0F;

    CStdString tmp;
    switch(g_groupRecordings)
    {
      case DvbRecording::GroupByDirectory:
        XMLUtils::GetString(xRecording, "file", tmp);
        tmp.ToLower();
        for (std::vector<CStdString>::reverse_iterator recf = m_recfolders.rbegin();
            recf != m_recfolders.rend(); ++recf)
        {
          if (tmp.compare(0, recf->length(), *recf) != 0)
            continue;
          tmp = tmp.substr(recf->length(), tmp.ReverseFind('\\') - recf->length());
          tmp.Replace('\\', '/');
          PVR_STRCPY(tag.strDirectory, tmp.c_str() + 1);
          break;
        }
        break;
      case DvbRecording::GroupByDate:
        tmp.Format("%s/%s", startTime.substr(0, 4), startTime.substr(4, 2));
        PVR_STRCPY(tag.strDirectory, tmp.c_str());
        break;
      case DvbRecording::GroupByFirstLetter:
        tag.strDirectory[0] = recording.title[0];
        tag.strDirectory[1] = '\0';
        break;
      case DvbRecording::GroupByTVChannel:
        PVR_STRCPY(tag.strDirectory, recording.channelName.c_str());
        break;
      case DvbRecording::GroupBySeries:
        tmp = "Unknown";
        XMLUtils::GetString(xRecording, "series", tmp);
        PVR_STRCPY(tag.strDirectory, tmp.c_str());
        break;
      default:
        break;
    }

    PVR->TransferRecordingEntry(handle, &tag);
    ++m_recordingAmount;

    XBMC->Log(LOG_DEBUG, "%s loaded Recording entry '%s': start=%u, length=%u",
        __FUNCTION__, recording.title.c_str(), recording.startTime,
        recording.duration);
  }

  XBMC->Log(LOG_INFO, "Loaded %u Recording Entries", m_recordingAmount);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR Dvb::DeleteRecording(const PVR_RECORDING& recinfo)
{
  CStdString recid = recinfo.strRecordingId;
  CStdString::size_type pos = recid.find('_');
  if (pos != CStdString::npos)
    recid.erase(pos);

  // RS api doesn't return a result
  GetHttpXML(BuildURL("api/recdelete.html?recid=%s&delfile=1", recid.c_str()));

  PVR->TriggerRecordingUpdate();

  return PVR_ERROR_NO_ERROR;
}

unsigned int Dvb::GetRecordingsAmount()
{
  return m_recordingAmount;
}


bool Dvb::OpenLiveStream(const PVR_CHANNEL& channelinfo)
{
  XBMC->Log(LOG_DEBUG, "%s channel=%u", __FUNCTION__, channelinfo.iUniqueId);

  if (channelinfo.iUniqueId == m_currentChannel)
    return true;

  SwitchChannel(channelinfo);
  if (!g_useTimeshift)
    return true;

  if (m_tsBuffer)
    SAFE_DELETE(m_tsBuffer);

  CStdString streamURL = GetLiveStreamURL(channelinfo);
  XBMC->Log(LOG_INFO, "Timeshift starts; url=%s", streamURL.c_str());
  m_tsBuffer = new TimeshiftBuffer(streamURL, g_timeshiftBufferPath);
  return m_tsBuffer->IsValid();
}

void Dvb::CloseLiveStream(void)
{
  m_currentChannel = 0;
  if (m_tsBuffer)
    SAFE_DELETE(m_tsBuffer);
}

TimeshiftBuffer *Dvb::GetTimeshiftBuffer()
{
  return m_tsBuffer;
}

CStdString& Dvb::GetLiveStreamURL(const PVR_CHANNEL& channelinfo)
{
  return m_channels[channelinfo.iUniqueId - 1]->streamURL;
}


void *Dvb::Process()
{
  int updateTimer = 0;
  XBMC->Log(LOG_DEBUG, "%s starting", __FUNCTION__);

  while (!IsStopped())
  {
    Sleep(1000);
    ++updateTimer;

    if (m_updateEPG)
    {
      Sleep(8000); /* Sleep enough time to let the recording service grab the EPG data */
      PVR->TriggerEpgUpdate(m_currentChannel);
      m_updateEPG = false;
    }

    if (updateTimer > 60 || m_updateTimers)
    {
      updateTimer = 0;

      // Trigger Timer and Recording updates acording to the addon settings
      CLockObject lock(m_mutex);
      XBMC->Log(LOG_INFO, "Performing timer/recording updates!");

      if (m_updateTimers)
      {
        Sleep(500);
        m_updateTimers = false;
      }
      TimerUpdates();
      PVR->TriggerRecordingUpdate();
    }
  }

  CLockObject lock(m_mutex);
  m_started.Broadcast();

  return NULL;
}


CStdString Dvb::GetHttpXML(const CStdString& url)
{
  CStdString result;
  void *fileHandle = XBMC->OpenFile(url, READ_NO_CACHE);
  if (fileHandle)
  {
    char buffer[1024];
    while (int bytesRead = XBMC->ReadFile(fileHandle, buffer, 1024))
      result.append(buffer, bytesRead);
    XBMC->CloseFile(fileHandle);
  }
  return result;
}

CStdString Dvb::URLEncodeInline(const CStdString& data)
{
  /* Copied from xbmc/URL.cpp */
  CStdString result;

  /* wonder what a good value is here is, depends on how often it occurs */
  result.reserve(data.length() * 2);

  for (unsigned int i = 0; i < data.length(); ++i)
  {
    int kar = (unsigned char)data[i];
    //if (kar == ' ') result += '+'; // obsolete
    if (isalnum(kar) || strchr("-_.!()" , kar) ) // Don't URL encode these according to RFC1738
      result += kar;
    else
    {
      CStdString tmp;
      tmp.Format("%%%02.2x", kar);
      result += tmp;
    }
  }
  return result;
}

bool Dvb::LoadChannels()
{
  CStdString url = BuildURL("api/getchannelsxml.html?subchannels=1&rtsp=1&upnp=1&logo=1");
  CStdString req = GetHttpXML(url);

  TiXmlDocument doc;
  doc.Parse(req);
  if (doc.Error())
  {
    XBMC->Log(LOG_ERROR, "Unable to parse channels. Error: %s",
        doc.ErrorDesc());
    XBMC->QueueNotification(QUEUE_ERROR, XBMC->GetLocalizedString(30502));
    XBMC->QueueNotification(QUEUE_ERROR, XBMC->GetLocalizedString(30503));
    return false;
  }

  TiXmlElement *root = doc.RootElement();

  CStdString streamURL;
  XMLUtils::GetString(root, (g_useRTSP) ? "rtspURL" : "upnpURL", streamURL);

  m_channels.clear();
  m_channelAmount = 0;
  m_groups.clear();
  m_groupAmount = 0;

  for (TiXmlElement *xRoot = doc.RootElement()->FirstChildElement("root");
      xRoot; xRoot = xRoot->NextSiblingElement("root"))
  {
    for (TiXmlElement *xGroup = xRoot->FirstChildElement("group");
        xGroup; xGroup = xGroup->NextSiblingElement("group"))
    {
      m_groups.push_back(DvbGroup());
      DvbGroup *group = &m_groups.back();
      group->name     = xGroup->Attribute("name");
      group->hidden   = g_useFavourites;
      group->radio    = true;
      if (!group->hidden)
        ++m_groupAmount;

      for (TiXmlElement *xChannel = xGroup->FirstChildElement("channel");
          xChannel; xChannel = xChannel->NextSiblingElement("channel"))
      {
        DvbChannel *channel = new DvbChannel();
        unsigned int flags = 0;
        xChannel->QueryUnsignedAttribute("flags", &flags);
        channel->radio      = !(flags & VIDEO_FLAG);
        channel->encrypted  = (flags & ENCRYPTED_FLAG);
        channel->name       = xChannel->Attribute("name");
        channel->hidden     = g_useFavourites;
        channel->frontendNr = (!g_useFavourites) ? m_channels.size() + 1 : 0;
        xChannel->QueryUnsignedAttribute("nr", &channel->backendNr);
        xChannel->QueryValueAttribute<uint64_t>("EPGID", &channel->epgId);

        uint64_t backendId = 0;
        xChannel->QueryValueAttribute<uint64_t>("ID", &backendId);
        channel->backendIds.push_back(backendId);

        CStdString logoURL;
        if (!g_lowPerformance && XMLUtils::GetString(xChannel, "logo", logoURL))
          channel->logoURL = BuildURL("%s", logoURL.c_str());

        if (g_useRTSP)
        {
          CStdString urlParams;
          XMLUtils::GetString(xChannel, "rtsp", urlParams);
          channel->streamURL = BuildExtURL(streamURL, "%s", urlParams.c_str());
        }
        else
          channel->streamURL = BuildExtURL(streamURL, "%u.ts", channel->backendNr);

        for (TiXmlElement* xSubChannel = xChannel->FirstChildElement("subchannel");
            xSubChannel; xSubChannel = xSubChannel->NextSiblingElement("subchannel"))
        {
          uint64_t backendId = 0;
          xSubChannel->QueryValueAttribute<uint64_t>("ID", &backendId);
          channel->backendIds.push_back(backendId);
        }

        //FIXME: PVR_CHANNEL.UniqueId is uint32 but DVBViewer ids are uint64
        // so generate our own unique ids, at least for this session
        channel->id = m_channels.size() + 1;
        m_channels.push_back(channel);
        group->channels.push_back(channel);
        if (!channel->hidden)
          ++m_channelAmount;

        if (!channel->radio)
          group->radio = false;
      }
    }
  }

  if (g_useFavourites)
  {
    CStdString url = BuildURL("api/getfavourites.html");
    if (g_useFavouritesFile)
    {
      if (!XBMC->FileExists(g_favouritesFile, false))
      {
        XBMC->Log(LOG_ERROR, "Unable to open local favourites.xml");
        XBMC->QueueNotification(QUEUE_ERROR, XBMC->GetLocalizedString(30504));
        return false;
      }
      url = g_favouritesFile;
    }

    CStdString req = GetHttpXML(url);
    RemoveNullChars(req);

    TiXmlDocument doc;
    doc.Parse(req);
    if (doc.Error())
    {
      XBMC->Log(LOG_ERROR, "Unable to parse favourites.xml. Error: %s",
          doc.ErrorDesc());
      XBMC->QueueNotification(QUEUE_ERROR, XBMC->GetLocalizedString(30505));
      XBMC->QueueNotification(QUEUE_ERROR, XBMC->GetLocalizedString(30503));
      return false;
    }

    m_groups.clear();
    m_groupAmount = 0;

    /* example data:
     * <settings>
     *    <section name="0">
     *      <entry name="Header">Group 1</entry>
     *      <entry name="0">1234567890123456789|Channel 1</entry>
     *      <entry name="1">1234567890123456789|Channel 2</entry>
     *    </section>
     *   <section name="1">
     *     <entry name="Header">1234567890123456789|Channel 3</entry>
     *    </section>
     *    ...
     *  </settings>
     */
    for (TiXmlElement *xSection = doc.RootElement()->FirstChildElement("section");
        xSection; xSection = xSection->NextSiblingElement("section"))
    {
      DvbGroup *group = NULL;
      for (TiXmlElement *xEntry = xSection->FirstChildElement("entry");
          xEntry; xEntry = xEntry->NextSiblingElement("entry"))
      {
        // name="Header" doesn't indicate a group alone. we must have at least
        // one additional child. see example above
        if (!group && CStdString(xEntry->Attribute("name")) == "Header"
            && xEntry->NextSiblingElement("entry"))
        {
          m_groups.push_back(DvbGroup());
          group = &m_groups.back();
          group->name   = ConvertToUtf8(xEntry->GetText());
          group->hidden = false;
          group->radio  = false;
          ++m_groupAmount;
          continue;
        }

        uint64_t backendId = 0;
        std::istringstream ss(xEntry->GetText());
        ss >> backendId;
        if (!backendId)
          continue;

        for (DvbChannels_t::iterator it = m_channels.begin();
            it != m_channels.end(); ++it)
        {
          DvbChannel *channel = *it;
          bool found = false;

          for (std::list<uint64_t>::iterator it2 = channel->backendIds.begin();
              it2 != channel->backendIds.end(); ++it2)
          {
            /* legacy support for old 32bit channel ids */
            uint64_t channelId = (backendId > 0xFFFFFFFF) ? *it2 :
              *it2 & 0xFFFFFFFF;
            if (channelId == backendId)
            {
              found = true;
              break;
            }
          }

          if (found)
          {
            channel->hidden = false;
            channel->frontendNr = ++m_channelAmount;
            if (!ss.eof())
            {
              ss.ignore(1);
              CStdString channelName;
              getline(ss, channelName);
              channel->name = ConvertToUtf8(channelName);
            }

            if (group)
            {
              group->channels.push_back(channel);
              if (!channel->radio)
                group->radio = false;
            }
            break;
          }
        }
      }
    }

    // assign channel number to remaining channels
    unsigned int channelNumber = m_channelAmount;
    for (DvbChannels_t::iterator it = m_channels.begin();
        it != m_channels.end(); ++it)
    {
      if (!(*it)->frontendNr)
        (*it)->frontendNr = ++channelNumber;
    }
  }

  XBMC->Log(LOG_INFO, "Loaded (%u/%u) channels in (%u/%u) groups",
      m_channelAmount, m_channels.size(), m_groupAmount, m_groups.size());
  // force channel sync as stream urls may have changed (e.g. rstp on/off)
  PVR->TriggerChannelUpdate();
  return true;
}

//TODO: rewrite
DvbTimers_t Dvb::LoadTimers()
{
  DvbTimers_t timers;

  CStdString url = BuildURL("api/timerlist.html?utf8");
  CStdString req = GetHttpXML(url);
  RemoveNullChars(req);

  TiXmlDocument doc;
  doc.Parse(req);
  if (doc.Error())
  {
    XBMC->Log(LOG_ERROR, "Unable to parse timers. Error: %s",
        doc.ErrorDesc());
    XBMC->QueueNotification(QUEUE_ERROR, XBMC->GetLocalizedString(30506));
    XBMC->QueueNotification(QUEUE_ERROR, XBMC->GetLocalizedString(30503));
    return timers;
  }

  for (TiXmlElement *xTimer = doc.RootElement()->FirstChildElement("Timer");
      xTimer; xTimer = xTimer->NextSiblingElement("Timer"))
  {
    DvbTimer timer;

    CStdString strTmp;
    if (XMLUtils::GetString(xTimer, "Descr", strTmp))
      XBMC->Log(LOG_DEBUG, "%s Processing timer '%s'", __FUNCTION__, strTmp.c_str());

    timer.strTitle = strTmp;
    timer.iChannelUid = GetChannelUid(xTimer->FirstChildElement("Channel")->Attribute("ID"));
    if (timer.iChannelUid == 0)
      continue;
    timer.state = PVR_TIMER_STATE_SCHEDULED;

    CStdString DateTime = xTimer->Attribute("Date");
    DateTime.append(xTimer->Attribute("Start"));
    timer.startTime = ParseDateTime(DateTime, false);
    timer.endTime = timer.startTime + atoi(xTimer->Attribute("Dur")) * 60;

    CStdString Weekdays = xTimer->Attribute("Days");
    timer.iWeekdays = 0;
    for (unsigned int j = 0; j < Weekdays.length(); ++j)
    {
      if (Weekdays.data()[j] != '-')
        timer.iWeekdays += (1 << j);
    }

    if (timer.iWeekdays != 0)
    {
      timer.iFirstDay = timer.startTime;
      timer.bRepeating = true;
    }
    else
      timer.bRepeating = false;

    timer.iPriority = atoi(xTimer->Attribute("Priority"));

    if (xTimer->Attribute("EPGEventID"))
      timer.iEpgId = atoi(xTimer->Attribute("EPGEventID"));

    if (xTimer->Attribute("Enabled")[0] == '0')
      timer.state = PVR_TIMER_STATE_CANCELLED;

    int iTmp;
    if (XMLUtils::GetInt(xTimer, "Recording", iTmp))
    {
      if (iTmp == -1)
        timer.state = PVR_TIMER_STATE_RECORDING;
    }

    if (XMLUtils::GetInt(xTimer, "ID", iTmp))
      timer.iTimerId = iTmp;

    timers.push_back(timer);

    XBMC->Log(LOG_DEBUG, "%s loaded Timer entry '%s': start=%u, end=%u",
        __FUNCTION__, timer.strTitle.c_str(), timer.startTime, timer.endTime);
  }

  XBMC->Log(LOG_INFO, "Loaded %u Timer entries", timers.size());
  return timers;
}

//TODO: rewrite
void Dvb::TimerUpdates()
{
  for (DvbTimers_t::iterator timer = m_timers.begin();
      timer != m_timers.end(); ++timer)
    timer->iUpdateState = DVB_UPDATE_STATE_NONE;

  DvbTimers_t newtimers = LoadTimers();
  unsigned int updated = 0, unchanged = 0;
  for (DvbTimers_t::iterator newtimer = newtimers.begin();
      newtimer != newtimers.end(); ++newtimer)
  {
    for (DvbTimers_t::iterator timer = m_timers.begin();
        timer != m_timers.end(); ++timer)
    {
      if (!timer->like(*newtimer))
        continue;

      if (*timer == *newtimer)
      {
        timer->iUpdateState = newtimer->iUpdateState = DVB_UPDATE_STATE_FOUND;
        ++unchanged;
      }
      else
      {
        timer->iUpdateState = newtimer->iUpdateState = DVB_UPDATE_STATE_UPDATED;
        timer->strTitle     = newtimer->strTitle;
        timer->strPlot      = newtimer->strPlot;
        timer->iChannelUid  = newtimer->iChannelUid;
        timer->startTime    = newtimer->startTime;
        timer->endTime      = newtimer->endTime;
        timer->bRepeating   = newtimer->bRepeating;
        timer->iWeekdays    = newtimer->iWeekdays;
        timer->iEpgId       = newtimer->iEpgId;
        timer->iTimerId     = newtimer->iTimerId;
        timer->iPriority    = newtimer->iPriority;
        timer->iFirstDay    = newtimer->iFirstDay;
        timer->state        = newtimer->state;
        ++updated;
      }
    }
  }

  unsigned int removed = 0;
  for (DvbTimers_t::iterator it = m_timers.begin(); it != m_timers.end();)
  {
    if (it->iUpdateState == DVB_UPDATE_STATE_NONE)
    {
      XBMC->Log(LOG_DEBUG, "%s Removed timer: '%s', ClientIndex: %u",
          __FUNCTION__, it->strTitle.c_str(), it->iClientIndex);
      it = m_timers.erase(it);
      ++removed;
    }
    else
      ++it;
  }

  unsigned int added = 0;
  for (DvbTimers_t::iterator it = newtimers.begin(); it != newtimers.end(); ++it)
  {
    if (it->iUpdateState == DVB_UPDATE_STATE_NEW)
    {
      it->iClientIndex = m_newTimerIndex;
      XBMC->Log(LOG_DEBUG, "%s New timer: '%s', ClientIndex: %u",
          __FUNCTION__, it->strTitle.c_str(), m_newTimerIndex);
      m_timers.push_back(*it);
      ++m_newTimerIndex;
      ++added;
    }
  }

  XBMC->Log(LOG_DEBUG, "%s Timers update: removed=%u, untouched=%u, updated=%u, added=%u",
      __FUNCTION__, removed, unchanged, updated, added);

  if (removed || updated || added)
  {
    XBMC->Log(LOG_INFO, "Changes in timerlist detected, triggering an update!");
    PVR->TriggerTimerUpdate();
  }
}

void Dvb::GenerateTimer(const PVR_TIMER& timer, bool newTimer)
{
  // http://en.dvbviewer.tv/wiki/Recording_Service_Web_API#Add_a_timer

  XBMC->Log(LOG_DEBUG, "%s iChannelUid=%u title='%s' epgid=%d",
      __FUNCTION__, timer.iClientChannelUid, timer.strTitle, timer.iEpgUid);

  time_t startTime = timer.startTime, endTime = timer.endTime;
  if (!startTime)
    startTime = time(NULL);
  else
  {
    startTime -= timer.iMarginStart * 60;
    endTime += timer.iMarginEnd * 60;
  }

  unsigned int date = ((startTime + m_timezone) / DAY_SECS) + DELPHI_DATE;
  struct tm *timeinfo;
  timeinfo = localtime(&startTime);
  unsigned int start = timeinfo->tm_hour * 60 + timeinfo->tm_min;
  timeinfo = localtime(&endTime);
  unsigned int stop = timeinfo->tm_hour * 60 + timeinfo->tm_min;

  char repeat[8] = "-------";
  for (int i = 0; i < 7; ++i)
  {
    if (timer.iWeekdays & (1 << i))
      repeat[i] = 'T';
  }

  uint64_t iChannelId = m_channels[timer.iClientChannelUid - 1]->backendIds.front();
  CStdString url;
  if (newTimer)
    url = BuildURL("api/timeradd.html?ch=%"PRIu64"&dor=%u&enable=1&start=%u&stop=%u&prio=%d&days=%s&title=%s&encoding=255",
        iChannelId, date, start, stop, timer.iPriority, repeat, URLEncodeInline(timer.strTitle).c_str());
  else
  {
    short enabled = (timer.state == PVR_TIMER_STATE_CANCELLED) ? 0 : 1;
    url = BuildURL("api/timeredit.html?id=%d&ch=%"PRIu64"&dor=%u&enable=%d&start=%u&stop=%u&prio=%d&days=%s&title=%s&encoding=255",
        GetTimerId(timer), iChannelId, date, enabled, start, stop, timer.iPriority, repeat, URLEncodeInline(timer.strTitle).c_str());
  }

  GetHttpXML(url);
  m_updateTimers = true;
}

int Dvb::GetTimerId(const PVR_TIMER& timer)
{
  for (DvbTimers_t::iterator it = m_timers.begin(); it != m_timers.end(); ++it)
  {
    if (it->iClientIndex == timer.iClientIndex)
      return it->iTimerId;
  }
  return 0;
}


void Dvb::RemoveNullChars(CStdString& str)
{
  /* favourites.xml and timers.xml sometimes have null chars that screw the xml */
  str.erase(std::remove(str.begin(), str.end(), '\0'), str.end());
}

bool Dvb::CheckBackendVersion()
{
  CStdString url = BuildURL("api/version.html");
  CStdString req = GetHttpXML(url);

  TiXmlDocument doc;
  doc.Parse(req);
  if (doc.Error())
  {
    XBMC->Log(LOG_ERROR, "Unable to connect to the backend service. Error: %s",
        doc.ErrorDesc());
    XBMC->QueueNotification(QUEUE_ERROR, XBMC->GetLocalizedString(30500));
    Sleep(10000);
    return false;
  }

  XBMC->Log(LOG_NOTICE, "Checking backend version...");
  if (doc.RootElement()->QueryUnsignedAttribute("iver", &m_backendVersion)
      != TIXML_SUCCESS)
  {
    XBMC->Log(LOG_ERROR, "Unable to parse version");
    return false;
  }
  XBMC->Log(LOG_NOTICE, "Version: %u", m_backendVersion);

  if (m_backendVersion < RS_VERSION_NUM)
  {
    XBMC->Log(LOG_ERROR, "Recording Service version %s or higher required",
        RS_VERSION_STR);
    XBMC->QueueNotification(QUEUE_ERROR, XBMC->GetLocalizedString(30501),
        RS_VERSION_STR);
    Sleep(10000);
    return false;
  }

  return true;
}

static bool StringGreaterThan(const CStdString& a, const CStdString& b)
{
  return (a.length() < b.length());
}

bool Dvb::UpdateBackendStatus(bool updateSettings)
{
  CStdString url = BuildURL("api/status.html");
  CStdString req = GetHttpXML(url);

  TiXmlDocument doc;
  doc.Parse(req);
  if (doc.Error())
  {
    XBMC->Log(LOG_ERROR, "Unable to get backend status. Error: %s",
        doc.ErrorDesc());
    return false;
  }

  TiXmlElement *root = doc.RootElement();

  if (updateSettings)
  {
    // RS doesn't update the timezone during daylight saving
    //if (XMLUtils::GetLong(root, "timezone", m_timezone))
    //  m_timezone *= 60;
    m_timezone = GetGMTOffset();

    m_recfolders.clear();
  }

  // compute disk space. duplicates are detected by their identical values
  typedef std::pair<long long, long long> Recfolder_t;
  std::set<Recfolder_t> folders;
  m_diskspace.total = m_diskspace.used = 0;
  for (TiXmlElement *xFolder = TiXmlHandle(root).FirstChild("recfolders")
      .FirstChild("folder").ToElement();
      xFolder; xFolder = xFolder->NextSiblingElement("folder"))
  {
    long long size = 0, free = 0;
    xFolder->QueryValueAttribute<long long>("size", &size);
    xFolder->QueryValueAttribute<long long>("free", &free);

    if (folders.insert(std::make_pair(size, free)).second)
    {
      m_diskspace.total += size / 1024;
      m_diskspace.used += (size - free) / 1024;
    }

    if (updateSettings && g_groupRecordings != DvbRecording::GroupDisabled)
      m_recfolders.push_back(CStdString(xFolder->GetText()).ToLower());
  }

  if (updateSettings && g_groupRecordings != DvbRecording::GroupDisabled)
    std::sort(m_recfolders.begin(), m_recfolders.end(), StringGreaterThan);

  return true;
}

time_t Dvb::ParseDateTime(const CStdString& date, bool iso8601)
{
  struct tm timeinfo;

  memset(&timeinfo, 0, sizeof(tm));
  if (iso8601)
    sscanf(date, "%04d%02d%02d%02d%02d%02d", &timeinfo.tm_year,
        &timeinfo.tm_mon, &timeinfo.tm_mday, &timeinfo.tm_hour,
        &timeinfo.tm_min, &timeinfo.tm_sec);
  else
    sscanf(date, "%02d.%02d.%04d%02d:%02d:%02d", &timeinfo.tm_mday,
        &timeinfo.tm_mon, &timeinfo.tm_year, &timeinfo.tm_hour,
        &timeinfo.tm_min, &timeinfo.tm_sec);
  timeinfo.tm_mon  -= 1;
  timeinfo.tm_year -= 1900;
  timeinfo.tm_isdst = -1;

  return mktime(&timeinfo);
}

uint64_t Dvb::ParseChannelString(const CStdString& str, CStdString& channelName)
{
  std::vector<CStdString> tokenlist;
  tokenize<std::vector<CStdString> >(str, tokenlist, "|");
  if (tokenlist.size() < 1)
  {
    XBMC->Log(LOG_ERROR, "Unable to parse channel string: %s", str.c_str());
    return 0;
  }

  uint64_t channelId = 0;
  std::istringstream ss(tokenlist[0]);
  ss >> channelId;
  if (ss.fail())
  {
    XBMC->Log(LOG_ERROR, "Unable to parse channel id: %s",
        tokenlist[0].c_str());
    return 0;
  }

  channelName.clear();
  if (tokenlist.size() >= 2)
    channelName = ConvertToUtf8(tokenlist[1]);
  return channelId;
}

unsigned int Dvb::GetChannelUid(const CStdString& str)
{
  CStdString channelName;
  uint64_t channelId = ParseChannelString(str, channelName);
  if (channelId == 0)
    return 0;
  return GetChannelUid(channelId);
}

unsigned int Dvb::GetChannelUid(const uint64_t channelId)
{
  for (DvbChannels_t::iterator it = m_channels.begin();
      it != m_channels.end(); ++it)
  {
    DvbChannel *channel = *it;
    for (std::list<uint64_t>::iterator backendId = channel->backendIds.begin();
        backendId != channel->backendIds.end(); backendId++)
    {
      if (channelId == *backendId)
        return channel->id;
    }
  }
  return 0;
}

CStdString Dvb::BuildURL(const char* path, ...)
{
  CStdString url(m_url);
  va_list argList;
  va_start(argList, path);
  url.AppendFormatV(path, argList);
  va_end(argList);
  return url;
}

CStdString Dvb::BuildExtURL(const CStdString& baseURL, const char* path, ...)
{
  CStdString url(baseURL);
  // simply add user@pass in front of the URL if username/password is set
  if (!g_username.empty() && !g_password.empty())
  {
    CStdString auth;
    auth.Format("%s:%s@", g_username.c_str(), g_password.c_str());
    CStdString::size_type pos = url.find("://");
    if (pos != CStdString::npos)
      url.insert(pos + strlen("://"), auth);
  }
  va_list argList;
  va_start(argList, path);
  url.AppendFormatV(path, argList);
  va_end(argList);
  return url;
}

CStdString Dvb::ConvertToUtf8(const CStdString& src)
{
  char *tmp = XBMC->UnknownToUTF8(src);
  CStdString dest(tmp);
  XBMC->FreeString(tmp);
  return dest;
}

long Dvb::GetGMTOffset()
{
#ifdef TARGET_POSIX
  struct tm t;
  tzset();
  time_t tt = time(NULL);
  if (localtime_r(&tt, &t))
    return t.tm_gmtoff;
#else
  TIME_ZONE_INFORMATION tz;
  switch(GetTimeZoneInformation(&tz))
  {
    case TIME_ZONE_ID_DAYLIGHT:
      return (tz.Bias + tz.DaylightBias) * -60;
      break;
    case TIME_ZONE_ID_STANDARD:
      return (tz.Bias + tz.StandardBias) * -60;
      break;
    case TIME_ZONE_ID_UNKNOWN:
      return tz.Bias * -60;
      break;
  }
#endif
  return 0;
}
