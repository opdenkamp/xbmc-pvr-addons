#include "DvbData.h"
#include "client.h"
#include "platform/util/util.h"
#include <inttypes.h>
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

  m_iCurrentChannel     = 0;
  m_iClientIndexCounter = 1;

  m_iUpdateTimer  = 0;
  m_bUpdateTimers = false;
  m_bUpdateEPG    = false;
  m_tsBuffer      = NULL;
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


CStdString Dvb::GetServerName()
{
  return m_strServerName;
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

bool Dvb::IsConnected()
{
  return m_bIsConnected;
}

PVR_ERROR Dvb::SignalStatus(PVR_SIGNAL_STATUS& signalStatus)
{
  CStdString strXML, url;
  url.Format("%sstatus.html?aktion=status", m_strURL);
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


bool Dvb::SwitchChannel(const PVR_CHANNEL& channel)
{
  m_iCurrentChannel = channel.iUniqueId;
  m_bUpdateEPG = true;
  return true;
}

unsigned int Dvb::GetCurrentClientChannel(void)
{
  return m_iCurrentChannel;
}

PVR_ERROR Dvb::GetChannels(ADDON_HANDLE handle, bool bRadio)
{
  for (DvbChannels_t::iterator channel = m_channels.begin();
      channel != m_channels.end(); ++channel)
  {
    if (channel->bRadio != bRadio)
      continue;

    PVR_CHANNEL xbmcChannel;
    memset(&xbmcChannel, 0, sizeof(PVR_CHANNEL));

    xbmcChannel.iUniqueId         = channel->iUniqueId;
    xbmcChannel.bIsRadio          = channel->bRadio;
    xbmcChannel.iChannelNumber    = channel->iChannelNumber;
    xbmcChannel.iEncryptionSystem = channel->Encrypted;
    xbmcChannel.bIsHidden         = false;
    PVR_STRCPY(xbmcChannel.strChannelName, channel->strChannelName.c_str());
    PVR_STRCPY(xbmcChannel.strInputFormat, ""); // unused
    PVR_STRCPY(xbmcChannel.strIconPath,    channel->strIconPath.c_str());

    if (!g_bUseTimeshift)
    {
      CStdString strStream;
      strStream.Format("pvr://stream/tv/%u.ts", channel->iUniqueId);
      PVR_STRCPY(xbmcChannel.strStreamURL, strStream.c_str());
    }

    PVR->TransferChannelEntry(handle, &xbmcChannel);
  }

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR Dvb::GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL& channel, time_t iStart, time_t iEnd)
{
  DvbChannel &myChannel = m_channels[channel.iUniqueId - 1];

  CStdString url;
  url.Format("%sapi/epg.html?lvl=2&channel=%"PRIu64"&start=%f&end=%f", m_strURL,
      myChannel.iEpgId, iStart/86400.0 + DELPHI_DATE, iEnd/86400.0 + DELPHI_DATE);

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
  return m_channels.size();
}


PVR_ERROR Dvb::GetChannelGroups(ADDON_HANDLE handle)
{
  for (DvbChannelGroups_t::iterator group = m_groups.begin();
      group != m_groups.end(); ++group)
  {
    PVR_CHANNEL_GROUP tag;
    memset(&tag, 0, sizeof(PVR_CHANNEL_GROUP));
    tag.bIsRadio = false;
    PVR_STRCPY(tag.strGroupName, group->getName().c_str());

    PVR->TransferChannelGroup(handle, &tag);
  }

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR Dvb::GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP& group)
{
  CStdString groupName(group.strGroupName);
  for (DvbChannels_t::iterator channel = m_channels.begin();
      channel != m_channels.end(); ++channel)
  {
    if (channel->group == groupName)
    {
      PVR_CHANNEL_GROUP_MEMBER tag;
      memset(&tag, 0, sizeof(PVR_CHANNEL_GROUP_MEMBER));
      PVR_STRCPY(tag.strGroupName, group.strGroupName);
      tag.iChannelUniqueId = channel->iUniqueId;
      tag.iChannelNumber   = channel->iChannelNumber;

      XBMC->Log(LOG_DEBUG, "%s add channel '%s' (%u) to group '%s'",
          __FUNCTION__, channel->strChannelName.c_str(), channel->iUniqueId,
          group.strGroupName);

      PVR->TransferChannelGroupMember(handle, &tag);
    }
  }
  return PVR_ERROR_NO_ERROR;
}

unsigned int Dvb::GetChannelGroupsAmount()
{
  return m_groups.size();
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
  CStdString strTmp;
  strTmp.Format("api/timerdelete.html?id=%d", GetTimerId(timer));
  SendSimpleCommand(strTmp);

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
  DvbRecordings_t old_recordings(m_recordings);
  m_recordings.clear();

  CStdString url;
  url.Format("%sapi/recordings.html?utf8", m_strURL);

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
  XBMC->Log(LOG_DEBUG, "%s Number of recording entries: %d", __FUNCTION__, n);

  unsigned int iNumRecording = 0, fetched = 0;
  for (int i = 0; i < n; ++i)
  {
    DvbRecording recording;
    XMLNode xTmp = xNode.getChildNode("recording", n - i - 1);

    if (xTmp.getAttribute("id"))
      recording.strRecordingId = xTmp.getAttribute("id");

    CStdString strTmp;
    if (GetXMLValue(xTmp, "title", strTmp))
      recording.strTitle = strTmp;

    CStdString strTmp2;
    GetXMLValue(xTmp, "desc", strTmp);
    GetXMLValue(xTmp, "info", strTmp2);
    if (strTmp.size() > strTmp2.size())
      recording.strPlot = strTmp;
    else
      recording.strPlot = strTmp2;

    if (GetXMLValue(xTmp, "channel", strTmp))
      recording.strChannelName = strTmp;

    strTmp.Format("%supnp/recordings/%d.ts", m_strURLRecording, atoi(recording.strRecordingId.c_str()));
    recording.strStreamURL = strTmp;

    recording.startTime = ParseDateTime(xTmp.getAttribute("start"));

    int hours, mins, secs;
    sscanf(xTmp.getAttribute("duration"), "%02d%02d%02d", &hours, &mins, &secs);
    recording.iDuration = hours*60*60 + mins*60 + secs;

    bool bGetThumbnails = (fetched < MAX_RECORDING_THUMBS);
    for (DvbRecordings_t::iterator old_rec = old_recordings.begin();
        bGetThumbnails && old_rec != old_recordings.end(); ++old_rec)
    {
      if (old_rec->strRecordingId == recording.strRecordingId
          && old_rec->strThumbnailPath.size() > 20
          && old_rec->strThumbnailPath.size() < 100)
      {
        recording.strThumbnailPath = old_rec->strThumbnailPath;
        bGetThumbnails = false;
        break;
      }
    }

    if (bGetThumbnails)
    {
      url.Format("%sepg_details.html?aktion=epg_details&recID=%s", m_strURL, recording.strRecordingId);
      CStdString strThumb;
      strThumb = GetHttpXML(url);
      ++fetched;

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

    ++iNumRecording;
    m_recordings.push_back(recording);

    XBMC->Log(LOG_DEBUG, "%s loaded Recording entry '%s': start=%u, length=%u",
        __FUNCTION__, tag.strTitle, recording.startTime, recording.iDuration);
  }

  XBMC->Log(LOG_INFO, "Loaded %u Recording Entries", iNumRecording);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR Dvb::DeleteRecording(const PVR_RECORDING& recinfo)
{
  CStdString strTmp;
  strTmp.Format("rec_list.html?aktion=delete_rec&recid=%s", recinfo.strRecordingId);
  SendSimpleCommand(strTmp);

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

  if (channelinfo.iUniqueId == m_iCurrentChannel)
    return true;

  if (!g_bUseTimeshift)
    return SwitchChannel(channelinfo);

  if (m_tsBuffer)
    SAFE_DELETE(m_tsBuffer);
  XBMC->Log(LOG_INFO, "Timeshifting starts; url=%s",
      GetLiveStreamURL(channelinfo).c_str());
  m_tsBuffer = new TimeshiftBuffer(GetLiveStreamURL(channelinfo),
      g_strTimeshiftBufferPath);
  return m_tsBuffer->IsValid();
}

void Dvb::CloseLiveStream(void)
{
  m_iCurrentChannel = 0;
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

CStdString Dvb::GetLiveStreamURL(const PVR_CHANNEL& channelinfo)
{
  SwitchChannel(channelinfo);
  return m_channels[channelinfo.iUniqueId - 1].strStreamURL;
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
      PVR->TriggerEpgUpdate(m_iCurrentChannel);
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

  for (unsigned int i = 0; i < strData.size(); ++i)
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

void Dvb::SendSimpleCommand(const CStdString& strCommandURL)
{
  CStdString url;
  url.Format("%s%s", m_strURL, strCommandURL);
  GetHttpXML(url);
}

bool Dvb::LoadChannels()
{
  m_groups.clear();

  CStdString url;
  url.Format("%sapi/getchannelsdat.html", m_strURL);
  CStdString strBIN(GetHttpXML(url));
  if (strBIN.IsEmpty())
  {
    XBMC->Log(LOG_ERROR, "Unable to load channels.dat");
    return false;
  }
  ChannelsDat *channel = (ChannelsDat*)(strBIN.data() + CHANNELDAT_HEADER_SIZE);
  int num_channels = strBIN.size() / sizeof(ChannelsDat);

  DvbChannels_t channels;
  DvbChannelGroups_t groups;
  DvbChannelGroup channelGroup("");
  int channel_pos = 0;

  for (int i = 0; i < num_channels; ++i, channel++)
  {
    if (channel->Flags & ADDITIONAL_AUDIO_TRACK_FLAG)
    {
      /* skip audio only channels (e.g. AC3, other languages, etc..) */
      if (!g_bUseFavourites || !(channel->Flags & VIDEO_FLAG))
        continue;
    }
    else
      ++channel_pos;

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
    dvbChannel.iEpgId = ((uint64_t)(channel->TunerType + 1) << 48)
      + ((uint64_t)channel->OriginalNetwork_ID << 32)
      + ((uint64_t)channel->TransportStream_ID << 16)
      + channel->Service_ID;

    /* 32bit ChannelID = (tunertype + 1) * 536870912 + APID * 65536 + SID */
    //dvbChannel.iChannelId     = ((channel->TunerType + 1) << 29) + (channel->Audio_PID << 16) + channel->Service_ID;

    /* 2 Bit: 0 = undefined, 1 = tv, 2 = radio */
    short tv_radio = (channel->Flags & VIDEO_FLAG) ? 1
      : (channel->Flags & AUDIO_FLAG) ? 2 : 0;
    /* 64bit ChannelID = SID + APID*2^16 + (tunertyp + 1)*2^29 + TID*2^32 + (OrbitalPosition*10)*2^48 + TV/Radio-Flag*2^61  */
    dvbChannel.iChannelId = ((uint64_t)tv_radio << 61)
      + ((uint64_t)channel->OrbitalPos << 48)
      + ((uint64_t)channel->TransportStream_ID << 32)
      + (((uint64_t)channel->TunerType + 1) << 29)
      + ((uint64_t)channel->Audio_PID << 16)
      + channel->Service_ID;

    dvbChannel.Encrypted      = channel->Flags & ENCRYPTED_FLAG;
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

    dvbChannel.strChannelName = ConvertToUtf8(strChannelName);

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

    DvbChannels_t channelsfav;
    DvbChannelGroups_t groupsfav;
    DvbChannel favChannel;
    DvbChannelGroup favGroup("");

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
    for (int i = 0; i < m; ++i)
    {
      XMLNode xTmp = xNode.getChildNode("section", i);
      int n = xTmp.nChildNode("entry");
      for (int j = 0; j < n; ++j)
      {
        XMLNode xEntry = xTmp.getChildNode("entry", j);
        /* name="Header" doesn't indicate a group alone. see example above */
        if (CStdString(xEntry.getAttribute("name")) == "Header" && n > 1)
        {
          /* it's a group */
          char *strGroupNameUtf8 = XBMC->UnknownToUTF8(xEntry.getText());
          favGroup.setName(strGroupNameUtf8);
          groupsfav.push_back(favGroup);
          XBMC->FreeString(strGroupNameUtf8);
        }
        else
        {
          CStdString channelName;
          uint64_t favChannelId = ParseChannelString(xEntry.getText(),
              channelName);
          if (favChannelId == 0)
            continue;

          for (DvbChannels_t::iterator channel = channels.begin();
              channel != channels.end(); ++channel)
          {
            /* legacy support for old 32bit channel ids */
            uint64_t channelId = (favChannelId > 0xFFFFFFFF) ? channel->iChannelId :
              channel->iChannelId & 0xFFFFFFFF;
            if (channelId == favChannelId)
            {
              favChannel.iEpgId         = channel->iEpgId;
              favChannel.Encrypted      = channel->Encrypted;
              favChannel.iChannelId     = channel->iChannelId;
              favChannel.bRadio         = channel->bRadio;
              favChannel.group          = favGroup;
              favChannel.iUniqueId      = channelsfav.size() + 1;
              favChannel.iChannelNumber = channelsfav.size() + 1;
              favChannel.strChannelName = (!channelName.empty()) ? channelName
                : channel->strChannelName;
              favChannel.strStreamURL   = channel->strStreamURL;
              favChannel.strIconPath    = channel->strIconPath;
              channelsfav.push_back(favChannel);
              break;
            }
          }
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

  for (DvbChannels_t::iterator channel = m_channels.begin();
      channel != m_channels.end(); ++channel)
  {
    CStdString name(channel->strChannelName), iconPath;
    std::replace(name.begin(), name.end(), '/', ' ');
    name.erase(name.find_last_not_of(".") + 1);
    iconPath.Format("%sLogos/%s.png", m_strURL, URLEncodeInline(name));
    channel->strIconPath = iconPath;
  }

  XBMC->Log(LOG_INFO, "Loaded %u channels in %u groups", m_channels.size(), m_groups.size());
  return true;
}

DvbTimers_t Dvb::LoadTimers()
{
  DvbTimers_t timers;

  CStdString url;
  url.Format("%sapi/timerlist.html?utf8", m_strURL);

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
    for (unsigned int j = 0; j < Weekdays.size(); ++j)
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

  int dor = ((startTime + m_iTimezone * 60) / DAY_SECS) + DELPHI_DATE;
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

  uint64_t iChannelId = m_channels[timer.iClientChannelUid - 1].iChannelId;
  CStdString strTmp;
  if (bNewTimer)
    strTmp.Format("api/timeradd.html?ch=%"PRIu64"&dor=%d&enable=1&start=%d&stop=%d&prio=%d&days=%s&title=%s&encoding=255",
        iChannelId, dor, start, stop, timer.iPriority, strWeek, URLEncodeInline(timer.strTitle));
  else
  {
    int enabled = (timer.state == PVR_TIMER_STATE_CANCELLED) ? 0 : 1;
    strTmp.Format("api/timeredit.html?id=%d&ch=%"PRIu64"&dor=%d&enable=%d&start=%d&stop=%d&prio=%d&days=%s&title=%s&encoding=255",
        GetTimerId(timer), iChannelId, dor, enabled, start, stop, timer.iPriority, strWeek, URLEncodeInline(timer.strTitle));
  }

  SendSimpleCommand(strTmp);
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
    if (lang && lang == m_strEPGLanguage)
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
  value.Empty();
  return false;
}

void Dvb::GetPreferredLanguage()
{
  CStdString strXML, url;
  url.Format("%sconfig.html?aktion=config", m_strURL);
  strXML = GetHttpXML(url);
  unsigned int iLanguagePos = strXML.find("EPGLanguage");
  iLanguagePos = strXML.find(" selected>", iLanguagePos);
  m_strEPGLanguage = strXML.substr(iLanguagePos - 4, 3);
}

void Dvb::GetTimeZone()
{
  CStdString url;
  url.Format("%sapi/status.html", m_strURL);

  CStdString strXML(GetHttpXML(url));

  XMLResults xe;
  XMLNode xMainNode = XMLNode::parseString(strXML, NULL, &xe);
  if (xe.error != 0)
    XBMC->Log(LOG_ERROR, "Unable to get recording service status. Invalid XML. Error: %s", XMLNode::getError(xe.error));
  else
  {
    XMLNode xNode = xMainNode.getChildNode("status");
    GetXMLValue(xNode, "timezone", m_iTimezone);
  }
}

void Dvb::RemoveNullChars(CStdString& str)
{
  /* favourites.xml and timers.xml sometimes have null chars that screw the xml */
  str.erase(std::remove(str.begin(), str.end(), '\0'), str.end());
}

bool Dvb::GetDeviceInfo()
{
  CStdString url;
  url.Format("%sapi/version.html", m_strURL);

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
  for (DvbChannels_t::iterator channel = m_channels.begin();
      channel != m_channels.end(); ++channel)
  {
    if (channel->iChannelId == channelId)
      return channel->iUniqueId;
  }
  return 0;
}

CStdString Dvb::ConvertToUtf8(const CStdString& src)
{
  char *tmp = XBMC->UnknownToUTF8(src);
  CStdString dest(tmp);
  XBMC->FreeString(tmp);
  return dest;
}
