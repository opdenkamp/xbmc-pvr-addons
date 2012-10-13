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

#include "MythDatabase.h"
#include "MythChannel.h"
#include "MythProgramInfo.h"
#include "MythTimer.h"
#include "MythPointer.h"

#include "../client.h"

using namespace ADDON;

/* Call 'call', then if 'cond' condition is true disconnect from
 * the database and call 'call' again. It should automatically
 * try to connect again. */
#define CMYTH_DB_CALL(var, cond, call) m_database_t->Lock(); \
                                       var = call; \
                                       m_database_t->Unlock(); \
                                       if (cond) \
                                       { \
                                         m_database_t->Lock(); \
                                         cmyth_database_close(*m_database_t); \
                                         var = call; \
                                         m_database_t->Unlock(); \
                                       } \

MythDatabase::MythDatabase()
  : m_database_t()
{
}

MythDatabase::MythDatabase(const CStdString &server, const CStdString &database, const CStdString &user, const CStdString &password)
  : m_database_t(new MythPointerThreadSafe<cmyth_database_t>())
{
  *m_database_t = cmyth_database_init(const_cast<char*>(server.c_str()), const_cast<char*>(database.c_str()), const_cast<char*>(user.c_str()), const_cast<char*>(password.c_str()));
}

bool MythDatabase::IsNull() const
{
  if (m_database_t == NULL)
    return true;
  return *m_database_t == NULL;
}

bool MythDatabase::TestConnection(CStdString *msg)
{
  char* cmyth_msg;
  int retval = 0;
  CMYTH_DB_CALL(retval, retval < 0, cmyth_mysql_testdb_connection(*m_database_t, &cmyth_msg));
  *msg = cmyth_msg;
  ref_release(cmyth_msg);
  return retval == 1;
}

bool MythDatabase::FindProgram(time_t starttime, int channelid, const CStdString &title, MythProgram* program)
{
  int retval = 0;
  CMYTH_DB_CALL(retval, retval < 0, cmyth_mysql_get_prog_finder_time_title_chan(*m_database_t, program, const_cast<char*>(title.c_str()), starttime, channelid));
  return retval > 0;
}

ProgramList MythDatabase::GetGuide(time_t starttime, time_t endtime)
{
  MythProgram *programs = 0;
  int len = 0;
  CMYTH_DB_CALL(len, len < 0, cmyth_mysql_get_guide(*m_database_t, &programs, starttime, endtime));

  if (len < 1)
    return ProgramList();

  ProgramList retval(programs, programs + len);
  ref_release(programs);
  return retval;
}

ChannelMap MythDatabase::GetChannels()
{
  ChannelMap retval;
  cmyth_chanlist_t channels = NULL;
  CMYTH_DB_CALL(channels, channels == NULL, cmyth_mysql_get_chanlist(*m_database_t));
  int channelCount = cmyth_chanlist_get_count(channels);

  for (int i = 0; i < channelCount; i++)
  {
    cmyth_channel_t channel = cmyth_chanlist_get_item(channels, i);
    int channelID = cmyth_channel_chanid(channel);
    bool isRadio = cmyth_mysql_is_radio(*m_database_t, channelID) == 1;
    retval.insert(std::pair<int, MythChannel>(channelID, MythChannel(channel, isRadio)));
  }
  ref_release(channels);
  return retval;
}

ChannelGroupMap MythDatabase::GetChannelGroups()
{
  ChannelGroupMap retval;
  cmyth_channelgroups_t *channelGroups = NULL;
  int channelGroupCount = 0;
  CMYTH_DB_CALL(channelGroupCount, channelGroupCount < 0, cmyth_mysql_get_channelgroups(*m_database_t, &channelGroups));

  if (!channelGroups)
    return retval;

  m_database_t->Lock();

  for (int i = 0; i < channelGroupCount; i++)
  {
    MythChannelGroup channelGroup;
    channelGroup.first = channelGroups[i].channelgroup;

    // cmyth_mysql_get_channelids_in_group writes the list of all channel IDs to its third parameter (int**)
    int* channelIDs = 0;
    int channelCount = cmyth_mysql_get_channelids_in_group(*m_database_t, channelGroups[i].ID, &channelIDs);
    if (channelCount)
    {
      channelGroup.second = std::vector<int>(channelIDs, channelIDs + channelCount);
      ref_release(channelIDs);
    }
    else
      channelGroup.second = std::vector<int>();

    retval.insert(channelGroup);
  }
  ref_release(channelGroups);

  m_database_t->Unlock();

  return retval;
}

SourceMap MythDatabase::GetSources()
{
  SourceMap retval;
  cmyth_rec_t *recorders = 0;
  int recorderCount = 0;
  CMYTH_DB_CALL(recorderCount, recorderCount < 0, cmyth_mysql_get_recorder_list(*m_database_t, &recorders));

  m_database_t->Lock();

  for (int i = 0; i < recorderCount; i++)
  {
    SourceMap::iterator it = retval.find(recorders[i].sourceid);
    if (it != retval.end())
      it->second.push_back(recorders[i].recid);
    else
      retval[recorders[i].sourceid] = std::vector<int>(1, recorders[i].recid);
  }
  ref_release(recorders);

  m_database_t->Unlock();

  return retval;
}

