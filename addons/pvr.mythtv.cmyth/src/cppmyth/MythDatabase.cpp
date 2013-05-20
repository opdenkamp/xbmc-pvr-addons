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
#include "MythRecordingRule.h"
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

MythDatabase::MythDatabase(const CStdString &server, const CStdString &database, const CStdString &user, const CStdString &password, unsigned short port)
  : m_database_t(new MythPointerThreadSafe<cmyth_database_t>())
{
  *m_database_t = cmyth_database_init(const_cast<char*>(server.c_str()), const_cast<char*>(database.c_str()), const_cast<char*>(user.c_str()), const_cast<char*>(password.c_str()), port);
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

int MythDatabase::GetSchemaVersion()
{
  int retval = 0;
  CMYTH_DB_CALL(retval, retval < 0, cmyth_database_get_version(*m_database_t));
  return retval;
}

CStdString MythDatabase::GetSetting(const CStdString &setting)
{
  int retval;
  char* buf = NULL;
  CMYTH_DB_CALL(retval, retval < 0, cmyth_mysql_get_setting(*m_database_t, const_cast<char*>(setting.c_str()), &buf));
  CStdString data(buf);
  ref_release(buf);
  return data;
}

bool MythDatabase::FindProgram(time_t starttime, int channelid, const CStdString &title, MythProgram* program)
{
  int retval = 0;
  CMYTH_DB_CALL(retval, retval < 0, cmyth_mysql_get_prog_finder_time_title_chan(*m_database_t, program, starttime, const_cast<char*>(title.c_str()), channelid));
  return retval > 0;
}

bool MythDatabase::FindCurrentProgram(int channelid, MythProgram* program)
{
  int retval = 0;
  CMYTH_DB_CALL(retval, retval < 0, cmyth_mysql_get_prog_finder_chan(*m_database_t, program, channelid));
  return retval > 0;
}

ProgramList MythDatabase::GetGuide(int channelid, time_t starttime, time_t endtime)
{
  MythProgram *programs = 0;
  int len = 0;
  CMYTH_DB_CALL(len, len < 0, cmyth_mysql_get_guide(*m_database_t, &programs, channelid, starttime, endtime));

  if (len < 1)
    return ProgramList();

  ProgramList retval(programs, programs + len);
  ref_release(programs);
  return retval;
}

ChannelIdMap MythDatabase::GetChannels()
{
  int ret;
  ChannelIdMap retval;
  cmyth_chanlist_t channels = NULL;
  CMYTH_DB_CALL(ret, ret < 0, cmyth_mysql_get_chanlist(*m_database_t, &channels));
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
  cmyth_channelgroup_t *channelGroups = NULL;
  int channelGroupCount = 0;
  CMYTH_DB_CALL(channelGroupCount, channelGroupCount < 0, cmyth_mysql_get_channelgroups(*m_database_t, &channelGroups));

  if (!channelGroups)
    return retval;

  for (int i = 0; i < channelGroupCount; i++)
  {
    // cmyth_mysql_get_channelids_in_group writes the list of all channel IDs to its third parameter (int**)
    unsigned int *channelIDs = 0;
    int channelCount = cmyth_mysql_get_channelids_in_group(*m_database_t, channelGroups[i].grpid, (uint32_t**)&channelIDs);
    if (channelCount > 0)
    {
      retval.insert(std::make_pair(channelGroups[i].name, std::vector<int>(channelIDs, channelIDs + channelCount)));
      ref_release(channelIDs);
    }
    else
    {
      retval.insert(std::make_pair(channelGroups[i].name, std::vector<int>()));
    }
  }
  ref_release(channelGroups);
  return retval;
}

RecorderSourceList MythDatabase::GetLiveTVRecorderSourceList(const CStdString &channum)
{
  RecorderSourceList retval;
  cmyth_recorder_source_t *recorders = 0;
  int recorderCount = 0;
  CMYTH_DB_CALL(recorderCount, recorderCount < 0, cmyth_mysql_get_recorder_source_channum(*m_database_t, const_cast<char*>(channum.c_str()), &recorders));

  for (int i = 0; i < recorderCount; ++i)
  {
    retval.push_back(std::make_pair(recorders[i].recid, recorders[i].sourceid));
  }
  ref_release(recorders);
  return retval;
}

RecordingRuleMap MythDatabase::GetRecordingRules()
{
  int ret;
  RecordingRuleMap retval;
  cmyth_recordingrulelist_t rules = NULL;
  CMYTH_DB_CALL(ret, ret < 0, cmyth_mysql_get_recordingrules(*m_database_t, &rules));
  int ruleCount = cmyth_recordingrulelist_get_count(rules);

  for (int i = 0; i < ruleCount; i++)
  {
    cmyth_recordingrule_t rule = cmyth_recordingrulelist_get_item(rules, i);

    MythRecordingRule t(rule);
    retval.insert(std::pair<int,MythRecordingRule>(t.RecordID(), t));
  }
  ref_release(rules);
  return retval;
}

bool MythDatabase::AddRecordingRule(const MythRecordingRule &rule)
{
  int retval = 0;
  CMYTH_DB_CALL(retval, retval < 0, cmyth_mysql_add_recordingrule(*m_database_t, *rule.m_recordingrule_t));
  return retval > 0;
}

bool MythDatabase::UpdateRecordingRule(const MythRecordingRule &rule)
{
  int retval = 0;
  CMYTH_DB_CALL(retval, retval < 0, cmyth_mysql_update_recordingrule(*m_database_t, *rule.m_recordingrule_t));
  return retval > 0;
}

bool MythDatabase::DeleteRecordingRule(unsigned int recordid)
{
  int retval = 0;
  CMYTH_DB_CALL(retval, retval < 0, cmyth_mysql_delete_recordingrule(*m_database_t, recordid));
  return retval > 0;
}

MythRecordingRule MythDatabase::LoadRecordingRuleTemplate(const CStdString &category, const CStdString &category_type)
{
  int ret;
  MythRecordingRule retval;
  cmyth_recordingrule_t rule = NULL;
  CMYTH_DB_CALL(ret, ret < 0, cmyth_mysql_recordingrule_from_template(*m_database_t, const_cast<char*>(category.c_str()), const_cast<char*>(category_type.c_str()), &rule));
  if (ret >= 0)
    retval = MythRecordingRule(rule);
  return retval;
}

RecordingProfileList MythDatabase::GetRecordingProfiles()
{
  RecordingProfileList retval;
  cmyth_recprofile* recordingProfiles;
  int recordingProfileCount = 0;

  CMYTH_DB_CALL(recordingProfileCount, recordingProfileCount < 0, cmyth_mysql_get_recprofiles(*m_database_t, &recordingProfiles));

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
  return retval;
}

bool MythDatabase::SetWatchedStatus(const MythProgramInfo &recording, bool watched)
{
  int retval = 0;
  CMYTH_DB_CALL(retval, retval < 0, cmyth_mysql_set_watched_status(*m_database_t, *recording.m_proginfo_t, watched ? 1 : 0));
  return retval > 0;
}

long long MythDatabase::GetBookmarkMark(const MythProgramInfo &recording, long long bk, int mode)
{
  long long mark = 0;
  CMYTH_DB_CALL(mark, mark < 0, cmyth_mysql_get_bookmark_mark(*m_database_t, *recording.m_proginfo_t, bk, mode));
  return mark;
}

long long MythDatabase::GetRecordingMarkup(const MythProgramInfo &recording, int type)
{
  long long value = 0;
  CMYTH_DB_CALL(value, value < 0, cmyth_mysql_get_recording_markup(*m_database_t, *recording.m_proginfo_t, (cmyth_recording_markup_t)type));
  return value;
}

long long MythDatabase::GetRecordingFrameRate(const MythProgramInfo &recording)
{
  long long value = 0;
  CMYTH_DB_CALL(value, value < 0, cmyth_mysql_get_recording_framerate(*m_database_t, *recording.m_proginfo_t));
  return value;
}

bool MythDatabase::FillRecordingArtwork(MythProgramInfo &recording)
{
  int retval = 0;
  char* coverart;
  char* fanart;
  char* banner;
  CMYTH_DB_CALL(retval, retval < 0, cmyth_mysql_get_recording_artwork(*m_database_t, *recording.m_proginfo_t, &coverart, &fanart, &banner));
  if (retval > 0)
  {
    recording.m_coverart = coverart;
    recording.m_fanart = fanart;
    ref_release(coverart);
    ref_release(fanart);
    ref_release(banner);
    return true;
  }
  return false;
}