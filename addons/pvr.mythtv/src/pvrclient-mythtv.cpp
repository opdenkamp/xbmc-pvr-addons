/*
 *      Copyright (C) 2005-2014 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301 USA
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "pvrclient-mythtv.h"
#include "client.h"
#include "tools.h"

#include <time.h>
#include <set>

using namespace ADDON;
using namespace PLATFORM;

PVRClientMythTV::PVRClientMythTV()
: m_eventHandler(NULL)
, m_control(NULL)
, m_wsapi(NULL)
, m_liveStream(NULL)
, m_recordingStream(NULL)
, m_fileOps(NULL)
, m_scheduleManager(NULL)
, m_categories()
, m_channelGroups()
, m_demux(NULL)
, m_recordingChangePinCount(0)
{
}

PVRClientMythTV::~PVRClientMythTV()
{
  SAFE_DELETE(m_demux);
  SAFE_DELETE(m_fileOps);
  SAFE_DELETE(m_scheduleManager);
  SAFE_DELETE(m_eventHandler);
  SAFE_DELETE(m_wsapi);
  SAFE_DELETE(m_control);
}

void Log(int level, char *msg)
{
  if (msg && level != MYTH_DBG_NONE)
  {
    bool doLog = true; //g_bExtraDebug;
    addon_log_t loglevel = LOG_DEBUG;
    switch (level)
    {
    case MYTH_DBG_ERROR:
      loglevel = LOG_ERROR;
      doLog = true;
      break;
    case MYTH_DBG_WARN:
      loglevel = LOG_NOTICE;
      doLog = true;
      break;
    case MYTH_DBG_INFO:
      loglevel = LOG_INFO;
      doLog = true;
      break;
    case MYTH_DBG_DEBUG:
    case MYTH_DBG_PROTO:
    case MYTH_DBG_ALL:
      loglevel = LOG_DEBUG;
      break;
    }
    if (XBMC && doLog)
      XBMC->Log(loglevel, "%s", msg);
  }
}

void PVRClientMythTV::SetDebug()
{
  // Setup libcppmyth logging
  if (g_bExtraDebug)
    Myth::DBGAll();
  else
    Myth::DBGLevel(MYTH_DBG_ERROR);
  Myth::SetDBGMsgCallback(Log);
}

bool PVRClientMythTV::Connect()
{
  SetDebug();
  m_control = new Myth::Control(g_szMythHostname, g_iProtoPort);
  if (!m_control->IsOpen())
  {
    SAFE_DELETE(m_control);
    XBMC->Log(LOG_ERROR, "Failed to connect to MythTV backend on %s:%d", g_szMythHostname.c_str(), g_iProtoPort);
    return false;
  }
  m_wsapi = new Myth::WSAPI(g_szMythHostname, g_iWSApiPort);
  if (!m_wsapi->CheckService())
  {
    SAFE_DELETE(m_wsapi);
    SAFE_DELETE(m_control);
    XBMC->Log(LOG_ERROR,"Failed to connect to MythTV backend on %s:%d", g_szMythHostname.c_str(), g_iWSApiPort);
    return false;
  }

  // Create event handler
  m_eventHandler = new Myth::EventHandler(g_szMythHostname, g_iProtoPort);
  m_eventSubscriberId = m_eventHandler->CreateSubscription(this);
  m_eventHandler->SubscribeForEvent(m_eventSubscriberId, Myth::EVENT_HANDLER_STATUS);
  m_eventHandler->SubscribeForEvent(m_eventSubscriberId, Myth::EVENT_HANDLER_TIMER);
  m_eventHandler->SubscribeForEvent(m_eventSubscriberId, Myth::EVENT_SCHEDULE_CHANGE);
  m_eventHandler->SubscribeForEvent(m_eventSubscriberId, Myth::EVENT_ASK_RECORDING);
  m_eventHandler->SubscribeForEvent(m_eventSubscriberId, Myth::EVENT_RECORDING_LIST_CHANGE);
  m_eventHandler->Start();

  // Create schedule manager
  m_scheduleManager = new MythScheduleManager(g_szMythHostname, g_iWSApiPort, g_iProtoPort);

  // Create file operation helper (image caching)
  m_fileOps = new FileOps(g_szMythHostname, g_iWSApiPort);

  return true;
}

const char *PVRClientMythTV::GetBackendName()
{
  std::string label;
  label.append("MythTV (").append(m_wsapi->GetServerHostName()).append(")");
  XBMC->Log(LOG_DEBUG, "%s: %s", __FUNCTION__, label.c_str());
  return label.c_str();
}

const char *PVRClientMythTV::GetBackendVersion()
{
  Myth::VersionPtr version = m_wsapi->GetVersion();
  XBMC->Log(LOG_DEBUG, "%s: %s", __FUNCTION__, version->version.c_str());
  return version->version.c_str();
}

const char *PVRClientMythTV::GetConnectionString()
{
  std::string cs;
  cs.append("http://").append(g_szMythHostname).append(":").append(Myth::IntToString(g_iWSApiPort));
  XBMC->Log(LOG_DEBUG, "%s: %s", __FUNCTION__, cs.c_str());
  return cs.c_str();
}

PVR_ERROR PVRClientMythTV::GetDriveSpace(long long *iTotal, long long *iUsed)
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  int64_t total = 0, used = 0;
  if (m_control->QueryFreeSpaceSummary(&total, &used))
  {
    *iTotal = (long long)total;
    *iUsed = (long long)used;
    return PVR_ERROR_NO_ERROR;
  }
  return PVR_ERROR_UNKNOWN;
}

void PVRClientMythTV::OnSleep()
{
  if (m_eventHandler)
    m_eventHandler->Stop();
  if (m_fileOps)
    m_fileOps->Suspend();
}

void PVRClientMythTV::OnWake()
{
  if (m_eventHandler)
    m_eventHandler->Start(); // Resume(true)
  if (m_fileOps)
    m_fileOps->Resume();
}

void PVRClientMythTV::HandleBackendMessage(const Myth::EventMessage& msg)
{
  switch (msg.event)
  {
    case Myth::EVENT_SCHEDULE_CHANGE:
      HandleScheduleChange();
      break;
    case Myth::EVENT_ASK_RECORDING:
      HandleAskRecording(msg);
      break;
    case Myth::EVENT_RECORDING_LIST_CHANGE:
      HandleRecordingListChange(msg);
      break;
    case Myth::EVENT_HANDLER_TIMER:
      RunHouseKeeping();
      break;
    case Myth::EVENT_HANDLER_STATUS:
      if (msg.subject[0] == EVENTHANDLER_DISCONNECTED)
      {
      }
      break;
    default:
      break;
  }
}

void PVRClientMythTV::HandleScheduleChange()
{
  if (!m_scheduleManager)
    return;
  m_scheduleManager->Update();
  PVR->TriggerTimerUpdate();
}

void PVRClientMythTV::HandleAskRecording(const Myth::EventMessage& msg)
{
  // ASK_RECORDING <card id> <time until> <has rec> <has later>[]:[]<program info>
  // Example: ASK_RECORDING 9 29 0 1[]:[]<program>
  if (msg.subject.size() < 5)
  {
    for (unsigned i = 0; i < msg.subject.size(); ++i)
      XBMC->Log(LOG_ERROR, "%s: Incorrect message: %d : %s", __FUNCTION__, i, msg.subject[i].c_str());
    return;
  }
  // The scheduled recording will hang in MythTV if ASK_RECORDING is just ignored.
  // - Stop recorder (and blocked for time until seconds)
  // - Skip the recording by sending CANCEL_NEXT_RECORDING(true)
  uint32_t cardid = Myth::StringToId(msg.subject[1]);
  int timeuntil = Myth::StringToInt(msg.subject[2]);
  int hasrec = Myth::StringToInt(msg.subject[3]);
  int haslater = Myth::StringToInt(msg.subject[4]);
  XBMC->Log(LOG_NOTICE, "%s: Event ASK_RECORDING: rec=%d timeuntil=%d hasrec=%d haslater=%d", __FUNCTION__,
          cardid, timeuntil, hasrec, haslater);

  std::string title;
  if (msg.program)
    title = msg.program->title;
  XBMC->Log(LOG_NOTICE, "%s: Event ASK_RECORDING: title=%s", __FUNCTION__, title.c_str());

  if (timeuntil >= 0 && cardid > 0 && m_liveStream && m_liveStream->GetCardId() == cardid)
  {
    if (g_iLiveTVConflictStrategy == LIVETV_CONFLICT_STRATEGY_CANCELREC ||
      (g_iLiveTVConflictStrategy == LIVETV_CONFLICT_STRATEGY_HASLATER && haslater))
    {
      XBMC->QueueNotification(QUEUE_WARNING, XBMC->GetLocalizedString(30307), title.c_str()); // Canceling conflicting recording: %s
      m_control->CancelNextRecording((int)cardid, true);
    }
    else // LIVETV_CONFLICT_STRATEGY_STOPTV
    {
      XBMC->QueueNotification(QUEUE_WARNING, XBMC->GetLocalizedString(30308), title.c_str()); // Stopping Live TV due to conflicting recording: %s
      CloseLiveStream();
    }
  }
}

void PVRClientMythTV::HandleRecordingListChange(const Myth::EventMessage& msg)
{
  unsigned cs = (unsigned)msg.subject.size();
  if (cs == 1)
  {
    if (g_bExtraDebug)
      XBMC->Log(LOG_DEBUG, "%s: Reload all recordings", __FUNCTION__);
    CLockObject lock(m_recordingsLock);
    FillRecordings();
    ++m_recordingChangePinCount;
  }
  else if (cs == 4 && msg.subject[1] == "ADD")
  {
    uint32_t chanid = Myth::StringToId(msg.subject[2]);
    time_t startts = Myth::StringToTime(msg.subject[3]);
    MythProgramInfo prog(m_wsapi->GetRecorded(chanid, startts));
    if (!prog.IsNull())
    {
      CLockObject lock(m_recordingsLock);
      ProgramInfoMap::iterator it = m_recordings.find(prog.UID());
      if (it == m_recordings.end())
      {
        if (g_bExtraDebug)
          XBMC->Log(LOG_DEBUG, "%s: Add recording: %s", __FUNCTION__, prog.UID().c_str());
        // Add recording
        m_recordings.insert(std::pair<std::string, MythProgramInfo>(prog.UID().c_str(), prog));
        ++m_recordingChangePinCount;
      }
    }
    else
      XBMC->Log(LOG_ERROR, "%s: Add recording failed for %u %ld", __FUNCTION__, (unsigned)chanid, (long)startts);
  }
  else if (cs == 2 && msg.subject[1] == "UPDATE" && msg.program)
  {
    CLockObject lock(m_recordingsLock);
    MythProgramInfo prog(msg.program);
    ProgramInfoMap::iterator it = m_recordings.find(prog.UID());
    if (it != m_recordings.end())
    {
      if (g_bExtraDebug)
        XBMC->Log(LOG_DEBUG, "%s: Update recording: %s", __FUNCTION__, prog.UID().c_str());
      // Copy cached framerate
      //prog.SetFrameRate(it->second.FrameRate());
      // Update recording
      it->second = prog;
      ++m_recordingChangePinCount;
    }
  }
  else if (cs == 4 && msg.subject[1] == "DELETE")
  {
    // MythTV send two DELETE events. First requests deletion, second confirms deletion.
    // On first we delete recording. On second program will not be found.
    uint32_t chanid = Myth::StringToId(msg.subject[2]);
    time_t startts = Myth::StringToTime(msg.subject[3]);
    MythProgramInfo prog(m_wsapi->GetRecorded(chanid, startts));
    if (!prog.IsNull())
    {
      CLockObject lock(m_recordingsLock);
      ProgramInfoMap::iterator it = m_recordings.find(prog.UID());
      if (it != m_recordings.end())
      {
        if (g_bExtraDebug)
          XBMC->Log(LOG_DEBUG, "%s: Delete recording: %s", __FUNCTION__, prog.UID().c_str());
        // Remove recording
        m_recordings.erase(it);
        ++m_recordingChangePinCount;
      }
    }
  }
}

void PVRClientMythTV::RunHouseKeeping()
{
  // It is time to work
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  if (m_recordingChangePinCount)
  {
    PVR->TriggerRecordingUpdate();
    m_recordingChangePinCount = 0;
  }
}

PVR_ERROR PVRClientMythTV::GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd)
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG,"%s: start: %ld, end: %ld, chanid: %u", __FUNCTION__, (long)iStart, (long)iEnd, channel.iUniqueId);

  if (!channel.bIsHidden)
  {
    Myth::ProgramMapPtr EPG = m_wsapi->GetProgramGuide(channel.iUniqueId, iStart, iEnd);
    // Transfer EPG for the given channel
    for (Myth::ProgramMap::reverse_iterator it = EPG->rbegin(); it != EPG->rend(); ++it)
    {
      EPG_TAG tag;
      memset(&tag, 0, sizeof(EPG_TAG));
      tag.startTime = it->first;
      tag.endTime = it->second->endTime;
      // Reject bad entry
      if (tag.endTime <= tag.startTime)
        continue;

      // EPG_TAG expects strings as char* and not as copies (like the other PVR types).
      // Therefore we have to make sure that we don't pass invalid (freed) memory to TransferEpgEntry.
      // In particular we have to use local variables and must not pass returned string values directly.
      tag.strTitle = it->second->title.c_str();
      tag.strPlot = it->second->description.c_str();
      tag.strGenreDescription = it->second->category.c_str();
      tag.iUniqueBroadcastId = MakeBroadcastID(it->second->channel.chanId, it->first);
      tag.iChannelNumber = atoi(it->second->channel.chanNum.c_str());
      int genre = m_categories.Category(it->second->category);
      tag.iGenreSubType = genre & 0x0F;
      tag.iGenreType = genre & 0xF0;
      tag.strEpisodeName = "";
      tag.strIconPath = "";
      tag.strPlotOutline = "";
      tag.bNotify = false;
      tag.firstAired = it->second->airdate;
      tag.iEpisodeNumber = (int)it->second->episode;
      tag.iEpisodePartNumber = 0;
      tag.iParentalRating = 0;
      tag.iSeriesNumber = (int)it->second->season;
      tag.iStarRating = atoi(it->second->stars.c_str());

      PVR->TransferEpgEntry(handle, &tag);
    }
  }

  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s: Done", __FUNCTION__);

  return PVR_ERROR_NO_ERROR;
}

int PVRClientMythTV::GetNumChannels()
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  LoadChannelsAndChannelGroups();

  return m_channelsById.size();
}

PVR_ERROR PVRClientMythTV::GetChannels(ADDON_HANDLE handle, bool bRadio)
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s: radio: %s", __FUNCTION__, (bRadio ? "true" : "false"));

  LoadChannelsAndChannelGroups();
  m_PVRChannelUidById.clear();

  // Create a map<(channum, callsign), chanid> to merge channels with same channum and callsign
  std::map<std::pair<std::string, std::string>, unsigned int> channelIdentifiers;

  // Transfer channels of the requested type (radio / tv)
  for (ChannelIdMap::iterator it = m_channelsById.begin(); it != m_channelsById.end(); ++it)
  {
    if (it->second.IsRadio() == bRadio && !it->second.IsNull())
    {
      // Skip channels with same channum and callsign
      std::pair<std::string, std::string> channelIdentifier = std::make_pair(it->second.Number(), it->second.Callsign());
      std::map<std::pair<std::string, std::string>, unsigned int>::iterator itm = channelIdentifiers.find(channelIdentifier);
      if (itm != channelIdentifiers.end())
      {
        XBMC->Log(LOG_DEBUG, "%s: skipping channel: %d", __FUNCTION__, it->second.ID());
        // Map channel with merged channel
        m_PVRChannelUidById.insert(std::make_pair(it->first, itm->second));
        continue;
      }
      channelIdentifiers.insert(std::make_pair(channelIdentifier, it->first));
      // Map channel to itself
      m_PVRChannelUidById.insert(std::make_pair(it->first, it->first));

      PVR_CHANNEL tag;
      memset(&tag, 0, sizeof(PVR_CHANNEL));

      tag.iUniqueId = it->first;
      tag.iChannelNumber = (unsigned)atoi(it->second.Number().c_str());
      PVR_STRCPY(tag.strChannelName, it->second.Name().c_str());
      tag.bIsHidden = !it->second.Visible();
      tag.bIsRadio = it->second.IsRadio();

      std::string icon = m_fileOps->GetChannelIconPath(it->second);
      PVR_STRCPY(tag.strIconPath, icon.c_str());

      // Unimplemented
      PVR_STRCPY(tag.strStreamURL, "");
      PVR_STRCPY(tag.strInputFormat, "");
      tag.iEncryptionSystem = 0;

      PVR->TransferChannelEntry(handle, &tag);
    }
  }

  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s: Done", __FUNCTION__);

  return PVR_ERROR_NO_ERROR;
}

int PVRClientMythTV::GetChannelGroupsAmount()
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  LoadChannelsAndChannelGroups();

  return m_channelGroups.size();
}

PVR_ERROR PVRClientMythTV::GetChannelGroups(ADDON_HANDLE handle, bool bRadio)
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s: radio: %s", __FUNCTION__, (bRadio ? "true" : "false"));

  LoadChannelsAndChannelGroups();

  // Transfer channel groups of the given type (radio / tv)
  for (ChannelGroupMap::iterator channelGroupsIt = m_channelGroups.begin(); channelGroupsIt != m_channelGroups.end(); ++channelGroupsIt)
  {
    PVR_CHANNEL_GROUP tag;
    memset(&tag, 0, sizeof(PVR_CHANNEL_GROUP));

    PVR_STRCPY(tag.strGroupName, channelGroupsIt->first.c_str());
    tag.bIsRadio = bRadio;

    // Only add the group if we have at least one channel of the correct type
    for (std::vector<uint32_t>::iterator channelGroupIt = channelGroupsIt->second.begin(); channelGroupIt != channelGroupsIt->second.end(); ++channelGroupIt)
    {
      ChannelIdMap::iterator channelIt = m_channelsById.find(*channelGroupIt);
      if (channelIt != m_channelsById.end() && channelIt->second.IsRadio() == bRadio)
      {
        PVR->TransferChannelGroup(handle, &tag);
        break;
      }
    }
  }

  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s: Done", __FUNCTION__);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR PVRClientMythTV::GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group)
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s: group: %s", __FUNCTION__, group.strGroupName);

  LoadChannelsAndChannelGroups();

  ChannelGroupMap::iterator channelGroupsIt = m_channelGroups.find(group.strGroupName);
  if (channelGroupsIt == m_channelGroups.end())
  {
    XBMC->Log(LOG_ERROR,"%s: Channel group not found", __FUNCTION__);
    return PVR_ERROR_INVALID_PARAMETERS;
  }

  // Transfer the channel group members for the requested group
  unsigned channelNumber = 0;
  for (std::vector<uint32_t>::iterator channelGroupIt = channelGroupsIt->second.begin(); channelGroupIt != channelGroupsIt->second.end(); ++channelGroupIt)
  {
    ChannelIdMap::iterator channelIt = m_channelsById.find(*channelGroupIt);
    if (channelIt != m_channelsById.end() && channelIt->second.IsRadio() == group.bIsRadio)
    {
      PVR_CHANNEL_GROUP_MEMBER tag;
      memset(&tag, 0, sizeof(PVR_CHANNEL_GROUP_MEMBER));

      tag.iChannelNumber = channelNumber++;
      tag.iChannelUniqueId = (unsigned)FindPVRChannelUid(channelIt->second.ID());
      PVR_STRCPY(tag.strGroupName, group.strGroupName);
      PVR->TransferChannelGroupMember(handle, &tag);
    }
  }

  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s: Done", __FUNCTION__);

  return PVR_ERROR_NO_ERROR;
}

void PVRClientMythTV::LoadChannelsAndChannelGroups()
{
  if (!m_channelsById.empty())
    return;

  // For each source create a channels group
  Myth::VideoSourceListPtr sources = m_wsapi->GetVideoSourceList();
  for (Myth::VideoSourceList::iterator its = sources->begin(); its != sources->end(); ++its)
  {
    Myth::ChannelListPtr channels = m_wsapi->GetChannelList((*its)->sourceId);
    std::vector<uint32_t> channelIDs;
    channelIDs.reserve(channels->size());
    for (Myth::ChannelList::iterator itc = channels->begin(); itc != channels->end(); ++itc)
    {
      MythChannel channel((*itc));
      m_channelsById.insert(std::make_pair(channel.ID(), channel));
      m_channelsByNumber.insert(std::make_pair(channel.Number(), channel));
      channelIDs.push_back(channel.ID());
    }
    m_channelGroups.insert(std::make_pair((*its)->sourceName, channelIDs));
  }
}

int PVRClientMythTV::FindPVRChannelUid(uint32_t channelId) const
{
  PVRChannelMap::const_iterator it = m_PVRChannelUidById.find(channelId);
  if (it != m_PVRChannelUidById.end())
    return it->second;
  return -1; // PVR dummy channel UID
}

void PVRClientMythTV::UpdateRecordings()
{
  PVR->TriggerRecordingUpdate();
}

int PVRClientMythTV::GetRecordingsAmount(void)
{
  int res = 0;
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  CLockObject lock(m_recordingsLock);

  if (m_recordings.empty())
    // Load recorings list
    res = FillRecordings();
  else
  {
    for (ProgramInfoMap::iterator it = m_recordings.begin(); it != m_recordings.end(); ++it)
    {
      if (!it->second.IsNull() && it->second.IsVisible())
        res++;
    }
  }
  if (res == 0)
    XBMC->Log(LOG_INFO, "%s: No recording", __FUNCTION__);
  return res;
}

PVR_ERROR PVRClientMythTV::GetRecordings(ADDON_HANDLE handle)
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  CLockObject lock(m_recordingsLock);

  // Load recordings list
  if (m_recordings.empty())
    FillRecordings();

  for (ProgramInfoMap::iterator it = m_recordings.begin(); it != m_recordings.end(); ++it)
  {
    if (!it->second.IsNull() && it->second.IsVisible())
    {
      PVR_RECORDING tag;
      memset(&tag, 0, sizeof(PVR_RECORDING));

      tag.recordingTime = it->second.RecordingStartTime();
      tag.iDuration = it->second.Duration();
      tag.iPlayCount = it->second.IsWatched() ? 1 : 0;
      //@TODO: tag.iLastPlayedPosition

      std::string id = it->second.UID();
      std::string title = this->MakeProgramTitle(it->second.Title(), it->second.Subtitle());

      PVR_STRCPY(tag.strRecordingId, id.c_str());
      PVR_STRCPY(tag.strTitle, title.c_str());
      PVR_STRCPY(tag.strPlot, it->second.Description().c_str());
      PVR_STRCPY(tag.strChannelName, it->second.ChannelName().c_str());

      int genre = m_categories.Category(it->second.Category());
      tag.iGenreSubType = genre&0x0F;
      tag.iGenreType = genre&0xF0;

      // Add recording title to directory to group everything according to its name just like MythTV does
      std::string strDirectory;
      strDirectory.append(it->second.RecordingGroup()).append("/").append(it->second.Title());
      PVR_STRCPY(tag.strDirectory, strDirectory.c_str());

      // Images
      std::string strIconPath;
      if (it->second.HasCoverart())
        strIconPath = m_fileOps->GetArtworkPath(it->second, FileOps::FileTypeCoverart);
      else if (it->second.IsLiveTV())
      {
        MythChannel channel = FindRecordingChannel(it->second);
        if (!channel.IsNull())
          strIconPath = m_fileOps->GetChannelIconPath(channel);
      }
      else
        strIconPath = m_fileOps->GetPreviewIconPath(it->second);

      std::string strFanartPath;
      if (it->second.HasFanart())
        strFanartPath = m_fileOps->GetArtworkPath(it->second, FileOps::FileTypeFanart);

      PVR_STRCPY(tag.strIconPath, strIconPath.c_str());
      PVR_STRCPY(tag.strThumbnailPath, strIconPath.c_str());
      PVR_STRCPY(tag.strFanartPath, strFanartPath.c_str());

      // Unimplemented
      tag.iLifetime = 0;
      tag.iPriority = 0;
      PVR_STRCPY(tag.strPlotOutline, "");
      PVR_STRCPY(tag.strStreamURL, "");

      PVR->TransferRecordingEntry(handle, &tag);
    }
  }

  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s: Done", __FUNCTION__);

  return PVR_ERROR_NO_ERROR;
}

void PVRClientMythTV::ForceUpdateRecording(ProgramInfoMap::iterator it)
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  if (!it->second.IsNull())
  {
    MythProgramInfo prog(m_wsapi->GetRecorded(it->second.ChannelID(), it->second.RecordingStartTime()));
    if (!prog.IsNull())
    {
      CLockObject lock(m_recordingsLock);
      // Copy cached framerate
      //prog.SetFrameRate(it->second.FrameRate());
      // Update recording
      it->second = prog;
      ++m_recordingChangePinCount;

      if (g_bExtraDebug)
        XBMC->Log(LOG_DEBUG, "%s: Done", __FUNCTION__);
    }
  }
}

int PVRClientMythTV::FillRecordings()
{
  int count = 0;
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  // Check event connection
  if (!m_eventHandler->IsConnected())
    return count;

  // Load recordings map
  m_recordings.clear();
  Myth::ProgramListPtr programs = m_wsapi->GetRecordedList();
  for (Myth::ProgramList::iterator it = programs->begin(); it != programs->end(); ++it)
  {
    MythProgramInfo prog = MythProgramInfo(*it);
    m_recordings.insert(std::make_pair(prog.UID(), prog));
    // Count visible recordings
    if (prog.IsVisible() && !prog.IsLiveTV())
      ++count;
  }
  return count;
}

PVR_ERROR PVRClientMythTV::DeleteRecording(const PVR_RECORDING &recording)
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  CLockObject lock(m_recordingsLock);

  ProgramInfoMap::iterator it = m_recordings.find(recording.strRecordingId);
  if (it != m_recordings.end())
  {
    // Deleting Live recording is prohibited. Otherwise continue
    if (this->IsMyLiveRecording(it->second))
    {
      if (it->second.IsLiveTV())
        return PVR_ERROR_RECORDING_RUNNING;
      // it is kept then ignore it now.
      if (m_liveStream && m_liveStream->KeepLiveRecording(false))
        return PVR_ERROR_NO_ERROR;
      else
        return PVR_ERROR_FAILED;
    }
    bool ret = m_control->DeleteRecording(*(it->second.GetPtr()));
    if (ret)
    {
      XBMC->Log(LOG_DEBUG, "%s: Deleted recording %s", __FUNCTION__, recording.strRecordingId);
      return PVR_ERROR_NO_ERROR;
    }
    else
    {
      XBMC->Log(LOG_ERROR, "%s: Failed to delete recording %s", __FUNCTION__, recording.strRecordingId);
    }
  }
  else
  {
    XBMC->Log(LOG_ERROR, "%s: Recording %s does not exist", __FUNCTION__, recording.strRecordingId);
  }
  return PVR_ERROR_FAILED;
}

PVR_ERROR PVRClientMythTV::DeleteAndForgetRecording(const PVR_RECORDING &recording)
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  CLockObject lock(m_recordingsLock);

  ProgramInfoMap::iterator it = m_recordings.find(recording.strRecordingId);
  if (it != m_recordings.end())
  {
    // Deleting Live recording is prohibited. Otherwise continue
    if (this->IsMyLiveRecording(it->second))
    {
      if (it->second.IsLiveTV())
        return PVR_ERROR_RECORDING_RUNNING;
      // it is kept then ignore it now.
      if (m_liveStream && m_liveStream->KeepLiveRecording(false))
        return PVR_ERROR_NO_ERROR;
      else
        return PVR_ERROR_FAILED;
    }
    bool ret = m_control->DeleteRecording(*(it->second.GetPtr()));
    if (ret)
    {
      XBMC->Log(LOG_DEBUG, "%s: Deleted and forget recording %s", __FUNCTION__, recording.strRecordingId);
      return PVR_ERROR_NO_ERROR;
    }
    else
    {
      XBMC->Log(LOG_ERROR, "%s: Failed to delete recording %s", __FUNCTION__, recording.strRecordingId);
    }
  }
  else
  {
    XBMC->Log(LOG_ERROR, "%s: Recording %s does not exist", __FUNCTION__, recording.strRecordingId);
  }
  return PVR_ERROR_FAILED;
}

PVR_ERROR PVRClientMythTV::SetRecordingPlayCount(const PVR_RECORDING &recording, int count)
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  CLockObject lock(m_recordingsLock);
  ProgramInfoMap::iterator it = m_recordings.find(recording.strRecordingId);
  if (it != m_recordings.end())
  {
    if (m_wsapi->UpdateRecordedWatchedStatus(it->second.ChannelID(), it->second.RecordingStartTime(), (count > 0 ? true : false)))
    {
      if (g_bExtraDebug)
        XBMC->Log(LOG_DEBUG, "%s: Set watched state for %s", __FUNCTION__, recording.strRecordingId);
      ForceUpdateRecording(it);
      return PVR_ERROR_NO_ERROR;
    }
    else
    {
      XBMC->Log(LOG_DEBUG, "%s: Failed setting watched state for: %s", __FUNCTION__, recording.strRecordingId);
      return PVR_ERROR_NO_ERROR;
    }
  }
  else
  {
    XBMC->Log(LOG_DEBUG, "%s: Recording %s does not exist", __FUNCTION__, recording.strRecordingId);
  }
  return PVR_ERROR_FAILED;
}

/*
 *  @TODO: Implement using proto or wsapi
 *
PVR_ERROR PVRClientMythTV::SetRecordingLastPlayedPosition(const PVR_RECORDING &recording, int lastplayedposition)
{
  // MythTV provides it's bookmarks as frame offsets whereas XBMC expects a time offset.
  // The frame offset is calculated by: frameOffset = bookmark * frameRate.

  if (g_bExtraDebug)
  {
    XBMC->Log(LOG_DEBUG, "%s: Setting Bookmark for: %s to %d", __FUNCTION__, recording.strTitle, lastplayedposition);
  }

  CLockObject lock(m_recordingsLock);
  ProgramInfoMap::iterator it = m_recordings.find(recording.strRecordingId);
  if (it != m_recordings.end())
  {
    // pin framerate value
    //if (it->second.FrameRate() < 0)
    //  it->second.SetFrameRate(m_db.GetRecordingFrameRate(it->second));
    // Calculate the frame offset
    long long frameOffset = (long long)(lastplayedposition * it->second.FrameRate() / 1000.0f);
    if (frameOffset < 0) frameOffset = 0;
    if (g_bExtraDebug)
    {
      XBMC->Log(LOG_DEBUG, "%s: FrameOffset: %lld)", __FUNCTION__, frameOffset);
    }

    // Write the bookmark
    if (m_con.SetBookmark(it->second, frameOffset))
    {
      if (g_bExtraDebug)
        XBMC->Log(LOG_DEBUG, "%s: Setting Bookmark successful", __FUNCTION__);
      return PVR_ERROR_NO_ERROR;
    }
    else
    {
      if (g_bExtraDebug)
      {
        XBMC->Log(LOG_ERROR, "%s: Setting Bookmark failed", __FUNCTION__);
      }
    }
  }
  else
  {
    XBMC->Log(LOG_DEBUG, "%s: Recording %s does not exist", __FUNCTION__, recording.strRecordingId);
  }
  return PVR_ERROR_FAILED;
}

int PVRClientMythTV::GetRecordingLastPlayedPosition(MythProgramInfo &programInfo)
{
  // MythTV provides it's bookmarks as frame offsets whereas XBMC expects a time offset.
  // The bookmark in seconds is calculated by: bookmark = frameOffset / frameRate.
  int bookmark = 0;

  if (programInfo.HasBookmark())
  {
    long long frameOffset = m_con.GetBookmark(programInfo); // returns 0 if no bookmark was found
    if (frameOffset > 0)
    {
      if (g_bExtraDebug)
      {
        XBMC->Log(LOG_DEBUG, "%s: FrameOffset: %lld)", __FUNCTION__, frameOffset);
      }
      // Pin framerate value
      //if (programInfo.FrameRate() < 0)
      //  programInfo.SetFrameRate(m_db.GetRecordingFrameRate(programInfo));
      float frameRate = (float)programInfo.FrameRate() / 1000.0f;
      if (frameRate > 0)
      {
        bookmark = (int)((float)frameOffset / frameRate);
        if (g_bExtraDebug)
        {
          XBMC->Log(LOG_DEBUG, "%s: Bookmark: %d)", __FUNCTION__, bookmark);
        }
      }
    }
  }
  else
  {
    if (g_bExtraDebug)
    {
      XBMC->Log(LOG_DEBUG, "%s: Recording %s has no bookmark", __FUNCTION__, programInfo.Title().c_str());
    }
  }

  if (bookmark < 0) bookmark = 0;
  return bookmark;
}

PVR_ERROR PVRClientMythTV::GetRecordingEdl(const PVR_RECORDING &recording, PVR_EDL_ENTRY entries[], int *size)
{
  if (g_bExtraDebug)
  {
    XBMC->Log(LOG_DEBUG, "%s: Reading edl for: %s", __FUNCTION__, recording.strTitle);
  }

  CLockObject lock(m_recordingsLock);
  ProgramInfoMap::iterator it = m_recordings.find(recording.strRecordingId);
  if (it == m_recordings.end())
  {
    XBMC->Log(LOG_DEBUG, "%s: Recording %s does not exist", __FUNCTION__, recording.strRecordingId);
    *size = 0;
    return PVR_ERROR_FAILED;
  }

  bool useFrameRate = true;
  float frameRate = 0.0f;
  int64_t psoffset, nsoffset;

  if (m_db.GetSchemaVersion() >= 1309)
  {
    if (m_db.GetRecordingSeekOffset(it->second, MARK_DURATION_MS, 0, &psoffset, &nsoffset) > 0)
    {
      // mark 33 found for recording. no need convertion using framerate.
      useFrameRate = false;
    }
    else
    {
      XBMC->Log(LOG_DEBUG, "%s: Missing recordedseek map, try running 'mythcommflag --rebuild' for recording %s", __FUNCTION__, recording.strRecordingId);
      XBMC->Log(LOG_DEBUG, "%s: Fallback using framerate ...", __FUNCTION__);
    }
  }
  if (useFrameRate)
  {
    frameRate = m_db.GetRecordingFrameRate(it->second) / 1000.0f;
    if (frameRate <= 0)
    {
      XBMC->Log(LOG_DEBUG, "%s: Failed to read framerate for %s", __FUNCTION__, recording.strRecordingId);
      *size = 0;
      return PVR_ERROR_FAILED;
    }
  }

  Edl commbreakList = m_con.GetCommbreakList(it->second);
  int commbreakCount = commbreakList.size();
  XBMC->Log(LOG_DEBUG, "%s: Found %d commercial breaks for: %s", __FUNCTION__, commbreakCount, recording.strTitle);

  Edl cutList = m_con.GetCutList(it->second);
  XBMC->Log(LOG_DEBUG, "%s: Found %d cut list entries for: %s", __FUNCTION__, cutList.size(), recording.strTitle);

  commbreakList.insert(commbreakList.end(), cutList.begin(), cutList.end());
  int index = 0;
  Edl::const_iterator edlIt;
  for (edlIt = commbreakList.begin(); edlIt != commbreakList.end(); ++edlIt)
  {
    if (index < *size)
    {
      int64_t start, end;
      start = end = 0;

      // Pull the closest match in the DB if it exists
      if (useFrameRate)
      {
        start = (int64_t)(edlIt->start_mark / frameRate * 1000);
        end = (int64_t)(edlIt->end_mark / frameRate * 1000);
        XBMC->Log(LOG_DEBUG, "%s: start_mark: %lld, end_mark: %lld, start: %lld, end: %lld", __FUNCTION__, edlIt->start_mark, edlIt->end_mark, start, end);
      }
      else
      {
        // mask == 1 ==> Found before the mark (use psoffset, nsoffset is zero)
        // mask == 2 ==> Found after the mark (use nsoffset, psoffset is zero)
        // mask == 3 ==> Found around the mark (use nsoffset [next offset / later] for start, psoffset [prev. offset / before] for end)
        int mask = m_db.GetRecordingSeekOffset(it->second, MARK_DURATION_MS, edlIt->start_mark, &psoffset, &nsoffset);
        XBMC->Log(LOG_DEBUG, "%s: start_mark offset mask: %d, psoffset: %"PRId64", nsoffset: %"PRId64, __FUNCTION__, mask, psoffset, nsoffset);
        if (mask > 0)
        {
          if (mask == 1)
            start = psoffset;
          else if (edlIt->start_mark == 0)
            start = 0;
          else
            start = nsoffset;

          mask = m_db.GetRecordingSeekOffset(it->second, MARK_DURATION_MS, edlIt->end_mark, &psoffset, &nsoffset);
          XBMC->Log(LOG_DEBUG, "%s: end_mark offset mask: %d, psoffset: %"PRId64", nsoffset: %"PRId64, __FUNCTION__, mask, psoffset, nsoffset);
          if (mask == 2)
            end = nsoffset;
          else if (mask == 1 || mask == 3)
            end = psoffset;
          else
            // By forcing the end to be zero, it will never be > start, which makes the values invalid
            // This is only for failed lookups for the end position
            end = 0;
        }
        if (mask <= 0)
          XBMC->Log(LOG_DEBUG, "%s: Failed to retrieve recordedseek offset values", __FUNCTION__);
      }

      if (start < end)
      {
        // We have both a valid start and end value now
        XBMC->Log(LOG_DEBUG, "%s: start_mark: %"PRId64", end_mark: %"PRId64", start: %"PRId64", end: %"PRId64, __FUNCTION__, edlIt->start_mark, edlIt->end_mark, start, end);
        PVR_EDL_ENTRY entry;
        entry.start = start;
        entry.end = end;
        entry.type = index < commbreakCount ? PVR_EDL_TYPE_COMBREAK : PVR_EDL_TYPE_CUT;
        entries[index] = entry;
        index++;
      }
      else
        XBMC->Log(LOG_DEBUG, "%s: invalid offset: start_mark: %"PRId64", end_mark: %"PRId64", start: %"PRId64", end: %"PRId64, __FUNCTION__, edlIt->start_mark, edlIt->end_mark, start, end);
    }
    else
    {
      XBMC->Log(LOG_ERROR, "%s: Maximum number of edl entries reached for %s", __FUNCTION__, recording.strTitle);
      break;
    }
  }
  *size = index;
  return PVR_ERROR_NO_ERROR;
}

int PVRClientMythTV::GetRecordingLastPlayedPosition(const PVR_RECORDING &recording)
{
  // MythTV provides it's bookmarks as frame offsets whereas XBMC expects a time offset.
  // The bookmark in seconds is calculated by: bookmark = frameOffset / frameRate.
  int bookmark = 0;

  if (g_bExtraDebug)
  {
    XBMC->Log(LOG_DEBUG, "%s: Reading Bookmark for: %s", __FUNCTION__, recording.strTitle);
  }

  CLockObject lock(m_recordingsLock);
  ProgramInfoMap::iterator it = m_recordings.find(recording.strRecordingId);
  if (it != m_recordings.end())
    bookmark = GetRecordingLastPlayedPosition(it->second);
  else
    XBMC->Log(LOG_ERROR, "%s: Recording %s does not exist", __FUNCTION__, recording.strRecordingId);

  return bookmark;
}
*/
MythChannel PVRClientMythTV::FindRecordingChannel(const MythProgramInfo& programInfo)
{
  ChannelIdMap::iterator channelByIdIt = m_channelsById.find(programInfo.ChannelID());
  if (channelByIdIt != m_channelsById.end())
  {
    return channelByIdIt->second;
  }
  return MythChannel();
}