TimerMap MythDatabase::GetTimers()
{
  TimerMap retval;
  cmyth_timerlist_t timers = NULL;
  CMYTH_DB_CALL(timers, timers == NULL, cmyth_mysql_get_timers(*m_database_t));
  int timerCount = cmyth_timerlist_get_count(timers);

  for (int i = 0; i < timerCount; i++)
  {
    cmyth_timer_t timer = cmyth_timerlist_get_item(timers, i);

    MythTimer t(timer);
    retval.insert(std::pair<int,MythTimer>(t.RecordID(), t));
  }
  ref_release(timers);
  return retval;
}

int MythDatabase::AddTimer(const MythTimer &timer)
{
  int retval = 0;
  CMYTH_DB_CALL(retval, retval < 0,
    cmyth_mysql_add_timer(*m_database_t,
      timer.ChannelID(),
      const_cast<char*>(timer.Callsign().c_str()),
      const_cast<char*>(timer.Description().c_str()),
      timer.StartTime(),
      timer.EndTime(),
      const_cast<char*>(timer.Title(false).c_str()),
      const_cast<char*>(timer.Category().c_str()),
      timer.Type(),
      const_cast<char*>(timer.Subtitle().c_str()),
      timer.Priority(),
      timer.StartOffset(),
      timer.EndOffset(),
      timer.SearchType(),
      timer.Inactive() ? 1 : 0,
      timer.DuplicateControlMethod(),
      timer.CheckDuplicatesInType(),
      const_cast<char*>(timer.RecordingGroup().c_str()),
      const_cast<char*>(timer.StorageGroup().c_str()),
      const_cast<char*>(timer.PlaybackGroup().c_str()),
      timer.AutoTranscode(),
      timer.UserJobs(),
      timer.AutoCommFlag(),
      timer.AutoExpire(),
      timer.MaxEpisodes(),
      timer.NewExpiresOldRecord(),
      timer.Transcoder()));
  return retval;
}

bool MythDatabase::UpdateTimer(const MythTimer &timer)
{
  int retval = 0;
  CMYTH_DB_CALL(retval, retval < 0,
    cmyth_mysql_update_timer(*m_database_t,
      timer.RecordID(),
      timer.ChannelID(),
      const_cast<char*>(timer.m_callsign.c_str()),
      const_cast<char*>(timer.m_description.c_str()),
      timer.StartTime(),
      timer.EndTime(),
      const_cast<char*>(timer.Title(false).c_str()),
      const_cast<char*>(timer.Category().c_str()),
      timer.Type(),
      const_cast<char*>(timer.Subtitle().c_str()),
      timer.Priority(),
      timer.StartOffset(),
      timer.EndOffset(),
      timer.SearchType(),
      timer.Inactive() ? 1 : 0,
      timer.DuplicateControlMethod(),
      timer.CheckDuplicatesInType(),
      const_cast<char*>(timer.RecordingGroup().c_str()),
      const_cast<char*>(timer.StorageGroup().c_str()),
      const_cast<char*>(timer.PlaybackGroup().c_str()),
      timer.AutoTranscode(),
      timer.UserJobs(),
      timer.AutoCommFlag(),
      timer.AutoExpire(),
      timer.MaxEpisodes(),
      timer.NewExpiresOldRecord(),
      timer.Transcoder()));
  return retval == 0;
}

bool MythDatabase::DeleteTimer(int recordid)
{
  int retval = 0;
  CMYTH_DB_CALL(retval, retval < 0, cmyth_mysql_delete_timer(*m_database_t, recordid));
  return retval == 0;
}

RecordingProfileList MythDatabase::GetRecordingProfiles()
{
  RecordingProfileList retval;
  cmyth_recprofile* recordingProfiles;
  int recordingProfileCount = 0;

  CMYTH_DB_CALL(recordingProfileCount, recordingProfileCount < 0, cmyth_mysql_get_recprofiles(*m_database_t, &recordingProfiles));

  m_database_t->Lock();
  for (int i = 0; i < recordingProfileCount; i++)
  {
    RecordingProfileList::iterator it = std::find(retval.begin(), retval.end(), CStdString(recordingProfiles[i].cardtype));
    if (it == retval.end())
    {
      retval.push_back(MythRecordingProfile());
      it = --retval.end();
      it->Format("%s", recordingProfiles[i].cardtype);
    }
    it->profile.insert(std::pair<int, CStdString>(recordingProfiles[i].id, recordingProfiles[i].name));
  }
  ref_release(recordingProfiles);

  m_database_t->Unlock();

  return retval;
}

int MythDatabase::SetWatchedStatus(const MythProgramInfo &recording, bool watched)
{
  int retval = 0;
  CMYTH_DB_CALL(retval, retval < 0, cmyth_set_watched_status_mysql(*m_database_t, *recording.m_proginfo_t, watched ? 1 : 0));
  return retval;
}

long long MythDatabase::GetBookmarkMark(const MythProgramInfo &recording, long long bk, int mode)
{
  long long mark = 0;
  CMYTH_DB_CALL(mark, mark < 0, cmyth_get_bookmark_mark(*m_database_t, *recording.m_proginfo_t, bk, mode));
  return mark;
}
