#include "DvbData.h"
#include "client.h"
#include "platform/util/util.h"
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
  CStdString strAuth("");
  if (!g_strUsername.empty() && !g_strPassword.empty())
    strAuth.Format("%s:%s@", g_strUsername.c_str(), g_strPassword.c_str());
  m_strURL.Format("http://%s%s:%u/", strAuth.c_str(), g_strHostname.c_str(),
      g_iPortWeb);

  m_currentChannel     = 0;
  m_iClientIndexCounter = 1;

  m_iUpdateTimer  = 0;
  m_bUpdateTimers = false;
  m_bUpdateEPG    = false;
  m_tsBuffer      = NULL;
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

  XBMC->Log(LOG_INFO, "Starting separate client update thread...");
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
  m_bUpdateEPG = true;
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

    if (!g_bUseTimeshift)
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

PVR_ERROR Dvb::GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL& channel, time_t iStart, time_t iEnd)
{
  DvbChannel *myChannel = m_channels[channel.iUniqueId - 1];

  CStdString url(BuildURL("api/epg.html?lvl=2&channel=%"PRIu64"&start=%f&end=%f",
      myChannel->epgId, iStart/86400.0 + DELPHI_DATE, iEnd/86400.0 + DELPHI_DATE));
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
  XBMC->Log(LOG_DEBUG, "%s Number of EPG entries for channel '%s': %d",
    __FUNCTION__, channel.strChannelName, n);

  unsigned iNumEPG = 0;
  for (int i = 0; i < n; ++i)
  {
    DvbEPGEntry entry;
    entry.iChannelUid = channel.iUniqueId;

    XMLNode xTmp = xNode.getChildNode("programme", i);
    entry.startTime = ParseDateTime(xNode.getChildNode("programme", i).getAttribute("start"));
    entry.endTime = ParseDateTime(xNode.getChildNode("programme", i).getAttribute("stop"));

    if (iEnd > 1 && iEnd < entry.endTime)
       continue;

    if (!GetXMLValue(xTmp, "eventid", entry.iEventId))
      continue;

    if(!GetXMLValue(xTmp.getChildNode("titles"), "title", entry.strTitle, true))
      continue;

    CStdString strTmp, strTmp2;
    GetXMLValue(xTmp.getChildNode("descriptions"), "description", strTmp, true);
    GetXMLValue(xTmp.getChildNode("events"), "event", strTmp2, true);
    if (strTmp.length() > strTmp2.length())
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

    for(std::list<DvbChannel *>::iterator it = group->channels.begin();
        it != group->channels.end(); ++it)
    {
      DvbChannel *channel = *it;
      PVR_CHANNEL_GROUP_MEMBER tag;
      memset(&tag, 0, sizeof(PVR_CHANNEL_GROUP_MEMBER));
      PVR_STRCPY(tag.strGroupName, pvrGroup.strGroupName);
      tag.iChannelUniqueId = channel->id;
      tag.iChannelNumber   = channelNumberInGroup++;

      XBMC->Log(LOG_DEBUG, "%s add channel '%s' (%u) to group '%s'",
          __FUNCTION__, channel->name.c_str(), channel->backendNr,
          group->name.c_str());

      PVR->TransferChannelGroupMember(handle, &tag);
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

    tag.iClientChannelUid = timer->iChannelUid;
    tag.startTime         = timer->startTime;
    tag.endTime           = timer->endTime;
    PVR_STRCPY(tag.strTitle,     timer->strTitle.c_str());
    PVR_STRCPY(tag.strDirectory, "/");   // unused
    PVR_STRCPY(tag.strSummary,   timer->strPlot.c_str());
    tag.state             = timer->state;
    tag.iPriority         = timer->iPriority;
    tag.iLifetime         = 0;     // unused
    tag.bIsRepeating      = timer->bRepeating;
    tag.firstDay          = timer->iFirstDay;
    tag.iWeekdays         = timer->iWeekdays;
    tag.iEpgUid           = timer->iEpgId;
    tag.iMarginStart      = 0;     // unused
    tag.iMarginEnd        = 0;     // unused
    tag.iGenreType        = 0;     // unused
    tag.iGenreSubType     = 0;     // unused
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

  m_bUpdateTimers = true;
  return PVR_ERROR_NO_ERROR;
}

unsigned int Dvb::GetTimersAmount()
{
  return m_timers.size();
}


PVR_ERROR Dvb::GetRecordings(ADDON_HANDLE handle)
{
  CStdString url = BuildURL("api/recordings.html?images=1&nofilename=1");
  CStdString req = GetHttpXML(url);
  RemoveNullChars(req);

  XMLResults xe;
  XMLNode xMainNode = XMLNode::parseString(req, NULL, &xe);
  if (xe.error != 0)
  {
    XBMC->Log(LOG_ERROR, "Unable to parse recordings. Invalid XML. Error: %s",
        XMLNode::getError(xe.error));
    return PVR_ERROR_SERVER_ERROR;
  }

  XMLNode xNode = xMainNode.getChildNode("recordings");

  CStdString streamURL, imageURL;
  GetXMLValue(xNode, "serverURL", streamURL);
  GetXMLValue(xNode, "imageURL",  imageURL);

  int n = xNode.nChildNode("recording");
  XBMC->Log(LOG_DEBUG, "%s Number of recording entries: %d", __FUNCTION__, n);

  // there's no need to merge new recordings in older ones as XBMC does this
  // already for us (using strRecordingId). so just parse all recordings again
  m_recordings.clear();

  // insert recordings in reverse order
  for (int i = 0; i < n; ++i)
  {
    XMLNode xTmp = xNode.getChildNode("recording", n - i - 1);

    m_recordings.push_back(DvbRecording());
    DvbRecording *recording = &m_recordings.back();

    recording->id = xTmp.getAttribute("id");
    GetXMLValue(xTmp, "title",   recording->title);
    GetXMLValue(xTmp, "channel", recording->channelName);
    GetXMLValue(xTmp, "info",    recording->plotOutline);

    CStdString tmp;
    recording->plot = (GetXMLValue(xTmp, "desc", tmp)) ? tmp
      : recording->plotOutline;

    recording->streamURL = BuildExtURL(streamURL, "%s.ts",
        recording->id.c_str());

    if (GetXMLValue(xTmp, "image", tmp))
      recording->thumbnailPath = BuildExtURL(imageURL, "%s", tmp.c_str());

    CStdString startTime = xTmp.getAttribute("start");
    recording->startTime = ParseDateTime(startTime);

    int hours, mins, secs;
    sscanf(xTmp.getAttribute("duration"), "%02d%02d%02d", &hours, &mins, &secs);
    recording->duration = hours*60*60 + mins*60 + secs;

    // generate a more unique id
    recording->id += "_" + startTime;

    PVR_RECORDING tag;
    memset(&tag, 0, sizeof(PVR_RECORDING));
    PVR_STRCPY(tag.strRecordingId,   recording->id.c_str());
    PVR_STRCPY(tag.strTitle,         recording->title.c_str());
    PVR_STRCPY(tag.strStreamURL,     recording->streamURL.c_str());
    PVR_STRCPY(tag.strPlotOutline,   recording->plotOutline.c_str());
    PVR_STRCPY(tag.strPlot,          recording->plot.c_str());
    PVR_STRCPY(tag.strChannelName,   recording->channelName.c_str());
    PVR_STRCPY(tag.strThumbnailPath, recording->thumbnailPath.c_str());
    tag.recordingTime = recording->startTime;
    tag.iDuration     = recording->duration;
    PVR_STRCPY(tag.strDirectory, "/"); // unused

    PVR->TransferRecordingEntry(handle, &tag);

    XBMC->Log(LOG_DEBUG, "%s loaded Recording entry '%s': start=%u, length=%u",
        __FUNCTION__, recording->title.c_str(), recording->startTime,
        recording->duration);
  }

  XBMC->Log(LOG_INFO, "Loaded %u Recording Entries", m_recordings.size());

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
  return m_recordings.size();
}


bool Dvb::OpenLiveStream(const PVR_CHANNEL& channelinfo)
{
  XBMC->Log(LOG_DEBUG, "%s channel=%u", __FUNCTION__, channelinfo.iUniqueId);

  if (channelinfo.iUniqueId == m_currentChannel)
    return true;

  SwitchChannel(channelinfo);
  if (!g_bUseTimeshift)
    return true;

  if (m_tsBuffer)
    SAFE_DELETE(m_tsBuffer);
  XBMC->Log(LOG_INFO, "Timeshift starts; url=%s",
      GetLiveStreamURL(channelinfo).c_str());
  m_tsBuffer = new TimeshiftBuffer(GetLiveStreamURL(channelinfo),
      g_strTimeshiftBufferPath);
  return m_tsBuffer->IsValid();
}

void Dvb::CloseLiveStream(void)
{
  m_currentChannel = 0;
  if (m_tsBuffer)
    SAFE_DELETE(m_tsBuffer);
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

CStdString& Dvb::GetLiveStreamURL(const PVR_CHANNEL& channelinfo)
{
  SwitchChannel(channelinfo);
  return m_channels[channelinfo.iUniqueId - 1]->streamURL;
}


void *Dvb::Process()
{
  XBMC->Log(LOG_DEBUG, "%s starting", __FUNCTION__);

  while (!IsStopped())
  {
    Sleep(1000);
    ++m_iUpdateTimer;

    if (m_bUpdateEPG)
    {
      Sleep(8000); /* Sleep enough time to let the recording service grab the EPG data */
      PVR->TriggerEpgUpdate(m_currentChannel);
      m_bUpdateEPG = false;
    }

    if (m_iUpdateTimer > 60 || m_bUpdateTimers)
    {
      m_iUpdateTimer = 0;

      // Trigger Timer and Recording updates acording to the addon settings
      CLockObject lock(m_mutex);
      XBMC->Log(LOG_INFO, "Perform timer/recording updates!");

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

  return NULL;
}


CStdString Dvb::GetHttpXML(const CStdString& url)
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

CStdString Dvb::URLEncodeInline(const CStdString& strData)
{
  /* Copied from xbmc/URL.cpp */
  CStdString strResult;

  /* wonder what a good value is here is, depends on how often it occurs */
  strResult.reserve(strData.length() * 2);

  for (unsigned int i = 0; i < strData.length(); ++i)
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

bool Dvb::LoadChannels()
{
  CStdString url = BuildURL("api/getchannelsxml.html?subchannels=1&rtsp=1&upnp=1&logo=1");
  CStdString req = GetHttpXML(url);

  XMLResults xe;
  XMLNode xMainNode = XMLNode::parseString(req, NULL, &xe);
  if (xe.error != 0)
  {
    XBMC->Log(LOG_ERROR, "Unable to parse channels. Invalid XML. Error: %s",
        XMLNode::getError(xe.error));
    return false;
  }

  XMLNode xChannels = xMainNode.getChildNode("channels");

  CStdString streamURL;
  GetXMLValue(xChannels, (g_useRTSP) ? "rtspURL" : "upnpURL", streamURL);

  m_channels.clear();
  m_channelAmount = 0;
  m_groups.clear();
  m_groupAmount = 0;

  int n = xChannels.nChildNode("root");
  for (int i = 0; i < n; ++i)
  {
    XMLNode xRoot = xChannels.getChildNode("root", i);
    int num_groups = xRoot.nChildNode("group");
    for (int j = 0; j < num_groups; ++j)
    {
      XMLNode xGroup = xRoot.getChildNode("group", j);

      m_groups.push_back(DvbGroup());
      DvbGroup *group = &m_groups.back();
      group->name     = xGroup.getAttribute("name");
      group->hidden   = g_useFavourites;
      group->radio    = true;
      if (!group->hidden)
        ++m_groupAmount;

      int num_channels = xGroup.nChildNode("channel");
      for (int k = 0; k < num_channels; ++k)
      {
        XMLNode xChannel = xGroup.getChildNode("channel", k);

        DvbChannel *channel = new DvbChannel();
        int flags  = atoi(xChannel.getAttribute("flags"));
        channel->radio      = !(flags & VIDEO_FLAG);
        channel->encrypted  = (flags & ENCRYPTED_FLAG);
        channel->name       = xChannel.getAttribute("name");
        channel->backendNr  = atoi(xChannel.getAttribute("nr"));
        channel->epgId      = ParseUInt64(xChannel.getAttribute("EPGID"));
        channel->hidden     = g_useFavourites;
        channel->frontendNr = (!g_useFavourites) ? m_channels.size() + 1 : 0;
        channel->backendIds.push_back(ParseUInt64(xChannel.getAttribute("ID")));

        CStdString logoURL;
        if (GetXMLValue(xChannel, "logo", logoURL))
          channel->logoURL = BuildURL("%s", logoURL.c_str());

        if (g_useRTSP)
        {
          CStdString urlParams;
          GetXMLValue(xChannel, "rtsp", urlParams);
          channel->streamURL = BuildExtURL(streamURL, "%s", urlParams.c_str());
        }
        else
          channel->streamURL = BuildExtURL(streamURL, "%u.ts", channel->backendNr);

        int num_subchannels = xChannel.nChildNode("subchannel");
        for (int l = 0; l < num_subchannels; ++l)
        {
          XMLNode xSubChannel = xChannel.getChildNode("subchannel", l);
          channel->backendIds.push_back(ParseUInt64(xSubChannel.getAttribute("ID")));
        }

        //FIXME: PVR_CHANNEL.UniqueId is uint32 but DVB Viewer ids are uint64
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
        //TODO: print error instead of loading favourites
        return false;
      }
      url = g_favouritesFile;
    }

    CStdString req = GetHttpXML(url);
    RemoveNullChars(req);

    XMLResults xe;
    XMLNode xMainNode = XMLNode::parseString(req, NULL, &xe);
    if (xe.error != 0)
    {
      XBMC->Log(LOG_ERROR, "Unable to parse favourites.xml. Error: %s",
          XMLNode::getError(xe.error));
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
    XMLNode xSettings = xMainNode.getChildNode("settings");
    int n = xSettings.nChildNode("section");
    for (int i = 0; i < n; ++i)
    {
      DvbGroup *group = NULL;
      XMLNode xSection = xSettings.getChildNode("section", i);
      int m = xSection.nChildNode("entry");
      for (int j = 0; j < m; ++j)
      {
        XMLNode xEntry = xSection.getChildNode("entry", j);
        // name="Header" doesn't indicate a group alone. see example above
        if (CStdString(xEntry.getAttribute("name")) == "Header" && m > 1)
        {
          m_groups.push_back(DvbGroup());
          group = &m_groups.back();
          group->name   = ConvertToUtf8(xEntry.getText());
          group->hidden = false;
          group->radio  = false;
          ++m_groupAmount;
          continue;
        }

        uint64_t backendId = 0;
        std::istringstream ss(xEntry.getText());
        ss >> backendId;

        //TODO: use getchannelbybackendid?
        for (DvbChannels_t::iterator it = m_channels.begin();
            it != m_channels.end(); ++it)
        {
          DvbChannel *channel = *it;
          bool found = false;

          for(std::list<uint64_t>::iterator it2 = channel->backendIds.begin();
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

  CStdString url(BuildURL("api/timerlist.html?utf8"));
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
  XBMC->Log(LOG_DEBUG, "%s Number of timer entries: %d", __FUNCTION__, n);

  for (int i = 0; i < n; ++i)
  {
    XMLNode xTmp = xNode.getChildNode("Timer", n - i - 1);

    CStdString strTmp;
    if (GetXMLValue(xTmp, "Descr", strTmp))
      XBMC->Log(LOG_DEBUG, "%s Processing timer '%s'", __FUNCTION__, strTmp.c_str());

    DvbTimer timer;
    timer.strTitle = strTmp;
    timer.iChannelUid = GetChannelUid(xTmp.getChildNode("Channel").getAttribute("ID"));
    if (timer.iChannelUid == 0)
      continue;
    timer.state = PVR_TIMER_STATE_SCHEDULED;

    CStdString DateTime = xTmp.getAttribute("Date");
    DateTime.append(xTmp.getAttribute("Start"));
    timer.startTime = ParseDateTime(DateTime, false);
    timer.endTime = timer.startTime + atoi(xTmp.getAttribute("Dur"))*60;

    CStdString Weekdays = xTmp.getAttribute("Days");
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

    timer.iPriority = atoi(xTmp.getAttribute("Priority"));

    if (xTmp.getAttribute("EPGEventID"))
      timer.iEpgId = atoi(xTmp.getAttribute("EPGEventID"));

    if (xTmp.getAttribute("Enabled")[0] == '0')
      timer.state = PVR_TIMER_STATE_CANCELLED;

    int iTmp;
    if (GetXMLValue(xTmp, "Recording", iTmp))
    {
      if (iTmp == -1)
        timer.state = PVR_TIMER_STATE_RECORDING;
    }

    if (GetXMLValue(xTmp, "ID", iTmp))
      timer.iTimerId = iTmp;

    timers.push_back(timer);

    XBMC->Log(LOG_DEBUG, "%s loaded Timer entry '%s': start=%u, end=%u",
        __FUNCTION__, timer.strTitle.c_str(), timer.startTime, timer.endTime);
  }

  XBMC->Log(LOG_INFO, "Loaded %u Timer entries", timers.size());
  return timers;
}

void Dvb::TimerUpdates()
{
  for (DvbTimers_t::iterator timer = m_timers.begin();
      timer != m_timers.end(); ++timer)
    timer->iUpdateState = DVB_UPDATE_STATE_NONE;

  DvbTimers_t newtimers = LoadTimers();
  unsigned int iUpdated = 0;
  unsigned int iUnchanged = 0;
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
        ++iUnchanged;
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
        ++iUpdated;
      }
    }
  }

  unsigned int iRemoved = 0;
  for (DvbTimers_t::iterator it = m_timers.begin(); it != m_timers.end();)
  {
    if (it->iUpdateState == DVB_UPDATE_STATE_NONE)
    {
      XBMC->Log(LOG_DEBUG, "%s Removed timer: '%s', ClientIndex: %u",
          __FUNCTION__, it->strTitle.c_str(), it->iClientIndex);
      it = m_timers.erase(it);
      ++iRemoved;
    }
    else
      ++it;
  }

  unsigned int iNew = 0;
  for (DvbTimers_t::iterator it = newtimers.begin(); it != newtimers.end(); ++it)
  {
    if (it->iUpdateState == DVB_UPDATE_STATE_NEW)
    {
      it->iClientIndex = m_iClientIndexCounter;
      XBMC->Log(LOG_DEBUG, "%s New timer: '%s', ClientIndex: %u",
          __FUNCTION__, it->strTitle.c_str(), m_iClientIndexCounter);
      m_timers.push_back(*it);
      ++m_iClientIndexCounter;
      ++iNew;
    }
  }

  XBMC->Log(LOG_DEBUG, "%s Timers update: removed=%u, untouched=%u, updated=%u, new=%u",
      __FUNCTION__, iRemoved, iUnchanged, iUpdated, iNew);

  if (iRemoved || iUpdated || iNew)
  {
    XBMC->Log(LOG_INFO, "Changes in timerlist detected, triggering an update!");
    PVR->TriggerTimerUpdate();
  }
}

void Dvb::GenerateTimer(const PVR_TIMER& timer, bool bNewTimer)
{
  XBMC->Log(LOG_DEBUG, "%s iChannelUid=%u title='%s' epgid=%d",
      __FUNCTION__, timer.iClientChannelUid, timer.strTitle, timer.iEpgUid);

  struct tm *timeinfo;
  time_t startTime = timer.startTime, endTime = timer.endTime;
  if (!startTime)
    time(&startTime);
  else
  {
    startTime -= timer.iMarginStart * 60;
    endTime += timer.iMarginEnd * 60;
  }

  int dor = ((startTime + m_timezone * 60) / DAY_SECS) + DELPHI_DATE;
  timeinfo = localtime(&startTime);
  int start = timeinfo->tm_hour * 60 + timeinfo->tm_min;
  timeinfo = localtime(&endTime);
  int stop = timeinfo->tm_hour * 60 + timeinfo->tm_min;

  char strWeek[8] = "-------";
  for (int i = 0; i < 7; ++i)
  {
    if (timer.iWeekdays & (1 << i))
      strWeek[i] = 'T';
  }

  uint64_t iChannelId = m_channels[timer.iClientChannelUid - 1]->backendIds.front();
  CStdString url;
  if (bNewTimer)
    url = BuildURL("api/timeradd.html?ch=%"PRIu64"&dor=%d&enable=1&start=%d&stop=%d&prio=%d&days=%s&title=%s&encoding=255",
        iChannelId, dor, start, stop, timer.iPriority, strWeek, URLEncodeInline(timer.strTitle).c_str());
  else
  {
    int enabled = (timer.state == PVR_TIMER_STATE_CANCELLED) ? 0 : 1;
    url = BuildURL("api/timeredit.html?id=%d&ch=%"PRIu64"&dor=%d&enable=%d&start=%d&stop=%d&prio=%d&days=%s&title=%s&encoding=255",
        GetTimerId(timer), iChannelId, dor, enabled, start, stop, timer.iPriority, strWeek, URLEncodeInline(timer.strTitle).c_str());
  }

  GetHttpXML(url);
  m_bUpdateTimers = true;
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


bool Dvb::GetXMLValue(const XMLNode& node, const char* tag, int& value)
{
  XMLNode xNode(node.getChildNode(tag));
  if (xNode.isEmpty())
     return false;
  value = atoi(xNode.getText());
  return true;
}

bool Dvb::GetXMLValue(const XMLNode& node, const char* tag, bool& value)
{
  XMLNode xNode(node.getChildNode(tag));
  if (xNode.isEmpty())
    return false;

  CStdString str(xNode.getText());
  str.ToLower();
  if (str == "off" || str == "no" || str == "disabled" || str == "false" || str == "0" )
    value = false;
  else
  {
    value = true;
    if (str != "on" && str != "yes" && str != "enabled" && str != "true")
      return false; // invalid bool switch - it's probably some other string.
  }
  return true;
}

bool Dvb::GetXMLValue(const XMLNode& node, const char* tag, CStdString& value,
    bool localize)
{
  XMLNode xNode;
  bool found = false;

  for (int i = 0; localize && i < node.nChildNode(tag); ++i)
  {
    xNode = node.getChildNode(tag, i);
    const char *lang = xNode.getAttribute("lng");
    if (lang && lang == m_epgLanguage)
    {
      found = true;
      break;
    }
  }

  if (!found)
    xNode = node.getChildNode(tag);
  if (!xNode.isEmpty())
  {
    value = xNode.getText();
    return true;
  }
  value.clear();
  return false;
}

void Dvb::RemoveNullChars(CStdString& str)
{
  /* favourites.xml and timers.xml sometimes have null chars that screw the xml */
  str.erase(std::remove(str.begin(), str.end(), '\0'), str.end());
}

bool Dvb::CheckBackendVersion()
{
  CStdString url(BuildURL("api/version.html"));
  CStdString strXML(GetHttpXML(url));

  XMLResults xe;
  XMLNode xMainNode = XMLNode::parseString(strXML, NULL, &xe);
  if (xe.error != 0)
  {
    XBMC->Log(LOG_ERROR, "Unable to connect to the recording service");
    XBMC->QueueNotification(QUEUE_ERROR, XBMC->GetLocalizedString(30500));
    Sleep(10000);
    return false;
  }

  XBMC->Log(LOG_NOTICE, "Checking backend version...");
  XMLNode xNode = xMainNode.getChildNode("version");
  if (xNode.isEmpty())
  {
    XBMC->Log(LOG_ERROR, "Could not parse version from result!");
    return false;
  }
  XBMC->Log(LOG_NOTICE, "Version: %s", xNode.getText());

  XMLCSTR strVersion = xNode.getAttribute("iver");
  if (strVersion)
  {
    std::istringstream ss(strVersion);
    ss >> m_backendVersion;
  }

  if (m_backendVersion < RS_VERSION_NUM)
  {
    XBMC->Log(LOG_ERROR, "Recording Service version %s or higher required", RS_VERSION_STR);
    XBMC->QueueNotification(QUEUE_ERROR, XBMC->GetLocalizedString(30501), RS_VERSION_STR);
    Sleep(10000);
    return false;
  }

  return true;
}

bool Dvb::UpdateBackendStatus(bool updateSettings)
{
  CStdString url(BuildURL("api/status.html"));
  CStdString strXML(GetHttpXML(url));

  XMLResults xe;
  XMLNode xMainNode = XMLNode::parseString(strXML, NULL, &xe);
  if (xe.error != 0)
  {
    XBMC->Log(LOG_ERROR, "Unable to get backend status. Invalid XML. Error: %s",
        XMLNode::getError(xe.error));
    return false;
  }

  XMLNode xNode = xMainNode.getChildNode("status");
  if (updateSettings)
  {
    GetXMLValue(xNode, "timezone", m_timezone);
    GetXMLValue(xNode, "epglang", m_epgLanguage);
  }

  // compute disk space. duplicates are detected by their identical values
  typedef std::pair<long long, long long> Recfolder_t;
  std::set<Recfolder_t> folders;
  XMLNode recfolders = xNode.getChildNode("recfolders");
  int n = recfolders.nChildNode("folder");
  m_diskspace.total = m_diskspace.used = 0;
  for (int i = 0; i < n; ++i)
  {
    XMLNode folder = recfolders.getChildNode("folder", i);

    long long size = 0, free = 0;
    std::istringstream ss(folder.getAttribute("size"));
    ss >> size;
    ss.clear();
    ss.str(folder.getAttribute("free"));
    ss >> free;

    if (folders.insert(std::make_pair(size, free)).second)
    {
      m_diskspace.total += size / 1024;
      m_diskspace.used += (size - free) / 1024;
    }
  }
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
    for(std::list<uint64_t>::iterator backendId = channel->backendIds.begin();
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
  CStdString url(m_strURL);
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
  if (!g_strUsername.empty() && !g_strPassword.empty())
  {
    CStdString strAuth;
    strAuth.Format("%s:%s@", g_strUsername.c_str(), g_strPassword.c_str());
    CStdString::size_type pos = url.find("://");
    if (pos != CStdString::npos)
      url.insert(pos + strlen("://"), strAuth);
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

uint64_t Dvb::ParseUInt64(const CStdString& str)
{
  uint64_t value = 0;
  std::istringstream ss(str);
  ss >> value;
  if (ss.fail())
    return 0;
  return value;
}
