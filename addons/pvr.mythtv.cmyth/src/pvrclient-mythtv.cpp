/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "pvrclient-mythtv.h"

#include "client.h"
#include "tools.h"

#include <time.h>
#include <inttypes.h>
#include <set>

using namespace ADDON;
using namespace PLATFORM;

PVRClientMythTV::PVRClientMythTV()
 : m_con()
 , m_pEventHandler(NULL)
 , m_db()
 , m_fileOps(NULL)
 , m_scheduleManager(NULL)
 , m_backendVersion("")
 , m_connectionString("")
 , m_categories()
 , m_channelGroups()
 , m_demux(NULL)
{
}

PVRClientMythTV::~PVRClientMythTV()
{
  if (m_fileOps)
  {
    delete m_fileOps;
    m_fileOps = NULL;
  }

  if (m_pEventHandler)
  {
    delete m_pEventHandler;
    m_pEventHandler = NULL;
  }

  if (m_scheduleManager)
  {
    delete m_scheduleManager;
    m_scheduleManager = NULL;
  }

  if (m_demux)
  {
    delete m_demux;
    m_demux = NULL;
  }
}

void Log(int level, char *msg)
{
  if (msg && level != CMYTH_DBG_NONE)
  {
    bool doLog = g_bExtraDebug;
    addon_log_t loglevel = LOG_DEBUG;
    switch (level)
    {
    case CMYTH_DBG_ERROR:
      loglevel = LOG_ERROR;
      doLog = true;
      break;
    case CMYTH_DBG_WARN:
    case CMYTH_DBG_INFO:
      loglevel = LOG_INFO;
      break;
    case CMYTH_DBG_DETAIL:
    case CMYTH_DBG_DEBUG:
    case CMYTH_DBG_PROTO:
    case CMYTH_DBG_ALL:
      loglevel = LOG_DEBUG;
      break;
    }
    if (XBMC && doLog)
      XBMC->Log(loglevel, "LibCMyth: %s", msg);
  }
}

bool PVRClientMythTV::Connect()
{
  // Setup libcmyth logging
  if (g_bExtraDebug)
    cmyth_dbg_all();
  else
    cmyth_dbg_level(CMYTH_DBG_ERROR);
  cmyth_set_dbg_msgcallback(Log);

  // Create MythTV connection
  m_con = MythConnection(g_szMythHostname, g_iMythPort, false);
  if (!m_con.IsConnected())
  {
    XBMC->Log(LOG_ERROR,"Failed to connect to MythTV backend on %s:%d", g_szMythHostname.c_str(), g_iMythPort);
    return false;
  }

  // Create event handler
  m_pEventHandler = m_con.CreateEventHandler();
  if (!m_pEventHandler)
  {
    XBMC->Log(LOG_ERROR, "Failed to create MythEventHandler");
    XBMC->QueueNotification(QUEUE_ERROR, XBMC->GetLocalizedString(30300));
    return false;
  }
  m_pEventHandler->RegisterObserver(this);

  // Create database connection
  m_db = MythDatabase(g_szDBHostname, g_szDBName, g_szDBUser, g_szDBPassword, g_iDBPort);
  if (m_db.IsNull())
  {
    XBMC->Log(LOG_ERROR,"Failed to connect to MythTV database %s@%s:%d with user %s", g_szDBName.c_str(), g_szDBHostname.c_str(), g_iDBPort, g_szDBUser.c_str());
    XBMC->QueueNotification(QUEUE_ERROR, XBMC->GetLocalizedString(30301));
    return false;
  }

  // Test database connection
  CStdString db_test;
  if (!m_db.TestConnection(&db_test))
  {
    XBMC->Log(LOG_ERROR,"Failed to connect to MythTV database %s@%s:%d with user %s: %s", g_szDBName.c_str(), g_szDBHostname.c_str(), g_iDBPort, g_szDBUser.c_str(), db_test.c_str());
    XBMC->QueueNotification(QUEUE_ERROR, XBMC->GetLocalizedString(30301));
    return false;
  }

  // Create file operation helper (image caching)
  m_fileOps = new FileOps(m_con);

  // Create schedule manager
  m_scheduleManager = new MythScheduleManager(m_con, m_db);

  return true;
}

const char *PVRClientMythTV::GetBackendName()
{
  m_backendName.Format("MythTV (%s)", m_con.GetBackendName());
  XBMC->Log(LOG_DEBUG, "GetBackendName: %s", m_backendName.c_str());
  return m_backendName;
}

const char *PVRClientMythTV::GetBackendVersion()
{
  m_backendVersion.Format(XBMC->GetLocalizedString(30100), m_con.GetProtocolVersion(), m_db.GetSchemaVersion());
  XBMC->Log(LOG_DEBUG, "GetBackendVersion: %s", m_backendVersion.c_str());
  return m_backendVersion;
}

const char *PVRClientMythTV::GetConnectionString()
{
  m_connectionString.Format("%s:%d - %s@%s:%d", g_szMythHostname, g_iMythPort, g_szDBName, g_szDBHostname, g_iDBPort);
  XBMC->Log(LOG_DEBUG, "GetConnectionString: %s", m_connectionString.c_str());
  return m_connectionString;
}

bool PVRClientMythTV::GetDriveSpace(long long *iTotal, long long *iUsed)
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);
  return m_con.GetDriveSpace(*iTotal, *iUsed);
}

void PVRClientMythTV::OnSleep()
{
  if (m_pEventHandler)
    m_pEventHandler->Suspend();
  if (m_fileOps)
    m_fileOps->Suspend();
}

void PVRClientMythTV::OnWake()
{
  if (m_pEventHandler)
    m_pEventHandler->Resume(true);
  if (m_fileOps)
    m_fileOps->Resume();
}

