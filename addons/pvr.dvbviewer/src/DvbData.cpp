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

Dvb::Dvb()
  : m_connected(false), m_backendVersion(0), m_currentChannel(0),
  m_nextTimerId(0)
{
  // simply add user@pass in front of the URL if username/password is set
  CStdString auth("");
  if (!g_username.empty() && !g_password.empty())
    auth.Format("%s:%s@", g_username.c_str(), g_password.c_str());
  m_url.Format("http://%s%s:%u/", auth.c_str(), g_hostname.c_str(), g_webPort);

  m_updateTimers = false;
  m_updateEPG    = false;
}

Dvb::~Dvb()
{
  StopThread();

  for (DvbChannels_t::iterator channel = m_channels.begin();
      channel != m_channels.end(); ++channel)
    delete *channel;
}

bool Dvb::Open()
{
  CLockObject lock(m_mutex);

  if (!(m_connected = CheckBackendVersion()))
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

bool Dvb::GetDriveSpace(long long *total, long long *used)
{
  if (!UpdateBackendStatus())
    return false;
  *total = m_diskspace.total;
  *used  = m_diskspace.used;
  return true;
}

bool Dvb::SwitchChannel(const PVR_CHANNEL& channelinfo)
{
  m_currentChannel = channelinfo.iUniqueId;
  m_updateEPG = true;
  return true;
}

unsigned int Dvb::GetCurrentClientChannel(void)
{
  return m_currentChannel;
}

bool Dvb::GetChannels(ADDON_HANDLE handle, bool radio)
{
  for (DvbChannels_t::iterator it = m_channels.begin();
      it != m_channels.end(); ++it)
  {
    DvbChannel *channel = *it;
    if (channel->hidden)
      continue;
    if (channel->radio != radio)
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
      PVR_STRCPY(xbmcChannel.strInputFormat, "video/mp2t");
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
  return true;
}

bool Dvb::GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL& channelinfo,
    time_t start, time_t end)
{
  DvbChannel *channel = m_channels[channelinfo.iUniqueId - 1];

  CStdString url = BuildURL("api/epg.html?lvl=2&channel=%" PRIu64 "&start=%f&end=%f",
      channel->epgId, start/86400.0 + DELPHI_DATE, end/86400.0 + DELPHI_DATE);
  CStdString req = GetHttpXML(url);

  TiXmlDocument doc;
  doc.Parse(req);
  if (doc.Error())
  {
    XBMC->Log(LOG_ERROR, "Unable to parse EPG. Error: %s",
        doc.ErrorDesc());
    return false;
  }

  unsigned int numEPG = 0;
  for (TiXmlElement *xEntry = doc.RootElement()->FirstChildElement("programme");
      xEntry; xEntry = xEntry->NextSiblingElement("programme"))
  {
    DvbEPGEntry entry;
    entry.channel = channel;
    entry.start   = ParseDateTime(xEntry->Attribute("start"));
    entry.end     = ParseDateTime(xEntry->Attribute("stop"));

    if (end > 1 && end < entry.end)
       continue;

    if (!XMLUtils::GetUInt(xEntry, "eventid", entry.id))
      continue;

    // since RS 1.26.0 the correct language is already merged into the elements
    TiXmlNode *xTitles = xEntry->FirstChild("titles");
    if (!xTitles || !XMLUtils::GetString(xTitles, "title", entry.title))
      continue;

    TiXmlNode *xDescriptions = xEntry->FirstChild("descriptions");
    if (xDescriptions)
      XMLUtils::GetString(xDescriptions, "description", entry.plot);

    TiXmlNode *xEvents = xEntry->FirstChild("events");
    if (xEvents)
    {
      XMLUtils::GetString(xEvents, "event", entry.plotOutline);
      if (entry.plot.empty())
      {
        entry.plot = entry.plotOutline;
        entry.plotOutline.clear();
      }
      else if (PrependOutline::test(PrependOutline::IN_EPG))
      {
        entry.plot.insert(0, entry.plotOutline + "\n");
        entry.plotOutline.clear();
      }
    }

    XMLUtils::GetUInt(xEntry, "content", entry.genre);

    EPG_TAG broadcast;
    memset(&broadcast, 0, sizeof(EPG_TAG));
    broadcast.iUniqueBroadcastId  = entry.id;
    broadcast.strTitle            = entry.title.c_str();
    broadcast.iChannelNumber      = channelinfo.iChannelNumber;
    broadcast.startTime           = entry.start;
    broadcast.endTime             = entry.end;
    broadcast.strPlotOutline      = entry.plotOutline.c_str();
    broadcast.strPlot             = entry.plot.c_str();
    broadcast.iGenreType          = entry.genre & 0xF0;
    broadcast.iGenreSubType       = entry.genre & 0x0F;

    PVR->TransferEpgEntry(handle, &broadcast);
    ++numEPG;

    XBMC->Log(LOG_DEBUG, "%s: Loaded EPG entry '%u:%s': start=%u, end=%u",
        __FUNCTION__, entry.id, entry.title.c_str(),
        entry.start, entry.end);
  }

  XBMC->Log(LOG_INFO, "Loaded %u EPG entries for channel '%s'",
      numEPG, channel->name.c_str());
  return true;
}

unsigned int Dvb::GetChannelsAmount()
{
  return m_channelAmount;
}


bool Dvb::GetChannelGroups(ADDON_HANDLE handle, bool radio)
{
  for (DvbGroups_t::iterator group = m_groups.begin();
      group != m_groups.end(); ++group)
  {
    if (group->hidden)
      continue;
    if (group->radio != radio)
      continue;

    PVR_CHANNEL_GROUP tag;
    memset(&tag, 0, sizeof(PVR_CHANNEL_GROUP));
    tag.bIsRadio = group->radio;
    PVR_STRCPY(tag.strGroupName, group->name.c_str());

    PVR->TransferChannelGroup(handle, &tag);
  }
  return true;
}

bool Dvb::GetChannelGroupMembers(ADDON_HANDLE handle,
    const PVR_CHANNEL_GROUP& pvrGroup)
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

      XBMC->Log(LOG_DEBUG, "%s: Add channel '%s' (%u) to group '%s'",
          __FUNCTION__, channel->name.c_str(), channel->backendNr,
          group->name.c_str());
    }
  }
  return true;
}