bool PVRClientMythTV::IsMyLiveRecording(const MythProgramInfo& programInfo)
{
  if (!programInfo.IsNull())
  {
    // Begin critical section
    CLockObject lock(m_lock);
    if (m_liveStream && m_liveStream->IsPlaying())
    {
      MythProgramInfo live(m_liveStream->GetPlayedProgram());
      if (live == programInfo)
        return true;
    }
  }
  return false;
}

int PVRClientMythTV::GetTimersAmount(void)
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  ScheduleList upcomingRecordings = m_scheduleManager->GetUpcomingRecordings();
  return upcomingRecordings.size();
}

PVR_ERROR PVRClientMythTV::GetTimers(ADDON_HANDLE handle)
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  m_PVRtimerMemorandum.clear();

  ScheduleList upcomingRecordings = m_scheduleManager->GetUpcomingRecordings();
  for (ScheduleList::iterator it = upcomingRecordings.begin(); it != upcomingRecordings.end(); ++it)
  {
    PVR_TIMER tag;
    memset(&tag, 0, sizeof(PVR_TIMER));

    std::string rulemarker = "";
    tag.startTime = it->second->StartTime();
    tag.endTime = it->second->EndTime();
    tag.iClientChannelUid = (int)FindPVRChannelUid(it->second->ChannelID());
    tag.iPriority = it->second->Priority();
    int genre = m_categories.Category(it->second->Category());
    tag.iGenreSubType = genre & 0x0F;
    tag.iGenreType = genre & 0xF0;

    // Fill info from recording rule if possible
    RecordingRuleNodePtr node = m_scheduleManager->FindRuleById(it->second->RecordID());
    if (node)
    {
      MythRecordingRule rule = node->GetRule();
      RuleMetadata meta = m_scheduleManager->GetMetadata(rule);
      tag.iMarginEnd = rule.EndOffset();
      tag.iMarginStart = rule.StartOffset();
      tag.firstDay = it->second->RecordingStartTime();
      tag.bIsRepeating = meta.isRepeating;
      tag.iWeekdays = meta.weekDays;
      if (*(meta.marker))
        rulemarker.append("(").append(meta.marker).append(")");
    }
    else
    {
      // Default rule info
      tag.iMarginEnd = 0;
      tag.iMarginStart = 0;
      tag.firstDay = 0;
      tag.bIsRepeating = false;
      tag.iWeekdays = 0;
    }

    // Status: Match recording status with PVR_TIMER status
    if (g_bExtraDebug)
      XBMC->Log(LOG_DEBUG,"%s ## - State: %d - ##", __FUNCTION__, it->second->Status());
    switch (it->second->Status())
    {
    case Myth::RS_RECORDING:
      tag.state = PVR_TIMER_STATE_RECORDING;
      break;
    case Myth::RS_TUNING:
      tag.state = PVR_TIMER_STATE_RECORDING;
      break;
    case Myth::RS_ABORTED:
      tag.state = PVR_TIMER_STATE_ABORTED;
      break;
    case Myth::RS_RECORDED:
      tag.state = PVR_TIMER_STATE_COMPLETED;
      break;
    case Myth::RS_EARLIER_RECORDING:
    case Myth::RS_WILL_RECORD:
      tag.state = PVR_TIMER_STATE_SCHEDULED;
      break;
    case Myth::RS_CONFLICT:
      tag.state = PVR_TIMER_STATE_CONFLICT_NOK;
      break;
    case Myth::RS_FAILED:
      tag.state = PVR_TIMER_STATE_ERROR;
      break;
    case Myth::RS_LOW_DISKSPACE:
      tag.state = PVR_TIMER_STATE_ERROR;
      break;
    case Myth::RS_UNKNOWN:
      rulemarker.append("(").append(XBMC->GetLocalizedString(30309)).append(")"); // Not recording
      // If there is a rule then check its state
      if (node && node->IsInactiveRule())
        tag.state = PVR_TIMER_STATE_CANCELLED;
      else
        // Nothing really scheduled. Waiting for upcoming...
        tag.state = PVR_TIMER_STATE_NEW;
      break;
    default:
      tag.state = PVR_TIMER_STATE_CANCELLED;
      break;
    }

    // Title
    // Must contain the original title at the begining.
    // String will be compare with EPG title to check if it is custom or not.
    std::string title = it->second->Title();
    if (!rulemarker.empty())
      title.append(" ").append(rulemarker);
    PVR_STRCPY(tag.strTitle, this->MakeProgramTitle(title, it->second->Subtitle()).c_str());

    // Summary
    PVR_STRCPY(tag.strSummary, it->second->Description().c_str());

    // Unimplemented
    tag.iEpgUid = 0;
    tag.iLifetime = 0;
    PVR_STRCPY(tag.strDirectory, "");

    tag.iClientIndex = it->first;
    PVR->TransferTimerEntry(handle, &tag);
    // Add it to memorandom: cf UpdateTimer()
    MYTH_SHARED_PTR<PVR_TIMER> pTag = MYTH_SHARED_PTR<PVR_TIMER>(new PVR_TIMER(tag));
    m_PVRtimerMemorandum.insert(std::make_pair(tag.iClientIndex, pTag));
  }

  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s: Done", __FUNCTION__);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR PVRClientMythTV::AddTimer(const PVR_TIMER &timer)
{
  XBMC->Log(LOG_DEBUG, "%s: title: %s, start: %ld, end: %ld, chanID: %u", __FUNCTION__, timer.strTitle, timer.startTime, timer.endTime, timer.iClientChannelUid);
  CLockObject lock(m_lock);
  // Check if our timer is a quick recording of live tv
  // Assumptions: Timer start time = 0, and our live recorder is locked on the same channel.
  // If true then keep recording, setup recorder and let the backend handle the rule.
  if (timer.startTime == 0 && m_liveStream && m_liveStream->IsPlaying())
  {
    Myth::ProgramPtr program = m_liveStream->GetPlayedProgram();
    //MythProgramInfo program = MythProgramInfo(m_liveStream->GetPlayedProgram());
    if ((unsigned int)timer.iClientChannelUid == program->channel.chanId)
    //if ((unsigned int)timer.iClientChannelUid == program.ChannelID())
    {
      XBMC->Log(LOG_DEBUG, "%s: Timer is a quick recording. Toggling Record on", __FUNCTION__);
      if (m_liveStream->IsLiveRecording())
        XBMC->Log(LOG_NOTICE, "%s: Record already on! Retrying...", __FUNCTION__);
      if (m_liveStream->KeepLiveRecording(true))
        return PVR_ERROR_NO_ERROR;
      else
        // Supress error notification! XBMC locks if we return an error here.
        return PVR_ERROR_NO_ERROR;
    }
  }

  // Otherwise create the rule to schedule record
  XBMC->Log(LOG_DEBUG, "%s: Creating new recording rule", __FUNCTION__);
  MythScheduleManager::MSM_ERROR ret;

  MythRecordingRule rule = PVRtoMythRecordingRule(timer);
  ret = m_scheduleManager->ScheduleRecording(rule);
  if (ret == MythScheduleManager::MSM_ERROR_FAILED)
    return PVR_ERROR_FAILED;
  if (ret == MythScheduleManager::MSM_ERROR_NOT_IMPLEMENTED)
    return PVR_ERROR_REJECTED;

  XBMC->Log(LOG_DEBUG, "%s: Done - %d", __FUNCTION__, rule.RecordID());

  // Completion of the scheduling will be signaled by a SCHEDULE_CHANGE event.
  // Thus no need to call TriggerTimerUpdate().

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR PVRClientMythTV::DeleteTimer(const PVR_TIMER &timer, bool bForceDelete)
{
  (void)bForceDelete;
  // Check if our timer is related to rule for live recording:
  // Assumptions: Recorder handle same recording.
  // If true then expire recording, setup recorder and let backend handle the rule.
  {
    CLockObject lock(m_lock);
    if (m_liveStream && m_liveStream->IsLiveRecording())
    {
      ScheduledPtr recording = m_scheduleManager->FindUpComingByIndex(timer.iClientIndex);
      if (this->IsMyLiveRecording(*recording))
      {
        XBMC->Log(LOG_DEBUG, "%s: Timer %u is a quick recording. Toggling Record off", __FUNCTION__, timer.iClientIndex);
        if (m_liveStream->KeepLiveRecording(false))
          return PVR_ERROR_NO_ERROR;
        else
          return PVR_ERROR_FAILED;
      }
    }
  }

  // Otherwise delete scheduled rule
  XBMC->Log(LOG_DEBUG, "%s: Deleting timer %u", __FUNCTION__, timer.iClientIndex);
  MythScheduleManager::MSM_ERROR ret;

  ret = m_scheduleManager->DeleteRecording(timer.iClientIndex);
  if (ret == MythScheduleManager::MSM_ERROR_FAILED)
    return PVR_ERROR_FAILED;
  if (ret == MythScheduleManager::MSM_ERROR_NOT_IMPLEMENTED)
    return PVR_ERROR_NOT_IMPLEMENTED;

  return PVR_ERROR_NO_ERROR;
}

MythRecordingRule PVRClientMythTV::PVRtoMythRecordingRule(const PVR_TIMER &timer)
{
  MythRecordingRule rule;
  MythEPGInfo epgInfo;
  bool epgFound = false;
  time_t st = timer.startTime;
  time_t et = timer.endTime;
  time_t now = time(NULL);
  std::string title = timer.strTitle;
  std::string cs;

  ChannelIdMap::iterator channelIt = m_channelsById.find(timer.iClientChannelUid);
    if (channelIt != m_channelsById.end())
      cs = channelIt->second.Callsign();

  // Fix timeslot as needed
  if (st == 0)
    st = now;
  if (et < st)
  {
    struct tm oldtm;
    struct tm newtm;
    localtime_r(&et, &oldtm);
    localtime_r(&st, &newtm);
    newtm.tm_hour = oldtm.tm_hour;
    newtm.tm_min = oldtm.tm_min;
    newtm.tm_sec = oldtm.tm_sec;
    newtm.tm_mday++;
    et = mktime(&newtm);
  }

  // Depending of timer type, create the best rule
  if (timer.bIsRepeating)
  {
    if (timer.iWeekdays < 0x7F && timer.iWeekdays > 0)
    {
      // Move time to next day of week and find program info
      // Then create a WEEKLY record rule
      for (int bDay = 0; bDay < 7; bDay++)
      {
        if ((timer.iWeekdays & (1 << bDay)) != 0)
        {
          int n = (((bDay + 1) % 7) - weekday(&st) + 7) % 7;
          struct tm stm;
          struct tm etm;
          localtime_r(&st, &stm);
          localtime_r(&et, &etm);
          stm.tm_mday += n;
          etm.tm_mday += n;
          st = mktime(&stm);
          et = mktime(&etm);
          break;
        }
      }

      Myth::ProgramMapPtr epg = m_wsapi->GetProgramGuide(timer.iClientChannelUid, st, st);
      Myth::ProgramMap::reverse_iterator epgit = epg->rbegin(); // Get last found
      if (epgit != epg->rend() && title.compare(0, epgit->second->title.length(), epgit->second->title) == 0)
      {
        epgInfo = MythEPGInfo(epgit->second);
        epgFound = true;
      }
      else
        epgInfo = MythEPGInfo();
      rule = m_scheduleManager->NewWeeklyRecord(epgInfo);
    }
    else if (timer.iWeekdays == 0x7F)
    {
      // Create a DAILY record rule
      Myth::ProgramMapPtr epg = m_wsapi->GetProgramGuide(timer.iClientChannelUid, st, st);
      Myth::ProgramMap::reverse_iterator epgit = epg->rbegin(); // Get last found
      if (epgit != epg->rend() && title.compare(0, epgit->second->title.length(), epgit->second->title) == 0)
      {
        epgInfo = MythEPGInfo(epgit->second);
        epgFound = true;
      }
      else
        epgInfo = MythEPGInfo();
      rule = m_scheduleManager->NewDailyRecord(epgInfo);
    }
  }
  else
  {
    // Find the program info at the given start time with the same title
    // When no entry was found with the same title, then the record rule type is manual
    Myth::ProgramMapPtr epg = m_wsapi->GetProgramGuide(timer.iClientChannelUid, st, st);
    Myth::ProgramMap::reverse_iterator epgit = epg->rbegin(); // Get last found
    if (epgit != epg->rend() && title.compare(0, epgit->second->title.length(), epgit->second->title) == 0)
    {
      epgInfo = MythEPGInfo(epgit->second);
      epgFound = true;
    }
    else
      epgInfo = MythEPGInfo();
    // Create a SIGNLE record rule
    rule = m_scheduleManager->NewSingleRecord(epgInfo);
  }

  if (!epgFound)
  {
    rule.SetStartTime(st);
    rule.SetEndTime(et);
    rule.SetTitle(timer.strTitle);
    rule.SetCategory(m_categories.Category(timer.iGenreType));
    rule.SetChannelID(timer.iClientChannelUid);
    rule.SetCallsign(cs);
  }
  else
  {
    XBMC->Log(LOG_DEBUG,"%s: Found program: %u %lu %s", __FUNCTION__, epgInfo.ChannelID(), epgInfo.StartTime(), epgInfo.Title().c_str());
  }
  // Override template with PVR settings
  rule.SetStartOffset(rule.StartOffset() + timer.iMarginStart);
  rule.SetEndOffset(rule.EndOffset() + timer.iMarginEnd);
  rule.SetPriority(timer.iPriority);
  rule.SetInactive(timer.state == PVR_TIMER_STATE_ABORTED || timer.state ==  PVR_TIMER_STATE_CANCELLED);
  return rule;
}

PVR_ERROR PVRClientMythTV::UpdateTimer(const PVR_TIMER &timer)
{
  XBMC->Log(LOG_DEBUG, "%s: title: %s, start: %ld, end: %ld, chanID: %u", __FUNCTION__, timer.strTitle, timer.startTime, timer.endTime, timer.iClientChannelUid);

  MythScheduleManager::MSM_ERROR ret = MythScheduleManager::MSM_ERROR_NOT_IMPLEMENTED;
  unsigned char diffmask = 0;

  enum
  {
    CTState = 0x01, // State has changed
    CTEnabled = 0x02, // The new state
    CTTimer = 0x04 // Timer has changed
  };

  // Get the extent of changes for original timer
  std::map<unsigned int, MYTH_SHARED_PTR<PVR_TIMER> >::const_iterator old = m_PVRtimerMemorandum.find(timer.iClientIndex);
  if (old == m_PVRtimerMemorandum.end())
    return PVR_ERROR_INVALID_PARAMETERS;
  else
  {
    if (old->second->iClientChannelUid != timer.iClientChannelUid)
      diffmask |= CTTimer;
    if (old->second->bIsRepeating != timer.bIsRepeating || old->second->iWeekdays != timer.iWeekdays)
      diffmask |= CTTimer;
    if (old->second->startTime != timer.startTime || old->second->endTime != timer.endTime)
      diffmask |= CTTimer;
    if (old->second->iPriority != timer.iPriority)
      diffmask |= CTTimer;
    if (strcmp(old->second->strTitle, timer.strTitle) != 0)
      diffmask |= CTTimer;
    if ((old->second->state == PVR_TIMER_STATE_ABORTED || old->second->state == PVR_TIMER_STATE_CANCELLED)
            && timer.state != PVR_TIMER_STATE_ABORTED && timer.state != PVR_TIMER_STATE_CANCELLED)
      diffmask |= CTState | CTEnabled;
    if (old->second->state != PVR_TIMER_STATE_ABORTED && old->second->state != PVR_TIMER_STATE_CANCELLED
            && (timer.state == PVR_TIMER_STATE_ABORTED || timer.state == PVR_TIMER_STATE_CANCELLED))
      diffmask |= CTState;
  }

  if (diffmask == 0)
    return PVR_ERROR_NO_ERROR;

  if ((diffmask & CTState) && (diffmask & CTEnabled))
  {
    // Timer was disabled and will be enabled. Update timer rule before enabling.
    // Update would failed if rule is an override. So continue anyway and enable.
    if ((diffmask & CTTimer))
    {
        MythRecordingRule rule = PVRtoMythRecordingRule(timer);
        ret = m_scheduleManager->UpdateRecording(timer.iClientIndex, rule);
    }
    else
      ret = MythScheduleManager::MSM_ERROR_SUCCESS;
    if (ret != MythScheduleManager::MSM_ERROR_FAILED)
      ret = m_scheduleManager->EnableRecording(timer.iClientIndex);
  }
  else if ((diffmask & CTState) && !(diffmask & CTEnabled))
  {
    // Timer was enabled and will be disabled. Disabling could be overriden rule.
    // So don't check timer update, disable only.
    ret = m_scheduleManager->DisableRecording(timer.iClientIndex);
  }
  else if (!(diffmask & CTState) && (diffmask & CTTimer))
  {
    // State doesn't change.
    MythRecordingRule rule = PVRtoMythRecordingRule(timer);
    ret = m_scheduleManager->UpdateRecording(timer.iClientIndex, rule);
  }

  if (ret == MythScheduleManager::MSM_ERROR_FAILED)
    return PVR_ERROR_FAILED;
  if (ret == MythScheduleManager::MSM_ERROR_NOT_IMPLEMENTED)
    return PVR_ERROR_NOT_IMPLEMENTED;

  XBMC->Log(LOG_DEBUG,"%s: Done", __FUNCTION__);
  return PVR_ERROR_NO_ERROR;
}

bool PVRClientMythTV::OpenLiveStream(const PVR_CHANNEL &channel)
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG,"%s: chanid: %u, channum: %u", __FUNCTION__, channel.iUniqueId, channel.iChannelNumber);

  // Begin critical section
  CLockObject lock(m_lock);
  // First we have to get the selected channel
  LoadChannelsAndChannelGroups();
  ChannelIdMap::iterator channelByIdIt = m_channelsById.find(channel.iUniqueId);
  if (channelByIdIt == m_channelsById.end())
  {
    XBMC->Log(LOG_ERROR,"%s: Channel not found", __FUNCTION__);
    return false;
  }
  // Copy and check
  Myth::ChannelPtr chan = channelByIdIt->second.GetPtr();
  if (!chan)
  {
    XBMC->Log(LOG_ERROR,"%s: Invalid channel", __FUNCTION__);
    return false;
  }
  // Need to create live
  if (!m_liveStream)
    m_liveStream = new Myth::LiveTVPlayback(*m_eventHandler);
  else if (m_liveStream->IsPlaying())
    return false;
  // Suspend fileOps to avoid connection hang
  m_fileOps->Suspend();
  // Set tuning delay
  m_liveStream->SetTuneDelay(g_iTuneDelay);
  // Try to open
  if (m_liveStream->SpawnLiveTV(*chan))
  {
    if(g_bDemuxing)
      m_demux = new Demux(m_liveStream);
    XBMC->Log(LOG_DEBUG, "%s: Done", __FUNCTION__);
    return true;
  }

  SAFE_DELETE(m_liveStream);
  // Resume fileOps
  m_fileOps->Resume();
  XBMC->Log(LOG_ERROR,"%s: Failed to open live stream", __FUNCTION__);
  XBMC->QueueNotification(QUEUE_WARNING, XBMC->GetLocalizedString(30305)); // Channel unavailable
  return false;
}