PVR_ERROR PVRClientMythTV::GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd)
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG,"%s - start: %ld, end: %ld, ChannelID: %u", __FUNCTION__, iStart, iEnd, channel.iUniqueId);

  if (!channel.bIsHidden)
  {
    EPGInfoMap EPG = m_db.GetGuide(channel.iUniqueId, iStart, iEnd);
    // Transfer EPG for the given channel
    for (EPGInfoMap::reverse_iterator it = EPG.rbegin(); it != EPG.rend(); ++it)
    {
      EPG_TAG tag;
      memset(&tag, 0, sizeof(EPG_TAG));
      tag.startTime = it->first;
      tag.endTime = it->second.EndTime();
      // Reject bad entry
      if (tag.endTime <= tag.startTime)
        continue;

      // EPG_TAG expects strings as char* and not as copies (like the other PVR types).
      // Therefore we have to make sure that we don't pass invalid (freed) memory to TransferEpgEntry.
      // In particular we have to use local variables and must not pass returned CStdString values directly.
      CStdString title;
      CStdString description;
      CStdString category;

      tag.iUniqueBroadcastId = MakeBroadcastID(it->second.ChannelID(), it->second.StartTime());
      tag.iChannelNumber = it->second.ChannelNumberInt();
      title = this->MakeProgramTitle(it->second.Title(), it->second.Subtitle());
      tag.strTitle = title;
      description = it->second.Description();
      tag.strPlot = description;

      int genre = m_categories.Category(it->second.Category());
      tag.iGenreSubType = genre & 0x0F;
      tag.iGenreType = genre & 0xF0;
      category = it->second.Category();
      tag.strGenreDescription = category;

      // Unimplemented
      tag.strEpisodeName = "";
      tag.strIconPath = "";
      tag.strPlotOutline = "";
      tag.bNotify = false;
      tag.firstAired = 0;
      tag.iEpisodeNumber = 0;
      tag.iEpisodePartNumber = 0;
      tag.iParentalRating = 0;
      tag.iSeriesNumber = 0;
      tag.iStarRating = 0;

      PVR->TransferEpgEntry(handle, &tag);
    }
  }

  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s - Done", __FUNCTION__);

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
    XBMC->Log(LOG_DEBUG, "%s - radio: %s", __FUNCTION__, (bRadio ? "true" : "false"));

  LoadChannelsAndChannelGroups();
  m_PVRChannelUidById.clear();

  // Create a map<(channum, callsign), chanid> to merge channels with same channum and callsign
  std::map<std::pair<CStdString, CStdString>, unsigned int> channelIdentifiers;

  // Transfer channels of the requested type (radio / tv)
  for (ChannelIdMap::iterator it = m_channelsById.begin(); it != m_channelsById.end(); ++it)
  {
    if (it->second.IsRadio() == bRadio && !it->second.IsNull())
    {
      // Skip channels with same channum and callsign
      std::pair<CStdString, CStdString> channelIdentifier = std::make_pair(it->second.Number(), it->second.Callsign());
      std::map<std::pair<CStdString, CStdString>, unsigned int>::iterator itm = channelIdentifiers.find(channelIdentifier);
      if (itm != channelIdentifiers.end())
      {
        XBMC->Log(LOG_DEBUG, "%s - skipping channel: %d", __FUNCTION__, it->second.ID());
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
      tag.iChannelNumber = it->second.NumberInt();
      PVR_STRCPY(tag.strChannelName, it->second.Name());
      tag.bIsHidden = !it->second.Visible();
      tag.bIsRadio = it->second.IsRadio();

      CStdString icon = m_fileOps->GetChannelIconPath(it->second.Icon());
      PVR_STRCPY(tag.strIconPath, icon);

      // Unimplemented
      PVR_STRCPY(tag.strStreamURL, "");
      PVR_STRCPY(tag.strInputFormat, "");
      tag.iEncryptionSystem = 0;

      PVR->TransferChannelEntry(handle, &tag);
    }
  }

  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s - Done", __FUNCTION__);

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
    XBMC->Log(LOG_DEBUG, "%s - radio: %s", __FUNCTION__, (bRadio ? "true" : "false"));

  LoadChannelsAndChannelGroups();

  // Transfer channel groups of the given type (radio / tv)
  for (ChannelGroupMap::iterator channelGroupsIt = m_channelGroups.begin(); channelGroupsIt != m_channelGroups.end(); ++channelGroupsIt)
  {
    PVR_CHANNEL_GROUP tag;
    memset(&tag, 0, sizeof(PVR_CHANNEL_GROUP));

    PVR_STRCPY(tag.strGroupName, channelGroupsIt->first);
    tag.bIsRadio = bRadio;

    // Only add the group if we have at least one channel of the correct type
    for (std::vector<int>::iterator channelGroupIt = channelGroupsIt->second.begin(); channelGroupIt != channelGroupsIt->second.end(); ++channelGroupIt)
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
    XBMC->Log(LOG_DEBUG, "%s - Done", __FUNCTION__);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR PVRClientMythTV::GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group)
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s - group: %s", __FUNCTION__, group.strGroupName);

  LoadChannelsAndChannelGroups();

  ChannelGroupMap::iterator channelGroupsIt = m_channelGroups.find(group.strGroupName);
  if (channelGroupsIt == m_channelGroups.end())
  {
    XBMC->Log(LOG_ERROR,"%s - Channel group not found", __FUNCTION__);
    return PVR_ERROR_INVALID_PARAMETERS;
  }

  // Transfer the channel group members for the requested group
  int channelNumber = 0;
  for (std::vector<int>::iterator channelGroupIt = channelGroupsIt->second.begin(); channelGroupIt != channelGroupsIt->second.end(); ++channelGroupIt)
  {
    ChannelIdMap::iterator channelIt = m_channelsById.find(*channelGroupIt);
    if (channelIt != m_channelsById.end() && channelIt->second.IsRadio() == group.bIsRadio)
    {
      PVR_CHANNEL_GROUP_MEMBER tag;
      memset(&tag, 0, sizeof(PVR_CHANNEL_GROUP_MEMBER));

      tag.iChannelNumber = channelNumber++;
      tag.iChannelUniqueId = FindPVRChannelUid(channelIt->second.ID());
      PVR_STRCPY(tag.strGroupName, group.strGroupName);
      PVR->TransferChannelGroupMember(handle, &tag);
    }
  }

  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s - Done", __FUNCTION__);

  return PVR_ERROR_NO_ERROR;
}

void PVRClientMythTV::LoadChannelsAndChannelGroups()
{
  if (!m_channelsById.empty())
    return;

  m_channelsById = m_db.GetChannels();

  for (ChannelIdMap::iterator channelIt = m_channelsById.begin(); channelIt != m_channelsById.end(); ++channelIt)
    m_channelsByNumber.insert(std::make_pair(channelIt->second.Number(), channelIt->second));

  m_channelGroups = m_db.GetChannelGroups();
}