unsigned int Dvb::GetChannelGroupsAmount()
{
  return m_groupAmount;
}


bool Dvb::GetTimers(ADDON_HANDLE handle)
{
  for (DvbTimers_t::iterator timer = m_timers.begin();
      timer != m_timers.end(); ++timer)
  {
    PVR_TIMER tag;
    memset(&tag, 0, sizeof(PVR_TIMER));
    PVR_STRCPY(tag.strTitle,   timer->title.c_str());
    tag.iClientIndex      = timer->id;
    tag.iClientChannelUid = timer->channel->id;
    tag.startTime         = timer->start;
    tag.endTime           = timer->end;
    tag.state             = timer->state;
    tag.iPriority         = timer->priority;
    tag.bIsRepeating      = (timer->weekdays != 0);
    tag.firstDay          = (timer->weekdays != 0) ? timer->start : 0;
    tag.iWeekdays         = timer->weekdays;

    PVR->TransferTimerEntry(handle, &tag);
  }
  return true;
}

bool Dvb::AddTimer(const PVR_TIMER& timer, bool update)
{
  XBMC->Log(LOG_DEBUG, "%s: channel=%u, title='%s'",
      __FUNCTION__, timer.iClientChannelUid, timer.strTitle);

  time_t startTime = timer.startTime - timer.iMarginStart * 60;
  time_t endTime   = timer.endTime   + timer.iMarginEnd * 60;
  if (!timer.startTime)
    startTime = time(NULL);

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

  uint64_t channelId = m_channels[timer.iClientChannelUid - 1]->backendIds.front();
  CStdString url;
  if (!update)
    url = BuildURL("api/timeradd.html?ch=%" PRIu64 "&dor=%u&enable=1&start=%u&stop=%u&prio=%d&days=%s&title=%s&encoding=255",
        channelId, date, start, stop, timer.iPriority, repeat, URLEncodeInline(timer.strTitle).c_str());
  else
  {
    DvbTimer *t = GetTimer(timer);
    if (!t)
      return false;

    short enabled = (timer.state == PVR_TIMER_STATE_CANCELLED) ? 0 : 1;
    url = BuildURL("api/timeredit.html?id=%d&ch=%" PRIu64 "&dor=%u&enable=%d&start=%u&stop=%u&prio=%d&days=%s&title=%s&encoding=255",
        t->backendId, channelId, date, enabled, start, stop, timer.iPriority, repeat, URLEncodeInline(timer.strTitle).c_str());
  }

  GetHttpXML(url);
  //TODO: instead of syncing all timers, we could only sync the new/modified
  m_updateTimers = true;
  return true;
}