void PVRClientMythTV::CloseLiveStream()
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  // Begin critical section
  CLockObject lock(m_lock);
  // Destroy my demuxer
  if (m_demux)
    SAFE_DELETE(m_demux);
  // Destroy my stream
  if (m_liveStream)
    SAFE_DELETE(m_liveStream);
  // Resume fileOps
  m_fileOps->Resume();

  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s: Done", __FUNCTION__);
}

int PVRClientMythTV::ReadLiveStream(unsigned char *pBuffer, unsigned int iBufferSize)
{
  // Keep unlocked
  return (m_liveStream ? m_liveStream->Read(pBuffer, iBufferSize) : -1);
}

int PVRClientMythTV::GetCurrentClientChannel()
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  // Begin critical section
  CLockObject lock(m_lock);
  // Have live stream
  if (!m_liveStream)
    return -1;

  Myth::ProgramPtr program = m_liveStream->GetPlayedProgram();
  return (int)program->channel.chanId;
}

bool PVRClientMythTV::SwitchChannel(const PVR_CHANNEL &channel)
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG,"%s: chanid: %u, channum: %u", __FUNCTION__, channel.iUniqueId, channel.iChannelNumber);

  // Begin critical section
  CLockObject lock(m_lock);
  // Have live stream
  if (!m_liveStream)
    return false;
  // Destroy my demuxer for reopening
  if (m_demux)
    SAFE_DELETE(m_demux);
  // Stop the live for reopening
  m_liveStream->StopLiveTV();
  // Try reopening for channel
  if (OpenLiveStream(channel))
    return true;
  return false;
}