int PVRClientMythTV::FindPVRChannelUid(int channelId) const
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

  if (m_recordings.empty())
    // Load recorings list
    FillRecordings();
  else
    // Update recording list from change events
    EventUpdateRecordings();

  for (ProgramInfoMap::iterator it = m_recordings.begin(); it != m_recordings.end(); ++it)
  {
    if (!it->second.IsNull() && it->second.IsVisible())
    {
      PVR_RECORDING tag;
      memset(&tag, 0, sizeof(PVR_RECORDING));

      tag.recordingTime = it->second.RecordingStartTime();
      tag.iDuration = it->second.Duration();
      tag.iPlayCount = it->second.IsWatched() ? 1 : 0;
      tag.iLastPlayedPosition = GetRecordingLastPlayedPosition(it->second);

      CStdString id = it->second.UID();
      CStdString title = this->MakeProgramTitle(it->second.Title(), it->second.Subtitle());

      PVR_STRCPY(tag.strRecordingId, id);
      PVR_STRCPY(tag.strTitle, title);
      PVR_STRCPY(tag.strPlot, it->second.Description());
      PVR_STRCPY(tag.strChannelName, it->second.ChannelName());

      int genre = m_categories.Category(it->second.Category());
      tag.iGenreSubType = genre&0x0F;
      tag.iGenreType = genre&0xF0;

      // Add recording title to directory to group everything according to its name just like MythTV does
      CStdString strDirectory;
      strDirectory.Format("%s/%s", it->second.RecordingGroup(), it->second.Title());
      PVR_STRCPY(tag.strDirectory, strDirectory);

      // Images
      CStdString strIconPath;
      if (!it->second.Coverart().IsEmpty())
        strIconPath = m_fileOps->GetArtworkPath(it->second.Coverart(), FileOps::FileTypeCoverart);
      else if (it->second.IsLiveTV())
      {
        MythChannel channel = FindRecordingChannel(it->second);
        if (!channel.IsNull())
          strIconPath = m_fileOps->GetChannelIconPath(channel.Icon());
      }
      else
        strIconPath = m_fileOps->GetPreviewIconPath(it->second.IconPath(), it->second.StorageGroup());

      CStdString strFanartPath;
      if (!it->second.Fanart().IsEmpty())
        strFanartPath = m_fileOps->GetArtworkPath(it->second.Fanart(), FileOps::FileTypeFanart);

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
    XBMC->Log(LOG_DEBUG, "%s - Done", __FUNCTION__);

  return PVR_ERROR_NO_ERROR;
}

void PVRClientMythTV::EventUpdateRecordings()
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  // Check event connection
  if (!m_pEventHandler->IsListening())
    return;

  while (true)
  {
    if (!m_pEventHandler->HasRecordingChangeEvent())
      break;

    MythEventHandler::RecordingChangeEvent event = m_pEventHandler->NextRecordingChangeEvent();

    switch (event.Type())
    {
      case MythEventHandler::CHANGE_ALL:
      {
        if (g_bExtraDebug)
          XBMC->Log(LOG_DEBUG, "%s - Reload all recordings", __FUNCTION__);
        FillRecordings();
        break;
      }
      case MythEventHandler::CHANGE_ADD:
      {
        MythProgramInfo prog = m_con.GetRecordedProgram(event.ChannelID(), event.RecordingStartTimeslot());
        if (!prog.IsNull())
        {
          ProgramInfoMap::iterator it = m_recordings.find(prog.UID());
          if (it == m_recordings.end())
          {
            if (g_bExtraDebug)
              XBMC->Log(LOG_DEBUG, "%s - Add recording: %s", __FUNCTION__, prog.UID().c_str());

            // Fill artwork
            m_db.FillRecordingArtwork(prog);

            // Add recording
            m_recordings.insert(std::pair<CStdString, MythProgramInfo>(prog.UID().c_str(), prog));
          }
        }
        else
          XBMC->Log(LOG_ERROR, "%s - Add recording failed for %u %s", __FUNCTION__, event.ChannelID(), event.RecordingStartTimeslot().NumString().c_str());
        break;
      }
      case MythEventHandler::CHANGE_UPDATE:
      {
        MythProgramInfo prog = event.Program();
        ProgramInfoMap::iterator it = m_recordings.find(prog.UID());
        if (it != m_recordings.end())
        {
          if (g_bExtraDebug)
            XBMC->Log(LOG_DEBUG, "%s - Update recording: %s", __FUNCTION__, prog.UID().c_str());

          // Copy cached framerate
          prog.SetFrameRate(it->second.FrameRate());

          // Fill artwork
          m_db.FillRecordingArtwork(prog);

          // Update recording
          it->second = prog;
        }
        break;
      }
      case MythEventHandler::CHANGE_DELETE:
      {
        // MythTV send two DELETE events. First requests deletion, second confirms deletion.
        // On first we delete recording. On second program will not be found.
        MythProgramInfo prog = m_con.GetRecordedProgram(event.ChannelID(), event.RecordingStartTimeslot());
        if (!prog.IsNull())
        {
          ProgramInfoMap::iterator it = m_recordings.find(prog.UID());
          if (it != m_recordings.end())
          {
            if (g_bExtraDebug)
              XBMC->Log(LOG_DEBUG, "%s - Delete recording: %s", __FUNCTION__, prog.UID().c_str());

            // Remove recording
            m_recordings.erase(it);
          }
        }
        break;
      }
    }
  }
}

void PVRClientMythTV::ForceUpdateRecording(ProgramInfoMap::iterator it)
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  if (!it->second.IsNull())
  {
    MythProgramInfo prog = m_con.GetRecordedProgram(it->second.BaseName());
    if (!prog.IsNull())
    {
      // Copy cached framerate
      prog.SetFrameRate(it->second.FrameRate());

      // Fill artwork
      m_db.FillRecordingArtwork(prog);

      // Update recording
      it->second = prog;
      PVR->TriggerRecordingUpdate();

      if (g_bExtraDebug)
        XBMC->Log(LOG_DEBUG, "%s - Done", __FUNCTION__);
    }
  }
}

int PVRClientMythTV::FillRecordings()
{
  int res = 0;
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  // Check event connection
  if (!m_pEventHandler->IsListening())
    return res;

  // Clear all recording change events
  m_pEventHandler->ClearRecordingChangeEvents();

  // Load recordings list
  m_recordings.clear();
  m_recordings = m_con.GetRecordedPrograms();

  // Fill artworks
  for (ProgramInfoMap::iterator it = m_recordings.begin(); it != m_recordings.end(); ++it)
  {
    if (!it->second.IsNull() && it->second.IsVisible() && !it->second.IsLiveTV())
    {
      m_db.FillRecordingArtwork(it->second);
      res++;
    }
  }
  return res;
}

PVR_ERROR PVRClientMythTV::DeleteRecording(const PVR_RECORDING &recording)
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  CLockObject lock(m_recordingsLock);

  ProgramInfoMap::iterator it = m_recordings.find(recording.strRecordingId);
  if (it != m_recordings.end())
  {
    // Deleting Live recording is prohibited. Otherwise continue
    if (this->IsMyLiveTVRecording(it->second))
    {
      if (it->second.IsLiveTV())
        return PVR_ERROR_RECORDING_RUNNING;
      // it is kept then ignore it now.
      if (KeepLiveTVRecording(it->second, false) && m_rec.SetLiveRecording(false))
        return PVR_ERROR_NO_ERROR;
      else
        return PVR_ERROR_FAILED;
    }
    bool ret = m_con.DeleteRecording(it->second);
    if (ret)
    {
      XBMC->Log(LOG_DEBUG, "%s - Deleted recording %s", __FUNCTION__, recording.strRecordingId);
      return PVR_ERROR_NO_ERROR;
    }
    else
    {
      XBMC->Log(LOG_ERROR, "%s - Failed to delete recording %s", __FUNCTION__, recording.strRecordingId);
    }
  }
  else
  {
    XBMC->Log(LOG_ERROR, "%s - Recording %s does not exist", __FUNCTION__, recording.strRecordingId);
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
    if (this->IsMyLiveTVRecording(it->second))
    {
      if (it->second.IsLiveTV())
        return PVR_ERROR_RECORDING_RUNNING;
      // it is kept then ignore it now.
      if (KeepLiveTVRecording(it->second, false) && m_rec.SetLiveRecording(false))
        return PVR_ERROR_NO_ERROR;
      else
        return PVR_ERROR_FAILED;
    }
    bool ret = m_con.DeleteAndForgetRecording(it->second);
    if (ret)
    {
      XBMC->Log(LOG_DEBUG, "%s - Deleted and forget recording %s", __FUNCTION__, recording.strRecordingId);
      return PVR_ERROR_NO_ERROR;
    }
    else
    {
      XBMC->Log(LOG_ERROR, "%s - Failed to delete recording %s", __FUNCTION__, recording.strRecordingId);
    }
  }
  else
  {
    XBMC->Log(LOG_ERROR, "%s - Recording %s does not exist", __FUNCTION__, recording.strRecordingId);
  }
  return PVR_ERROR_FAILED;
}