bool Dvb::DeleteTimer(const PVR_TIMER& timer)
{
  DvbTimer *t = GetTimer(timer);
  if (!t)
    return false;

  GetHttpXML(BuildURL("api/timerdelete.html?id=%u", t->backendId));
  if (timer.state == PVR_TIMER_STATE_RECORDING)
    PVR->TriggerRecordingUpdate();

  m_updateTimers = true;
  return true;
}

unsigned int Dvb::GetTimersAmount()
{
  return m_timers.size();
}


bool Dvb::GetRecordings(ADDON_HANDLE handle)
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
    return false;
  }

  CStdString imageURL;
  TiXmlElement *root = doc.RootElement();
  // refresh in case this has changed
  XMLUtils::GetString(root, "serverURL", m_recordingURL);
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
    {
      recording.plot = recording.plotOutline;
      recording.plotOutline.clear();
    }
    else if (PrependOutline::test(PrependOutline::IN_RECORDINGS))
    {
      recording.plot.insert(0, recording.plotOutline + "\n");
      recording.plotOutline.clear();
    }

    CStdString thumbnail;
    if (!g_lowPerformance && XMLUtils::GetString(xRecording, "image", thumbnail))
      recording.thumbnailPath = BuildExtURL(imageURL, "%s", thumbnail.c_str());

    CStdString startTime = xRecording->Attribute("start");
    recording.start = ParseDateTime(startTime);

    int hours, mins, secs;
    sscanf(xRecording->Attribute("duration"), "%02d%02d%02d", &hours, &mins, &secs);
    recording.duration = hours*60*60 + mins*60 + secs;

    PVR_RECORDING recinfo;
    memset(&recinfo, 0, sizeof(PVR_RECORDING));
    PVR_STRCPY(recinfo.strRecordingId,   recording.id.c_str());
    PVR_STRCPY(recinfo.strTitle,         recording.title.c_str());
    PVR_STRCPY(recinfo.strPlotOutline,   recording.plotOutline.c_str());
    PVR_STRCPY(recinfo.strPlot,          recording.plot.c_str());
    PVR_STRCPY(recinfo.strChannelName,   recording.channelName.c_str());
    PVR_STRCPY(recinfo.strThumbnailPath, recording.thumbnailPath.c_str());
    recinfo.recordingTime = recording.start;
    recinfo.iDuration     = recording.duration;
    recinfo.iGenreType    = recording.genre & 0xF0;
    recinfo.iGenreSubType = recording.genre & 0x0F;

    CStdString tmp;
    switch(g_groupRecordings)
    {
      case DvbRecording::GROUP_BY_DIRECTORY:
        XMLUtils::GetString(xRecording, "file", tmp);
        tmp.ToLower();
        for (std::vector<CStdString>::reverse_iterator recf = m_recfolders.rbegin();
            recf != m_recfolders.rend(); ++recf)
        {
          if (tmp.compare(0, recf->length(), *recf) != 0)
            continue;
          tmp = tmp.substr(recf->length(), tmp.ReverseFind('\\') - recf->length());
          tmp.Replace('\\', '/');
          PVR_STRCPY(recinfo.strDirectory, tmp.c_str() + 1);
          break;
        }
        break;
      case DvbRecording::GROUP_BY_DATE:
        tmp.Format("%s/%s", startTime.substr(0, 4), startTime.substr(4, 2));
        PVR_STRCPY(recinfo.strDirectory, tmp.c_str());
        break;
      case DvbRecording::GROUP_BY_FIRST_LETTER:
        recinfo.strDirectory[0] = recording.title[0];
        recinfo.strDirectory[1] = '\0';
        break;
      case DvbRecording::GROUP_BY_TV_CHANNEL:
        PVR_STRCPY(recinfo.strDirectory, recording.channelName.c_str());
        break;
      case DvbRecording::GROUP_BY_SERIES:
        tmp = "Unknown";
        XMLUtils::GetString(xRecording, "series", tmp);
        PVR_STRCPY(recinfo.strDirectory, tmp.c_str());
        break;
      default:
        break;
    }

    PVR->TransferRecordingEntry(handle, &recinfo);
    ++m_recordingAmount;

    XBMC->Log(LOG_DEBUG, "%s: Loaded recording entry '%s': start=%u, length=%u",
        __FUNCTION__, recording.title.c_str(), recording.start,
        recording.duration);
  }

  XBMC->Log(LOG_INFO, "Loaded %u recording entries", m_recordingAmount);
  return true;
}