long long PVRClientMythTV::SeekLiveStream(long long iPosition, int iWhence)
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s: pos: %lld, whence: %d", __FUNCTION__, iPosition, iWhence);

  if (!m_liveStream)
    return -1;

  Myth::WHENCE_t whence;
  switch (iWhence)
  {
  case SEEK_SET:
    whence = Myth::WHENCE_SET;
    break;
  case SEEK_CUR:
    whence = Myth::WHENCE_CUR;
    break;
  case SEEK_END:
    whence = Myth::WHENCE_END;
    break;
  default:
    return -1;
  }

  long long retval = (long long) m_liveStream->Seek((int64_t)iPosition, whence);

  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s: Done - position: %lld", __FUNCTION__, retval);

  return retval;
}

long long PVRClientMythTV::LengthLiveStream()
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  if (!m_liveStream)
    return -1;

  long long retval = (long long) m_liveStream->GetSize();

  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s: Done - duration: %lld", __FUNCTION__, retval);

  return retval;
}

PVR_ERROR PVRClientMythTV::SignalStatus(PVR_SIGNAL_STATUS &signalStatus)
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  CLockObject lock(m_lock);
  if (!m_liveStream)
    return PVR_ERROR_SERVER_ERROR;

  char buf[50];
  sprintf(buf, "Myth Recorder %u", (unsigned)m_liveStream->GetCardId());
  PVR_STRCPY(signalStatus.strAdapterName, buf);
  Myth::SignalStatusPtr signal = m_liveStream->GetSignal();
  if (signal)
  {
    if (signal->lock)
      PVR_STRCPY(signalStatus.strAdapterStatus, "Locked");
    else
      PVR_STRCPY(signalStatus.strAdapterStatus, "No lock");
    signalStatus.dAudioBitrate = 0;
    signalStatus.dDolbyBitrate = 0;
    signalStatus.dVideoBitrate = 0;
    signalStatus.iSignal = signal->signal;
    signalStatus.iBER = signal->ber;
    signalStatus.iSNR = signal->snr;
    signalStatus.iUNC = signal->ucb;
  }

  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s: Done", __FUNCTION__);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR PVRClientMythTV::GetStreamProperties(PVR_STREAM_PROPERTIES* pProperties)
{
  return m_demux && m_demux->GetStreamProperties(pProperties) ? PVR_ERROR_NO_ERROR : PVR_ERROR_SERVER_ERROR;
}