PVR_ERROR PVRClientMythTV::SetRecordingPlayCount(const PVR_RECORDING &recording, int count)
{
  XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  if (count > 1) count = 1;
  if (count < 0) count = 0;

  CLockObject lock(m_recordingsLock);
  ProgramInfoMap::iterator it = m_recordings.find(recording.strRecordingId);
  if (it != m_recordings.end())
  {
    if (m_db.SetWatchedStatus(it->second, count > 0))
    {
      if (g_bExtraDebug)
        XBMC->Log(LOG_DEBUG, "%s - Set watched state for %s", __FUNCTION__, recording.strRecordingId);
      ForceUpdateRecording(it);
      return PVR_ERROR_NO_ERROR;
    }
    else
    {
      XBMC->Log(LOG_DEBUG, "%s - Failed setting watched state for: %s", __FUNCTION__, recording.strRecordingId);
    }
  }
  else
  {
    XBMC->Log(LOG_DEBUG, "%s - Recording %s does not exist", __FUNCTION__, recording.strRecordingId);
  }
  return PVR_ERROR_FAILED;
}

PVR_ERROR PVRClientMythTV::SetRecordingLastPlayedPosition(const PVR_RECORDING &recording, int lastplayedposition)
{
  // MythTV provides it's bookmarks as frame offsets whereas XBMC expects a time offset.
  // The frame offset is calculated by: frameOffset = bookmark * frameRate.

  if (g_bExtraDebug)
  {
    XBMC->Log(LOG_DEBUG, "%s - Setting Bookmark for: %s to %d", __FUNCTION__, recording.strTitle, lastplayedposition);
  }

  CLockObject lock(m_recordingsLock);
  ProgramInfoMap::iterator it = m_recordings.find(recording.strRecordingId);
  if (it != m_recordings.end())
  {
    // pin framerate value
    if (it->second.FrameRate() < 0)
      it->second.SetFrameRate(m_db.GetRecordingFrameRate(it->second));
    // Calculate the frame offset
    long long frameOffset = (long long)(lastplayedposition * it->second.FrameRate() / 1000.0f);
    if (frameOffset < 0) frameOffset = 0;
    if (g_bExtraDebug)
    {
      XBMC->Log(LOG_DEBUG, "%s - FrameOffset: %lld)", __FUNCTION__, frameOffset);
    }

    // Write the bookmark
    if (m_con.SetBookmark(it->second, frameOffset))
    {
      if (g_bExtraDebug)
        XBMC->Log(LOG_DEBUG, "%s - Setting Bookmark successful", __FUNCTION__);
      return PVR_ERROR_NO_ERROR;
    }
    else
    {
      if (g_bExtraDebug)
      {
        XBMC->Log(LOG_ERROR, "%s - Setting Bookmark failed", __FUNCTION__);
      }
    }
  }
  else
  {
    XBMC->Log(LOG_DEBUG, "%s - Recording %s does not exist", __FUNCTION__, recording.strRecordingId);
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
        XBMC->Log(LOG_DEBUG, "%s - FrameOffset: %lld)", __FUNCTION__, frameOffset);
      }
      // Pin framerate value
      if (programInfo.FrameRate() < 0)
        programInfo.SetFrameRate(m_db.GetRecordingFrameRate(programInfo));
      float frameRate = (float)programInfo.FrameRate() / 1000.0f;
      if (frameRate > 0)
      {
        bookmark = (int)((float)frameOffset / frameRate);
        if (g_bExtraDebug)
        {
          XBMC->Log(LOG_DEBUG, "%s - Bookmark: %d)", __FUNCTION__, bookmark);
        }
      }
    }
  }
  else
  {
    if (g_bExtraDebug)
    {
      XBMC->Log(LOG_DEBUG, "%s - Recording %s has no bookmark", __FUNCTION__, programInfo.Title().c_str());
    }
  }

  if (bookmark < 0) bookmark = 0;
  return bookmark;
}