bool Dvb::DeleteRecording(const PVR_RECORDING& recinfo)
{
  // RS api doesn't return a result
  GetHttpXML(BuildURL("api/recdelete.html?recid=%s&delfile=1",
        recinfo.strRecordingId));

  PVR->TriggerRecordingUpdate();
  return true;
}

unsigned int Dvb::GetRecordingsAmount()
{
  return m_recordingAmount;
}

RecordingReader *Dvb::OpenRecordedStream(const PVR_RECORDING &recinfo)
{
  time_t now = time(NULL), end = 0;
  CStdString channelName = recinfo.strChannelName;
  for (DvbTimers_t::iterator timer = m_timers.begin();
      timer != m_timers.end(); ++timer)
  {
    if (timer->start <= now && now <= timer->end
        && timer->state != PVR_TIMER_STATE_CANCELLED
        && timer->channel->name.compare(0, channelName.length(), channelName) == 0)
    {
      end = timer->end;
      break;
    }
  }

  return new RecordingReader(BuildExtURL(m_recordingURL, "%s.ts",
        recinfo.strRecordingId), end);
}


bool Dvb::OpenLiveStream(const PVR_CHANNEL& channelinfo)
{
  XBMC->Log(LOG_DEBUG, "%s: channel=%u", __FUNCTION__, channelinfo.iUniqueId);

  if (channelinfo.iUniqueId == m_currentChannel)
    return true;

  SwitchChannel(channelinfo);
  return true;
}

void Dvb::CloseLiveStream(void)
{
  m_currentChannel = 0;
}

CStdString& Dvb::GetLiveStreamURL(const PVR_CHANNEL& channelinfo)
{
  return m_channels[channelinfo.iUniqueId - 1]->streamURL;
}


void *Dvb::Process()
{
  int updateTimer = 0;
  XBMC->Log(LOG_DEBUG, "%s: Running...", __FUNCTION__);

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

    if (!XMLUtils::GetString(xTimer, "GUID", timer.guid))
      continue;
    XMLUtils::GetUInt(xTimer, "ID", timer.backendId);
    XMLUtils::GetString(xTimer, "Descr", timer.title);

    uint64_t backendId = 0;
    std::istringstream ss(xTimer->FirstChildElement("Channel")->Attribute("ID"));
    ss >> backendId;
    if (!backendId)
      continue;

    timer.channel = GetChannelByBackendId(backendId);
    if (!timer.channel)
      continue;

    CStdString startDate = xTimer->Attribute("Date");
    startDate += xTimer->Attribute("Start");
    timer.start = ParseDateTime(startDate, false);
    timer.end   = timer.start + atoi(xTimer->Attribute("Dur")) * 60;

    CStdString weekdays = xTimer->Attribute("Days");
    timer.weekdays = 0;
    for (unsigned int j = 0; j < weekdays.length(); ++j)
    {
      if (weekdays.data()[j] != '-')
        timer.weekdays += (1 << j);
    }

    timer.priority    = atoi(xTimer->Attribute("Priority"));
    timer.updateState = DvbTimer::STATE_NEW;
    timer.state       = PVR_TIMER_STATE_SCHEDULED;
    if (xTimer->Attribute("Enabled")[0] == '0')
      timer.state = PVR_TIMER_STATE_CANCELLED;

    int tmp;
    XMLUtils::GetInt(xTimer, "Recording", tmp);
    if (tmp == -1)
      timer.state = PVR_TIMER_STATE_RECORDING;

    timers.push_back(timer);
    XBMC->Log(LOG_DEBUG, "%s: Loaded timer entry '%s': start=%u, end=%u",
        __FUNCTION__, timer.title.c_str(), timer.start, timer.end);
  }

  XBMC->Log(LOG_INFO, "Loaded %u timer entries", timers.size());
  return timers;
}