void PVRClientMythTV::DemuxAbort(void)
{
  if (m_demux)
    m_demux->Abort();
}

void PVRClientMythTV::DemuxFlush(void)
{
  if (m_demux)
    m_demux->Flush();
}

DemuxPacket* PVRClientMythTV::DemuxRead(void)
{
  return m_demux ? m_demux->Read() : NULL;
}

bool PVRClientMythTV::SeekTime(int time, bool backwards, double* startpts)
{
  return m_demux ? m_demux->SeekTime(time, backwards, startpts) : false;
}

time_t PVRClientMythTV::GetPlayingTime()
{
  CLockObject lock(m_lock);
  if (!m_liveStream || !m_demux)
    return 0;
  int sec = m_demux->GetPlayingTime() / 1000;
  time_t st = GetBufferTimeStart();
  struct tm playtm;
  localtime_r(&st, &playtm);
  playtm.tm_sec += sec;
  time_t pt = mktime(&playtm);
  return pt;
}

time_t PVRClientMythTV::GetBufferTimeStart()
{
  CLockObject lock(m_lock);
  if (!m_liveStream || !m_liveStream->IsPlaying())
    return 0;
  return m_liveStream->GetLiveTimeStart();
}

time_t PVRClientMythTV::GetBufferTimeEnd()
{
  CLockObject lock(m_lock);
  unsigned count;
  if (!m_liveStream || !(count = m_liveStream->GetChainedCount()))
    return (time_t)(-1);
  time_t now = time(NULL);
  MythProgramInfo prog = MythProgramInfo(m_liveStream->GetChainedProgram(count));
  return (now > prog.RecordingEndTime() ? prog.RecordingEndTime() : now);
}