PVR_ERROR PVRClientMythTV::GetRecordingEdl(const PVR_RECORDING &recording, PVR_EDL_ENTRY entries[], int *size)
{
  if (g_bExtraDebug)
  {
    XBMC->Log(LOG_DEBUG, "%s - Reading edl for: %s", __FUNCTION__, recording.strTitle);
  }

  CLockObject lock(m_recordingsLock);
  ProgramInfoMap::iterator it = m_recordings.find(recording.strRecordingId);
  if (it == m_recordings.end())
  {
    XBMC->Log(LOG_DEBUG, "%s - Recording %s does not exist", __FUNCTION__, recording.strRecordingId);
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
      XBMC->Log(LOG_DEBUG, "%s - Missing recordedseek map, try running 'mythcommflag --rebuild' for recording %s", __FUNCTION__, recording.strRecordingId);
      XBMC->Log(LOG_DEBUG, "%s - Fallback using framerate ...", __FUNCTION__);
    }
  }
  if (useFrameRate)
  {
    frameRate = m_db.GetRecordingFrameRate(it->second) / 1000.0f;
    if (frameRate <= 0)
    {
      XBMC->Log(LOG_DEBUG, "%s - Failed to read framerate for %s", __FUNCTION__, recording.strRecordingId);
      *size = 0;
      return PVR_ERROR_FAILED;
    }
  }

  Edl commbreakList = m_con.GetCommbreakList(it->second);
  int commbreakCount = commbreakList.size();
  XBMC->Log(LOG_DEBUG, "%s - Found %d commercial breaks for: %s", __FUNCTION__, commbreakCount, recording.strTitle);

  Edl cutList = m_con.GetCutList(it->second);
  XBMC->Log(LOG_DEBUG, "%s - Found %d cut list entries for: %s", __FUNCTION__, cutList.size(), recording.strTitle);

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
        XBMC->Log(LOG_DEBUG, "%s - start_mark: %lld, end_mark: %lld, start: %lld, end: %lld", __FUNCTION__, edlIt->start_mark, edlIt->end_mark, start, end);
      }
      else
      {
        // mask == 1 ==> Found before the mark (use psoffset, nsoffset is zero)
        // mask == 2 ==> Found after the mark (use nsoffset, psoffset is zero)
        // mask == 3 ==> Found around the mark (use nsoffset [next offset / later] for start, psoffset [prev. offset / before] for end)
        int mask = m_db.GetRecordingSeekOffset(it->second, MARK_DURATION_MS, edlIt->start_mark, &psoffset, &nsoffset);
        XBMC->Log(LOG_DEBUG, "%s - start_mark offset mask: %d, psoffset: %"PRId64", nsoffset: %"PRId64, __FUNCTION__, mask, psoffset, nsoffset);
        if (mask > 0)
        {
          if (mask == 1)
            start = psoffset;
          else if (edlIt->start_mark == 0)
            start = 0;
          else
            start = nsoffset;

          mask = m_db.GetRecordingSeekOffset(it->second, MARK_DURATION_MS, edlIt->end_mark, &psoffset, &nsoffset);
          XBMC->Log(LOG_DEBUG, "%s - end_mark offset mask: %d, psoffset: %"PRId64", nsoffset: %"PRId64, __FUNCTION__, mask, psoffset, nsoffset);
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
          XBMC->Log(LOG_DEBUG, "%s - Failed to retrieve recordedseek offset values", __FUNCTION__);
      }

      if (start < end)
      {
        // We have both a valid start and end value now
        XBMC->Log(LOG_DEBUG, "%s - start_mark: %"PRId64", end_mark: %"PRId64", start: %"PRId64", end: %"PRId64, __FUNCTION__, edlIt->start_mark, edlIt->end_mark, start, end);
        PVR_EDL_ENTRY entry;
        entry.start = start;
        entry.end = end;
        entry.type = index < commbreakCount ? PVR_EDL_TYPE_COMBREAK : PVR_EDL_TYPE_CUT;
        entries[index] = entry;
        index++;
      }
      else
        XBMC->Log(LOG_DEBUG, "%s - invalid offset: start_mark: %"PRId64", end_mark: %"PRId64", start: %"PRId64", end: %"PRId64, __FUNCTION__, edlIt->start_mark, edlIt->end_mark, start, end);
    }
    else
    {
      XBMC->Log(LOG_ERROR, "%s - Maximum number of edl entries reached for %s", __FUNCTION__, recording.strTitle);
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
    XBMC->Log(LOG_DEBUG, "%s - Reading Bookmark for: %s", __FUNCTION__, recording.strTitle);
  }

  CLockObject lock(m_recordingsLock);
  ProgramInfoMap::iterator it = m_recordings.find(recording.strRecordingId);
  if (it != m_recordings.end())
    bookmark = GetRecordingLastPlayedPosition(it->second);
  else
    XBMC->Log(LOG_ERROR, "%s - Recording %s does not exist", __FUNCTION__, recording.strRecordingId);

  return bookmark;
}

MythChannel PVRClientMythTV::FindRecordingChannel(MythProgramInfo &programInfo)
{
  ChannelIdMap::iterator channelByIdIt = m_channelsById.find(programInfo.ChannelID());
  if (channelByIdIt != m_channelsById.end())
  {
    return channelByIdIt->second;
  }
  return MythChannel();
}

bool PVRClientMythTV::IsMyLiveTVRecording(MythProgramInfo& programInfo)
{
  if (!programInfo.IsNull())
  {
    if (!m_rec.IsNull() && m_rec.IsRecording())
    {
      MythProgramInfo currentProgram = m_rec.GetCurrentProgram();
      if (currentProgram == programInfo)
        return true;
    }
  }
  return false;
}

bool PVRClientMythTV::KeepLiveTVRecording(MythProgramInfo &programInfo, bool keep)
{
  bool retval = m_db.KeepLiveTVRecording(programInfo, keep);
  if (retval)
  {
    // Force an update to get new status of the recording
    CLockObject lock(m_recordingsLock);
    ProgramInfoMap::iterator it = m_recordings.find(programInfo.UID());
    if (it != m_recordings.end())
    {
      ForceUpdateRecording(it);
      // On keep query to generate the preview.
      if (keep)
      {
        m_con.GenerateRecordingPreview(it->second);
      }
    }
    return true;
  }

  XBMC->Log(LOG_ERROR, "%s - Failed to keep live recording '%s'", __FUNCTION__, (keep ? "true" : "false"));
  return false;
}