void Dvb::TimerUpdates()
{
  for (DvbTimers_t::iterator timer = m_timers.begin();
      timer != m_timers.end(); ++timer)
    timer->updateState = DvbTimer::STATE_NONE;

  DvbTimers_t newtimers = LoadTimers();
  unsigned int updated = 0, unchanged = 0;
  for (DvbTimers_t::iterator newtimer = newtimers.begin();
      newtimer != newtimers.end(); ++newtimer)
  {
    for (DvbTimers_t::iterator timer = m_timers.begin();
        timer != m_timers.end(); ++timer)
    {
      if (timer->guid != newtimer->guid)
        continue;

      if (timer->updateFrom(*newtimer))
      {
        timer->updateState = newtimer->updateState = DvbTimer::STATE_UPDATED;
        ++updated;
      }
      else
      {
        timer->updateState = newtimer->updateState = DvbTimer::STATE_FOUND;
        ++unchanged;
      }
    }
  }

  unsigned int removed = 0;
  for (DvbTimers_t::iterator it = m_timers.begin(); it != m_timers.end();)
  {
    if (it->updateState == DvbTimer::STATE_NONE)
    {
      XBMC->Log(LOG_DEBUG, "%s: Removed timer '%s': id=%u", __FUNCTION__,
          it->title.c_str(), it->id);
      it = m_timers.erase(it);
      ++removed;
    }
    else
      ++it;
  }

  unsigned int added = 0;
  for (DvbTimers_t::iterator it = newtimers.begin(); it != newtimers.end(); ++it)
  {
    if (it->updateState == DvbTimer::STATE_NEW)
    {
      it->id = m_nextTimerId;
      XBMC->Log(LOG_DEBUG, "%s: New timer '%s': id=%u", __FUNCTION__,
          it->title.c_str(), it->id);
      m_timers.push_back(*it);
      ++m_nextTimerId;
      ++added;
    }
  }

  XBMC->Log(LOG_DEBUG, "%s: Timers update: removed=%u, unchanged=%u, updated=%u, added=%u",
      __FUNCTION__, removed, unchanged, updated, added);

  if (removed || updated || added)
  {
    XBMC->Log(LOG_INFO, "Changes in timerlist detected, triggering an update!");
    PVR->TriggerTimerUpdate();
  }
}

DvbTimer *Dvb::GetTimer(const PVR_TIMER& timer)
{
  for (DvbTimers_t::iterator it = m_timers.begin(); it != m_timers.end(); ++it)
  {
    if (it->id == timer.iClientIndex)
      return &*it;
  }
  return NULL;
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

    if (updateSettings && g_groupRecordings != DvbRecording::GROUPING_DISABLED)
      m_recfolders.push_back(CStdString(xFolder->GetText()).ToLower());
  }

  if (updateSettings && g_groupRecordings != DvbRecording::GROUPING_DISABLED)
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

DvbChannel *Dvb::GetChannelByBackendId(const uint64_t backendId)
{
  for (DvbChannels_t::iterator it = m_channels.begin();
      it != m_channels.end(); ++it)
  {
    DvbChannel *channel = *it;
    for (std::list<uint64_t>::iterator it2 = channel->backendIds.begin();
        it2 != channel->backendIds.end(); it2++)
    {
      if (backendId == *it2)
        return channel;
    }
  }
  return NULL;
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
  const char *tmp = XBMC->UnknownToUTF8(src);
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