bool PVRClientMythTV::OpenRecordedStream(const PVR_RECORDING &recording)
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s: title: %s, ID: %s, duration: %d", __FUNCTION__, recording.strTitle, recording.strRecordingId, recording.iDuration);

  MythProgramInfo prog;
  {
    CLockObject lock(m_recordingsLock);
    ProgramInfoMap::iterator it = m_recordings.find(recording.strRecordingId);
    if (it == m_recordings.end())
    {
      XBMC->Log(LOG_ERROR, "%s: Recording %s does not exist", __FUNCTION__, recording.strRecordingId);
      return false;
    }
    prog = it->second;
  }

  // Begin critical section
  CLockObject lock(m_lock);
  // Suspend fileOps to avoid connection hang
  m_fileOps->Suspend();

  if (!m_recordingStream)
  {
    if (prog.HostName() == m_wsapi->GetServerHostName())
      // Request the stream from our master using the opened event handler.
      m_recordingStream = new Myth::RecordingPlayback(*m_eventHandler);
    else
      // Request the stream from slave host. A dedicated event handler will be opened.
      m_recordingStream = new Myth::RecordingPlayback(prog.HostName(), g_iProtoPort);
  }
  else
    m_recordingStream->Open();

  if (!m_recordingStream->IsOpen())
    XBMC->QueueNotification(QUEUE_ERROR, XBMC->GetLocalizedString(30302)); // MythTV backend unavailable
  else if (m_recordingStream->OpenTransfer(prog.GetPtr()))
  {
    if (g_bExtraDebug)
      XBMC->Log(LOG_DEBUG, "%s: Done", __FUNCTION__);
    return true;
  }

  SAFE_DELETE(m_recordingStream);
  // Resume fileOps
  m_fileOps->Resume();
  XBMC->Log(LOG_ERROR,"%s: Failed to open recorded stream", __FUNCTION__);
  return false;
}