void PVRClientMythTV::UpdateSchedules()
{
  m_scheduleManager->Update();
  PVR->TriggerTimerUpdate();
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

    CStdString rulemarker = "";
    tag.startTime = it->second->StartTime();
    tag.endTime = it->second->EndTime();
    tag.iClientChannelUid = FindPVRChannelUid(it->second->ChannelID());
    tag.iPriority = it->second->Priority();
    int genre = m_categories.Category(it->second->Category());
    tag.iGenreSubType = genre & 0x0F;
    tag.iGenreType = genre & 0xF0;

    // Fill info from recording rule if possible
    boost::shared_ptr<MythRecordingRuleNode> node = m_scheduleManager->FindRuleById(it->second->RecordID());
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
    case RS_RECORDING:
      tag.state = PVR_TIMER_STATE_RECORDING;
      break;
    case RS_TUNING:
      tag.state = PVR_TIMER_STATE_RECORDING;
      break;
    case RS_ABORTED:
      tag.state = PVR_TIMER_STATE_ABORTED;
      break;
    case RS_RECORDED:
      tag.state = PVR_TIMER_STATE_COMPLETED;
      break;
    case RS_EARLIER_RECORDING:
    case RS_WILL_RECORD:
      tag.state = PVR_TIMER_STATE_SCHEDULED;
      break;
    case RS_CONFLICT:
      tag.state = PVR_TIMER_STATE_CONFLICT_NOK;
      break;
    case RS_FAILED:
      tag.state = PVR_TIMER_STATE_ERROR;
      break;
    case RS_LOW_DISKSPACE:
      tag.state = PVR_TIMER_STATE_ERROR;
      break;
    case RS_UNKNOWN:
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
    CStdString title = it->second->Title();
    if (!rulemarker.empty())
      title.append(" ").append(rulemarker);
    PVR_STRCPY(tag.strTitle, this->MakeProgramTitle(title, it->second->Subtitle()));

    // Summary
    PVR_STRCPY(tag.strSummary, it->second->Description());

    // Unimplemented
    tag.iEpgUid = 0;
    tag.iLifetime = 0;
    PVR_STRCPY(tag.strDirectory, "");

    tag.iClientIndex = it->first;
    PVR->TransferTimerEntry(handle, &tag);
    // Add it to memorandom: cf UpdateTimer()
    boost::shared_ptr<PVR_TIMER> pTag = boost::shared_ptr<PVR_TIMER>(new PVR_TIMER(tag));
    m_PVRtimerMemorandum.insert(std::make_pair(tag.iClientIndex, pTag));
  }

  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s - Done", __FUNCTION__);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR PVRClientMythTV::AddTimer(const PVR_TIMER &timer)
{
  XBMC->Log(LOG_DEBUG, "%s - title: %s, start: %ld, end: %ld, chanID: %u", __FUNCTION__, timer.strTitle, timer.startTime, timer.endTime, timer.iClientChannelUid);
  CLockObject lock(m_lock);
  // Check if our timer is a quick recording of live tv
  // Assumptions: Timer start time = 0, and our live recorder is locked on the same channel.
  // If true then keep recording, setup recorder and let the backend handle the rule.
  if (timer.startTime == 0 && !m_rec.IsNull() && m_rec.IsRecording())
  {
    MythProgramInfo currentProgram = m_rec.GetCurrentProgram();
    if ((unsigned int)timer.iClientChannelUid == currentProgram.ChannelID())
    {
      XBMC->Log(LOG_DEBUG, "%s - Timer is a quick recording. Toggling Record on", __FUNCTION__);
      if (m_rec.IsLiveRecording())
        XBMC->Log(LOG_NOTICE, "%s - Record already on! Retrying...", __FUNCTION__);
      if (KeepLiveTVRecording(currentProgram, true) && m_rec.SetLiveRecording(true))
        return PVR_ERROR_NO_ERROR;
      else
        // Supress error notification! XBMC locks if we return an error here.
        return PVR_ERROR_NO_ERROR;
    }
  }

  // Otherwise create the rule to schedule record
  XBMC->Log(LOG_DEBUG, "%s - Creating new recording rule", __FUNCTION__);
  MythScheduleManager::MSM_ERROR ret;

  MythRecordingRule rule = PVRtoMythRecordingRule(timer);
  ret = m_scheduleManager->ScheduleRecording(rule);
  if (ret == MythScheduleManager::MSM_ERROR_FAILED)
    return PVR_ERROR_FAILED;
  if (ret == MythScheduleManager::MSM_ERROR_NOT_IMPLEMENTED)
    return PVR_ERROR_REJECTED;

  XBMC->Log(LOG_DEBUG, "%s - Done - %d", __FUNCTION__, rule.RecordID());

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
    if (!m_rec.IsNull() && m_rec.IsLiveRecording())
    {
      boost::shared_ptr<MythProgramInfo> recording = m_scheduleManager->FindUpComingByIndex(timer.iClientIndex);
      if (recording && this->IsMyLiveTVRecording(*recording))
      {
        XBMC->Log(LOG_DEBUG, "%s - Timer %i is a quick recording. Toggling Record off", __FUNCTION__, timer.iClientIndex);
        if (KeepLiveTVRecording(*recording, false) && m_rec.SetLiveRecording(false))
          return PVR_ERROR_NO_ERROR;
        else
          return PVR_ERROR_FAILED;
      }
    }
  }

  // Otherwise delete scheduled rule
  XBMC->Log(LOG_DEBUG, "%s - Deleting timer %i", __FUNCTION__, timer.iClientIndex);
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
  CStdString title = timer.strTitle;
  CStdString cs;

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
      if (m_db.FindProgram(st, timer.iClientChannelUid, "%", epgInfo) && title.compare(0, epgInfo.Title().length(), epgInfo.Title()) == 0)
        epgFound = true;
      else
        epgInfo = MythEPGInfo();
      rule = m_scheduleManager->NewWeeklyRecord(epgInfo);
    }
    else if (timer.iWeekdays == 0x7F)
    {
      // Create a DAILY record rule
      if (m_db.FindProgram(st, timer.iClientChannelUid, "%", epgInfo) && title.compare(0, epgInfo.Title().length(), epgInfo.Title()) == 0)
        epgFound = true;
      else
        epgInfo = MythEPGInfo();
      rule = m_scheduleManager->NewDailyRecord(epgInfo);
    }
  }
  else
  {
    // Find the program info at the given start time with the same title
    // When no entry was found with the same title, then the record rule type is manual
    if (m_db.FindProgram(st, timer.iClientChannelUid, "%", epgInfo) && title.compare(0, epgInfo.Title().length(), epgInfo.Title()) == 0)
      epgFound = true;
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
    XBMC->Log(LOG_DEBUG,"%s - Found program: %u %lu %s", __FUNCTION__, epgInfo.ChannelID(), epgInfo.StartTime(), epgInfo.Title().c_str());
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
  XBMC->Log(LOG_DEBUG, "%s - title: %s, start: %ld, end: %ld, chanID: %u", __FUNCTION__, timer.strTitle, timer.startTime, timer.endTime, timer.iClientChannelUid);

  MythScheduleManager::MSM_ERROR ret = MythScheduleManager::MSM_ERROR_NOT_IMPLEMENTED;
  unsigned char diffmask = 0;

  enum
  {
    CTState = 0x01, // State has changed
    CTEnabled = 0x02, // The new state
    CTTimer = 0x04 // Timer has changed
  };

  // Get the extent of changes for original timer
  std::map<unsigned int, boost::shared_ptr<PVR_TIMER> >::const_iterator old = m_PVRtimerMemorandum.find(timer.iClientIndex);
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

  XBMC->Log(LOG_DEBUG,"%s - Done", __FUNCTION__);
  return PVR_ERROR_NO_ERROR;
}