void PVRClientMythTV::CloseRecordedStream()
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  // Begin critical section
  CLockObject lock(m_lock);
  // Destroy my stream
  SAFE_DELETE(m_recordingStream);
  // Resume fileOps
  m_fileOps->Resume();

  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s: Done", __FUNCTION__);
}

int PVRClientMythTV::ReadRecordedStream(unsigned char *pBuffer, unsigned int iBufferSize)
{
  // Keep unlocked
  return (m_recordingStream ? m_recordingStream->Read(pBuffer, iBufferSize) : -1);
}

long long PVRClientMythTV::SeekRecordedStream(long long iPosition, int iWhence)
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s: pos: %lld, whence: %d", __FUNCTION__, iPosition, iWhence);

  if (!m_recordingStream)
    return -1;

  Myth::WHENCE_t whence;
  switch (iWhence)
  {
  case SEEK_SET:
    whence = Myth::WHENCE_SET;
    break;
  case SEEK_CUR:
    whence = Myth::WHENCE_CUR;
    break;
  case SEEK_END:
    whence = Myth::WHENCE_END;
    break;
  default:
    return -1;
  }

  long long retval = (long long) m_recordingStream->Seek((int64_t)iPosition, whence);

  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s: Done - position: %lld", __FUNCTION__, retval);

  return retval;
}

long long PVRClientMythTV::LengthRecordedStream()
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  if (!m_recordingStream)
    return -1;

  long long retval = (long long) m_recordingStream->GetSize();

  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s: Done - duration: %lld", __FUNCTION__, retval);

  return retval;
}

PVR_ERROR PVRClientMythTV::CallMenuHook(const PVR_MENUHOOK &menuhook, const PVR_MENUHOOK_DATA &item)
{
  if (menuhook.iHookId == MENUHOOK_REC_DELETE_AND_RERECORD && item.cat == PVR_MENUHOOK_RECORDING) {
    return DeleteAndForgetRecording(item.data.recording);
  }

  if (menuhook.iHookId == MENUHOOK_KEEP_LIVETV_RECORDING && item.cat == PVR_MENUHOOK_RECORDING)
  {
    CLockObject lock(m_recordingsLock);
    ProgramInfoMap::iterator it = m_recordings.find(item.data.recording.strRecordingId);
    if (it == m_recordings.end())
    {
      XBMC->Log(LOG_ERROR,"%s: Recording not found", __FUNCTION__);
      return PVR_ERROR_INVALID_PARAMETERS;
    }

    if (!it->second.IsLiveTV())
      return PVR_ERROR_NO_ERROR;

    // If recording is current live show then keep it and set live recorder
    if (IsMyLiveRecording(it->second))
    {
      CLockObject lock(m_lock);
      if (m_liveStream && m_liveStream->KeepLiveRecording(true))
        return PVR_ERROR_NO_ERROR;
      return PVR_ERROR_FAILED;
    }
    // Else keep old live recording
    else
    {
      if (m_control->UndeleteRecording(*(it->second.GetPtr())))
      {
        std::string info = XBMC->GetLocalizedString(menuhook.iLocalizedStringId);
        info.append(": ").append(it->second.Title());
        XBMC->QueueNotification(QUEUE_INFO, info.c_str());
        return PVR_ERROR_NO_ERROR;
      }
    }
    return PVR_ERROR_FAILED;
  }

  if (menuhook.category == PVR_MENUHOOK_SETTING)
  {
    if (menuhook.iHookId == MENUHOOK_SHOW_HIDE_NOT_RECORDING && m_scheduleManager)
    {
      bool flag = m_scheduleManager->ToggleShowNotRecording();
      HandleScheduleChange();
      std::string info = (flag ? XBMC->GetLocalizedString(30310) : XBMC->GetLocalizedString(30311));
      XBMC->QueueNotification(QUEUE_INFO, info.c_str());
      return PVR_ERROR_NO_ERROR;
    }
  }

  if (menuhook.category == PVR_MENUHOOK_EPG && item.cat == PVR_MENUHOOK_EPG)
  {
    time_t attime;
    unsigned int chanid;
    BreakBroadcastID(item.data.iEpgUid, &chanid, &attime);
    MythEPGInfo epgInfo;
    Myth::ProgramMapPtr epg = m_wsapi->GetProgramGuide(chanid, attime, attime);
    Myth::ProgramMap::reverse_iterator epgit = epg->rbegin(); // Get last found
    if (epgit != epg->rend())
    {
      epgInfo = MythEPGInfo(epgit->second);
      // Scheduling actions
      if (m_scheduleManager)
      {
        MythRecordingRule rule;
        switch(menuhook.iHookId)
        {
          case MENUHOOK_EPG_REC_CHAN_ALL_SHOWINGS:
            rule = m_scheduleManager->NewChannelRecord(epgInfo);
            break;
          case MENUHOOK_EPG_REC_CHAN_WEEKLY:
            rule = m_scheduleManager->NewWeeklyRecord(epgInfo);
            break;
          case MENUHOOK_EPG_REC_CHAN_DAILY:
            rule = m_scheduleManager->NewDailyRecord(epgInfo);
            break;
          case MENUHOOK_EPG_REC_ONE_SHOWING:
            rule = m_scheduleManager->NewOneRecord(epgInfo);
            break;
          case MENUHOOK_EPG_REC_NEW_EPISODES:
            rule = m_scheduleManager->NewChannelRecord(epgInfo);
            rule.SetFilter(rule.Filter() | Myth::FM_FirstShowing);
            break;
          default:
            return PVR_ERROR_NOT_IMPLEMENTED;
        }
        if (m_scheduleManager->ScheduleRecording(rule) == MythScheduleManager::MSM_ERROR_SUCCESS)
          return PVR_ERROR_NO_ERROR;
      }
    }
    else
    {
      XBMC->QueueNotification(QUEUE_WARNING, XBMC->GetLocalizedString(30312));
      XBMC->Log(LOG_DEBUG, "%s: broadcast: %d chanid: %u attime: %lu", __FUNCTION__, item.data.iEpgUid, chanid, attime);
      return PVR_ERROR_INVALID_PARAMETERS;
    }
    return PVR_ERROR_FAILED;
  }

  return PVR_ERROR_NOT_IMPLEMENTED;
}

bool PVRClientMythTV::GetLiveTVPriority()
{
  if (m_wsapi)
  {
    Myth::SettingPtr setting = m_wsapi->GetSetting("LiveTVPriority", true);
    return ((setting && setting->value.compare("1") == 0) ? true : false);
  }
  return false;
}

void PVRClientMythTV::SetLiveTVPriority(bool enabled)
{
  if (m_wsapi)
  {
    std::string value = (enabled ? "1" : "0");
    m_wsapi->PutSetting("LiveTVPriority", value, true);
  }
}

std::string PVRClientMythTV::MakeProgramTitle(const std::string& title, const std::string& subtitle) const
{
  std::string epgtitle;
  if (subtitle.empty())
    epgtitle = title;
  else
    epgtitle = title + SUBTITLE_SEPARATOR + subtitle;
  return epgtitle;
}

// Broacast ID is 32 bits integer and allows to identify a EPG item.
// MythTV backend doesn't provide one. So we make it encoding time and channel
// as below:
// 31. . . . . . . . . . . . . . . 15. . . . . . . . . . . . . . 0
// [   timecode (self-relative)   ][         channel Id          ]
// Timecode is the count of minutes since epoch modulo 0xFFFF. Now therefore it
// is usable for a period of +/- 32767 minutes (+/-22 days) around itself.

int PVRClientMythTV::MakeBroadcastID(unsigned int chanid, time_t starttime) const
{
  int timecode = (int)(difftime(starttime, 0) / 60) & 0xFFFF;
  return (int)((timecode << 16) | (chanid & 0xFFFF));
}

void PVRClientMythTV::BreakBroadcastID(int broadcastid, unsigned int *chanid, time_t *attime) const
{
  time_t now;
  int ntc, ptc, distance;
  struct tm epgtm;

  now = time(NULL);
  ntc = (int)(difftime(now, 0) / 60) & 0xFFFF;
  ptc = (broadcastid >> 16) & 0xFFFF; // removes arithmetic bits
  if (ptc > ntc)
    distance = (ptc - ntc) < 0x8000 ? ptc - ntc : ptc - ntc - 0xFFFF;
  else
    distance = (ntc - ptc) < 0x8000 ? ptc - ntc : ptc - ntc + 0xFFFF;
  localtime_r(&now, &epgtm);
  epgtm.tm_min += distance;
  // Time precision is minute, so we are looking for program started before next minute.
  epgtm.tm_sec = 59;

  *attime = mktime(&epgtm);
  *chanid = (unsigned int)broadcastid & 0xFFFF;
}