bool PVRClientMythTV::OpenLiveStream(const PVR_CHANNEL &channel)
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG,"%s - chanID: %u, channumber: %u", __FUNCTION__, channel.iUniqueId, channel.iChannelNumber);

  // Check event connection
  if (!m_pEventHandler->IsListening())
  {
    XBMC->QueueNotification(QUEUE_ERROR, XBMC->GetLocalizedString(30302)); // MythTV backend unavailable
    return false;
  }

  CLockObject lock(m_lock);
  if (m_rec.IsNull())
  {
    LoadChannelsAndChannelGroups();

    // Suspend fileOps to avoid connection hang
    if (m_fileOps->IsRunning())
      m_fileOps->Suspend();

    // Enable playback mode: Keep quiet on connection
    m_pEventHandler->EnablePlayback();

    // First we have to get the channum of the selected channel
    // Due to the merged view (same channum+callsign) this might not yet be the preferred channel on the preferred source to switch to
    ChannelIdMap::iterator channelByIdIt = m_channelsById.find(channel.iUniqueId);
    if (channelByIdIt == m_channelsById.end())
    {
      XBMC->Log(LOG_ERROR,"%s - Channel not found", __FUNCTION__);
      return false;
    }

    // Retreive a list of recorders and sources for the given channum (ordered by LiveTV priority)
    RecorderSourceList recorderSourceList = m_db.GetLiveTVRecorderSourceList(channelByIdIt->second.Number());
    for (RecorderSourceList::iterator recorderSourceIt = recorderSourceList.begin(); recorderSourceIt != recorderSourceList.end(); ++recorderSourceIt)
    {
      // Get the recorder from the recorder source list and check if it's available
      m_rec = m_con.GetRecorder(recorderSourceIt->first);
      if (m_rec.IsNull() || m_rec.ID() <= 0)
      {
        XBMC->Log(LOG_ERROR,"%s - Recorder not found: %u", __FUNCTION__, recorderSourceIt->first);
        continue;
      }
      if (m_rec.IsRecording())
      {
        XBMC->Log(LOG_ERROR,"%s - Recorder is busy: %u", __FUNCTION__, recorderSourceIt->first);
        continue;
      }

      // Get the channel that matches the channum and the source id
      std::pair<ChannelNumberMap::iterator, ChannelNumberMap::iterator> channelsByNumber = m_channelsByNumber.equal_range(channelByIdIt->second.Number());
      for (ChannelNumberMap::iterator channelByNumberIt = channelsByNumber.first; channelByNumberIt != channelsByNumber.second; ++channelByNumberIt)
      {
        if ((*channelByNumberIt).second.SourceID() == recorderSourceIt->second)
        {
          // Check if the recorder is able to tune to that channel (virtual recorders might be locked to a multiplex ID)
          if (m_rec.IsTunable((*channelByNumberIt).second))
          {
            XBMC->Log(LOG_DEBUG,"%s: Opening new recorder %u", __FUNCTION__, m_rec.ID());

            m_pEventHandler->SetRecorder(m_rec);

            if (m_rec.SpawnLiveTV((*channelByNumberIt).second))
            {
              if(g_bDemuxing)
                m_demux = new Demux(m_rec);
              XBMC->Log(LOG_DEBUG, "%s - Done", __FUNCTION__);
              return true;
            }
          }
        }
      }
    }

    // Failed to open live stream (channel unavailable)
    m_rec = MythRecorder();
    m_pEventHandler->SetRecorder(m_rec);

    // Disable playback mode: Allow all
    m_pEventHandler->DisablePlayback();

    // Resume fileOps
    m_fileOps->Resume();

    XBMC->Log(LOG_ERROR,"%s - Failed to open live stream", __FUNCTION__);
    XBMC->QueueNotification(QUEUE_WARNING, XBMC->GetLocalizedString(30305)); // Channel unavailable
  }
  else
  {
    XBMC->Log(LOG_ERROR, "%s - Live stream is already opened. recorder: %lu", __FUNCTION__, m_rec.ID());
  }
  return false;
}

void PVRClientMythTV::CloseLiveStream()
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  CLockObject lock(m_lock);

  m_pEventHandler->PreventLiveChainUpdate();

  if (!m_rec.Stop())
    XBMC->Log(LOG_NOTICE, "%s - Stop live stream failed", __FUNCTION__);
  m_rec = MythRecorder();
  m_pEventHandler->SetRecordingListener("", m_file);
  m_pEventHandler->SetRecorder(m_rec);
  m_pEventHandler->DisablePlayback();
  m_pEventHandler->AllowLiveChainUpdate();

  if (m_demux)
  {
    delete m_demux;
    m_demux = NULL;
  }
  // Resume fileOps
  m_fileOps->Resume();

  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s - Done", __FUNCTION__);

  return;
}

int PVRClientMythTV::ReadLiveStream(unsigned char *pBuffer, unsigned int iBufferSize)
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s - size: %u", __FUNCTION__, iBufferSize);

  if (m_rec.IsNull())
    return -1;

  int dataread = m_rec.ReadLiveTV(pBuffer, iBufferSize);
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s: Read %d Bytes", __FUNCTION__, dataread);
  else if (dataread==0)
    XBMC->Log(LOG_INFO, "%s: Read 0 Bytes!", __FUNCTION__);
  return dataread;
}

int PVRClientMythTV::GetCurrentClientChannel()
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  if (m_rec.IsNull())
    return -1;

  MythProgramInfo currentProgram = m_rec.GetCurrentProgram();
  return currentProgram.ChannelID();
}

bool PVRClientMythTV::SwitchChannel(const PVR_CHANNEL &channelinfo)
{
  bool retval;

  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s - chanID: %u", __FUNCTION__, channelinfo.iUniqueId);

  if (m_rec.IsNull())
  {
    XBMC->Log(LOG_ERROR, "%s: No recorder", __FUNCTION__);
    return false;
  }

  // Keep playback mode enabled
  m_pEventHandler->PreventLiveChainUpdate();
  retval = m_rec.Stop();
  m_rec = MythRecorder();
  m_pEventHandler->SetRecordingListener("", m_file);
  m_pEventHandler->SetRecorder(m_rec);
  m_pEventHandler->AllowLiveChainUpdate();
  if (m_demux)
  {
    delete m_demux;
    m_demux = NULL;
  }
  // Try to reopen live stream
  if (retval)
    retval = OpenLiveStream(channelinfo);
  if (!retval)
    XBMC->Log(LOG_ERROR, "%s - Failed to reopening Livestream", __FUNCTION__);

  return retval;
}

long long PVRClientMythTV::SeekLiveStream(long long iPosition, int iWhence)
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s - pos: %lld, whence: %d", __FUNCTION__, iPosition, iWhence);

  if (m_rec.IsNull())
    return -1;

  int whence;
  if (iWhence == SEEK_SET)
    whence = WHENCE_SET;
  else if (iWhence == SEEK_CUR)
    whence = WHENCE_CUR;
  else
    whence = WHENCE_END;

  long long retval = m_rec.LiveTVSeek(iPosition, whence);

  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s - Done - pos: %lld", __FUNCTION__, retval);

  return retval;
}

long long PVRClientMythTV::LengthLiveStream()
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  if (m_rec.IsNull())
    return -1;

  long long retval = m_rec.LiveTVDuration();

  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s - Done - duration: %lld", __FUNCTION__, retval);

  return retval;
}

PVR_ERROR PVRClientMythTV::SignalStatus(PVR_SIGNAL_STATUS &signalStatus)
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  // Check event connection
  if (!m_pEventHandler->IsListening())
    return PVR_ERROR_SERVER_ERROR;

  MythSignal signal;
  signal = m_pEventHandler->GetSignal();

  signalStatus.dAudioBitrate = 0;
  signalStatus.dDolbyBitrate = 0;
  signalStatus.dVideoBitrate = 0;
  signalStatus.iBER = signal.BER();
  signalStatus.iSignal = signal.Signal();
  signalStatus.iSNR = signal.SNR();
  signalStatus.iUNC = signal.UNC();

  CStdString ID;
  ID.Format("Myth Recorder %u", signal.ID());

  CStdString strAdapterStatus = signal.AdapterStatus();
  PVR_STRCPY(signalStatus.strAdapterName, ID);
  PVR_STRCPY(signalStatus.strAdapterStatus, strAdapterStatus);

  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s - Done", __FUNCTION__);

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
  if (m_rec.IsNull() || !m_demux)
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
  if (m_rec.IsNull() || m_rec.GetLiveTVChainLast() < 0)
    return 0;
  MythProgramInfo prog = m_rec.GetLiveTVChainProgram(0);
  return prog.RecordingStartTime();
}

time_t PVRClientMythTV::GetBufferTimeEnd()
{
  CLockObject lock(m_lock);
  int last;
  if (m_rec.IsNull() || (last = m_rec.GetLiveTVChainLast()) < 0)
    return 0;
  time_t now = time(NULL);
  MythProgramInfo prog = m_rec.GetLiveTVChainProgram(last);
  return (now > prog.RecordingEndTime() ? prog.RecordingEndTime() : now);
}

bool PVRClientMythTV::OpenRecordedStream(const PVR_RECORDING &recording)
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s - title: %s, ID: %s, duration: %d", __FUNCTION__, recording.strTitle, recording.strRecordingId, recording.iDuration);

  // Check event connection
  if (!m_pEventHandler->IsListening())
  {
    XBMC->QueueNotification(QUEUE_ERROR, XBMC->GetLocalizedString(30302)); // MythTV backend unavailable
    return false;
  }

  CLockObject lock(m_recordingsLock);
  ProgramInfoMap::iterator it = m_recordings.find(recording.strRecordingId);
  if (it != m_recordings.end())
  {
    // Suspend fileOps to avoid connection hang
    m_fileOps->Suspend();

    // Enable playback mode: Keep quiet on connection
    m_pEventHandler->EnablePlayback();

    // Currently we only request the stream from the master backend.
    // Future implementations could request the stream from slaves if not available on the master.

    // Create dedicated control connection for file playback; smart pointer deletes it when file gets deleted.
    MythConnection fileControlConnection(g_szMythHostname, g_iMythPort, true);
    if (!fileControlConnection.IsNull())
      m_file = fileControlConnection.ConnectFile(it->second);

    m_pEventHandler->SetRecordingListener(recording.strRecordingId, m_file);

    // Resume fileOps
    if (m_file.IsNull())
    {
      m_fileOps->Resume();

      // Disable playback mode: Allow all
      m_pEventHandler->DisablePlayback();
    }

    if (g_bExtraDebug)
      XBMC->Log(LOG_DEBUG, "%s - Done - %s", __FUNCTION__, (m_file.IsNull() ? "false" : "true"));

    return !m_file.IsNull();
  }
  XBMC->Log(LOG_DEBUG, "%s - Recording %s does not exist", __FUNCTION__, recording.strRecordingId);
  return false;
}

void PVRClientMythTV::CloseRecordedStream()
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  m_file = MythFile();
  m_pEventHandler->SetRecordingListener("", m_file);

  m_pEventHandler->DisablePlayback();

  // Resume fileOps
  m_fileOps->Resume();

  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s - Done", __FUNCTION__);
}

int PVRClientMythTV::ReadRecordedStream(unsigned char *pBuffer, unsigned int iBufferSize)
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s - size: %u curPos: %lld", __FUNCTION__, iBufferSize, (long long)m_file.Position());

  int dataread = m_file.Read(pBuffer, iBufferSize);
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s: Read %d Bytes", __FUNCTION__, dataread);
  else if (dataread == 0)
    XBMC->Log(LOG_INFO, "%s: Read 0 Bytes!", __FUNCTION__);
  return dataread;
}

long long PVRClientMythTV::SeekRecordedStream(long long iPosition, int iWhence)
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s - pos: %lld, whence: %d", __FUNCTION__, iPosition, iWhence);

  int whence;
  if (iWhence == SEEK_SET)
    whence = WHENCE_SET;
  else if (iWhence == SEEK_CUR)
    whence = WHENCE_CUR;
  else
    whence = WHENCE_END;

  long long retval = m_file.Seek(iPosition, whence);

  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s - Done - pos: %lld", __FUNCTION__, retval);

  return retval;
}

long long PVRClientMythTV::LengthRecordedStream()
{
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s", __FUNCTION__);

  long long retval = m_file.Length();
  if (g_bExtraDebug)
    XBMC->Log(LOG_DEBUG, "%s - Done - duration: %lld", __FUNCTION__, retval);
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
      XBMC->Log(LOG_ERROR,"%s - Recording not found", __FUNCTION__);
      return PVR_ERROR_INVALID_PARAMETERS;
    }

    if (!it->second.IsLiveTV())
      return PVR_ERROR_NO_ERROR;

    // If recording is current live show then keep it and set live recorder
    if (IsMyLiveTVRecording(it->second))
    {
      if (KeepLiveTVRecording(it->second, true) && m_rec.SetLiveRecording(true))
        return PVR_ERROR_NO_ERROR;
      return PVR_ERROR_FAILED;
    }
    // Else keep old live recording
    else
    {
      if (KeepLiveTVRecording(it->second, true))
      {
        CStdString info = XBMC->GetLocalizedString(menuhook.iLocalizedStringId);
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
      UpdateSchedules();
      CStdString info = (flag ? XBMC->GetLocalizedString(30310) : XBMC->GetLocalizedString(30311));
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
    if (m_db.FindCurrentProgram(attime, chanid, epgInfo))
    {
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
            rule.SetFilter(rule.Filter() | MythRecordingRule::FM_FirstShowing);
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
      XBMC->Log(LOG_DEBUG, "%s - broadcast: %d chanid: %u attime: %lu", __FUNCTION__, item.data.iEpgUid, chanid, attime);
      return PVR_ERROR_INVALID_PARAMETERS;
    }
    return PVR_ERROR_FAILED;
  }

  return PVR_ERROR_NOT_IMPLEMENTED;
}

bool PVRClientMythTV::GetLiveTVPriority()
{
  if (!m_con.IsNull())
  {
    CStdString value = m_con.GetSettingOnHost("LiveTVPriority", m_con.GetHostname());
    if (value.compare("1") == 0)
      return true;
    else
      return false;
  }
  return false;
}

void PVRClientMythTV::SetLiveTVPriority(bool enabled)
{
  if (!m_con.IsNull())
  {
    CStdString value = enabled ? "1" : "0";
    m_con.SetSetting(m_con.GetHostname(), "LiveTVPriority", value);
  }
}

CStdString PVRClientMythTV::MakeProgramTitle(const CStdString &title, const CStdString &subtitle) const
{
  CStdString epgtitle;
  if (subtitle.IsEmpty())
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

void PVRClientMythTV::BreakBroadcastID(int broadcastid, unsigned int* chanid, time_t* attime) const
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
