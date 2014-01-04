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

#include "MythRecordingRule.h"
#include "MythPointer.h"

MythRecordingRule::MythRecordingRule()
  : m_recordingrule_t(new MythPointer<cmyth_recordingrule_t>())
{
  *m_recordingrule_t = cmyth_recordingrule_init();
}

MythRecordingRule::MythRecordingRule(cmyth_recordingrule_t cmyth_recordingrule)
  : m_recordingrule_t(new MythPointer<cmyth_recordingrule_t>())
{
  *m_recordingrule_t = cmyth_recordingrule;
}

bool MythRecordingRule::IsNull() const
{
  if (m_recordingrule_t == NULL)
    return true;
  return *m_recordingrule_t == NULL;
}

MythRecordingRule MythRecordingRule::DuplicateRecordingRule() const
{
  return MythRecordingRule(cmyth_recordingrule_dup(*m_recordingrule_t));
}

unsigned int MythRecordingRule::RecordID() const
{
  return cmyth_recordingrule_recordid(*m_recordingrule_t);
}

void MythRecordingRule::SetRecordID(unsigned int recordid)
{
  cmyth_recordingrule_set_recordid(*m_recordingrule_t, recordid);
}

unsigned int MythRecordingRule::ChannelID() const
{
  return cmyth_recordingrule_chanid(*m_recordingrule_t);
}

void MythRecordingRule::SetChannelID(unsigned int chanid)
{
  cmyth_recordingrule_set_chanid(*m_recordingrule_t, chanid);
}

CStdString MythRecordingRule::Callsign() const
{
  char *buf = cmyth_recordingrule_callsign(*m_recordingrule_t);
  CStdString retval(buf);
  ref_release(buf);
  return retval;
}

void MythRecordingRule::SetCallsign(const CStdString &channame)
{
  cmyth_recordingrule_set_callsign(*m_recordingrule_t, const_cast<char*>(channame.c_str()));
}

time_t MythRecordingRule::StartTime() const
{
  return cmyth_recordingrule_starttime(*m_recordingrule_t);
}

void MythRecordingRule::SetStartTime(time_t starttime)
{
  cmyth_recordingrule_set_starttime(*m_recordingrule_t, starttime);
}

time_t MythRecordingRule::EndTime() const
{
  return cmyth_recordingrule_endtime(*m_recordingrule_t);
}

void MythRecordingRule::SetEndTime(time_t endtime)
{
  cmyth_recordingrule_set_endtime(*m_recordingrule_t, endtime);
}

CStdString MythRecordingRule::Title() const
{
  char *buf = cmyth_recordingrule_title(*m_recordingrule_t);
  CStdString retval(buf);
  ref_release(buf);
  return retval;
}

void MythRecordingRule::SetTitle(const CStdString &title)
{
  cmyth_recordingrule_set_title(*m_recordingrule_t, const_cast<char*>(title.c_str()));
}

CStdString MythRecordingRule::Subtitle() const
{
  char *buf = cmyth_recordingrule_subtitle(*m_recordingrule_t);
  CStdString retval(buf);
  ref_release(buf);
  return retval;
}

void MythRecordingRule::SetSubtitle(const CStdString &subtitle)
{
  cmyth_recordingrule_set_subtitle(*m_recordingrule_t, const_cast<char*>(subtitle.c_str()));
}

CStdString MythRecordingRule::Description() const
{
  char *buf = cmyth_recordingrule_description(*m_recordingrule_t);
  CStdString retval(buf);
  ref_release(buf);
  return retval;
}

void MythRecordingRule::SetDescription(const CStdString &description)
{
  cmyth_recordingrule_set_description(*m_recordingrule_t, const_cast<char*>(description.c_str()));
}

MythRecordingRule::RuleType MythRecordingRule::Type() const
{
  return static_cast<RuleType>(cmyth_recordingrule_type(*m_recordingrule_t));
}

void MythRecordingRule::SetType(MythRecordingRule::RuleType type)
{
  cmyth_recordingrule_set_type(*m_recordingrule_t, type);
}

CStdString MythRecordingRule::Category() const
{
  char *buf = cmyth_recordingrule_category(*m_recordingrule_t);
  CStdString retval(buf);
  ref_release(buf);
  return retval;
}

void MythRecordingRule::SetCategory(const CStdString &category)
{
  cmyth_recordingrule_set_category(*m_recordingrule_t, const_cast<char*>(category.c_str()));
}

int MythRecordingRule::StartOffset() const
{
  return cmyth_recordingrule_startoffset(*m_recordingrule_t);
}

void MythRecordingRule::SetStartOffset(int startoffset)
{
  cmyth_recordingrule_set_startoffset(*m_recordingrule_t, startoffset);
}

int MythRecordingRule::EndOffset() const
{
  return cmyth_recordingrule_endoffset(*m_recordingrule_t);
}

void MythRecordingRule::SetEndOffset(int endoffset)
{
  cmyth_recordingrule_set_endoffset(*m_recordingrule_t, endoffset);
}

int MythRecordingRule::Priority() const
{
  return cmyth_recordingrule_recpriority(*m_recordingrule_t);
}

void MythRecordingRule::SetPriority(int priority)
{
  cmyth_recordingrule_set_recpriority(*m_recordingrule_t, priority);
}

bool MythRecordingRule::Inactive() const
{
  return (cmyth_recordingrule_inactive(*m_recordingrule_t) == 1);
}

void MythRecordingRule::SetInactive(bool inactive)
{
  cmyth_recordingrule_set_inactive(*m_recordingrule_t, (inactive ? 1 : 0));
}

MythRecordingRule::RuleSearchType MythRecordingRule::SearchType() const
{
  return static_cast<RuleSearchType>(cmyth_recordingrule_searchtype(*m_recordingrule_t));
}

void MythRecordingRule::SetSearchType(MythRecordingRule::RuleSearchType searchtype)
{
  cmyth_recordingrule_set_searchtype(*m_recordingrule_t, searchtype);
}

MythRecordingRule::RuleDuplicateControlMethod  MythRecordingRule::DuplicateControlMethod() const
{
  return static_cast<RuleDuplicateControlMethod>(cmyth_recordingrule_dupmethod(*m_recordingrule_t));
}

void  MythRecordingRule::SetDuplicateControlMethod( MythRecordingRule::RuleDuplicateControlMethod method)
{
  cmyth_recordingrule_set_dupmethod(*m_recordingrule_t, method);
}

MythRecordingRule::RuleCheckDuplicatesInType  MythRecordingRule::CheckDuplicatesInType() const
{
  return static_cast<RuleCheckDuplicatesInType>(cmyth_recordingrule_dupin(*m_recordingrule_t));
}

void  MythRecordingRule::SetCheckDuplicatesInType(MythRecordingRule::RuleCheckDuplicatesInType in)
{
  cmyth_recordingrule_set_dupin(*m_recordingrule_t, in);
}

CStdString  MythRecordingRule::RecordingGroup() const
{
  char *buf = cmyth_recordingrule_recgroup(*m_recordingrule_t);
  CStdString retval(buf);
  ref_release(buf);
  return retval;
}

void MythRecordingRule::SetRecordingGroup(const CStdString &group)
{
  cmyth_recordingrule_set_recgroup(*m_recordingrule_t, const_cast<char*>(group.c_str()));
}

CStdString  MythRecordingRule::StorageGroup() const
{
  char *buf = cmyth_recordingrule_storagegroup(*m_recordingrule_t);
  CStdString retval(buf);
  ref_release(buf);
  return retval;
}

void MythRecordingRule::SetStorageGroup(const CStdString &group)
{
  cmyth_recordingrule_set_storagegroup(*m_recordingrule_t, const_cast<char*>(group.c_str()));
}

CStdString  MythRecordingRule::PlaybackGroup() const
{
  char *buf = cmyth_recordingrule_playgroup(*m_recordingrule_t);
  CStdString retval(buf);
  ref_release(buf);
  return retval;
}

void  MythRecordingRule::SetPlaybackGroup(const CStdString &group)
{
  cmyth_recordingrule_set_playgroup(*m_recordingrule_t, const_cast<char*>(group.c_str()));
}

bool  MythRecordingRule::AutoTranscode() const
{
  return (cmyth_recordingrule_autotranscode(*m_recordingrule_t) == 1);
}

void  MythRecordingRule::SetAutoTranscode(bool enable)
{
  cmyth_recordingrule_set_autotranscode(*m_recordingrule_t, (enable ? 1 : 0));
}

bool MythRecordingRule::UserJob(int jobnumber) const
{
  if (jobnumber < 1 || jobnumber > 4)
    return false;
  return ((cmyth_recordingrule_userjobs(*m_recordingrule_t) & (1 << (jobnumber - 1))) == 1);
}

void MythRecordingRule::SetUserJob(int jobnumber, bool enable)
{
  if (jobnumber < 1 || jobnumber > 4)
    return;
  int jobs = cmyth_recordingrule_userjobs(*m_recordingrule_t);
  if (enable)
    jobs |= 1 << (jobnumber - 1);
  else
    jobs &= ~(1 << (jobnumber - 1));
  cmyth_recordingrule_set_userjobs(*m_recordingrule_t, jobs);
}

unsigned short MythRecordingRule::UserJobs() const
{
  return cmyth_recordingrule_userjobs(*m_recordingrule_t);
}

void MythRecordingRule::SetUserJobs(unsigned short jobs)
{
  cmyth_recordingrule_set_userjobs(*m_recordingrule_t, (uint8_t)jobs);
}

bool  MythRecordingRule::AutoMetadata() const
{
  return (cmyth_recordingrule_autometadata(*m_recordingrule_t) == 1);
}

void  MythRecordingRule::SetAutoMetadata(bool enable)
{
  cmyth_recordingrule_set_autometadata(*m_recordingrule_t, (enable ? 1 : 0));
}

bool  MythRecordingRule::AutoCommFlag() const
{
  return (cmyth_recordingrule_autocommflag(*m_recordingrule_t) == 1);
}

void  MythRecordingRule::SetAutoCommFlag(bool enable)
{
  cmyth_recordingrule_set_autocommflag(*m_recordingrule_t, (enable ? 1 : 0));
}

bool  MythRecordingRule::AutoExpire() const
{
  return (cmyth_recordingrule_autoexpire(*m_recordingrule_t) > 0);
}

void  MythRecordingRule::SetAutoExpire(bool enable)
{
  cmyth_recordingrule_set_autoexpire(*m_recordingrule_t, (enable ? 1 : 0));
}

int  MythRecordingRule::MaxEpisodes() const
{
  return cmyth_recordingrule_maxepisodes(*m_recordingrule_t);
}

void  MythRecordingRule::SetMaxEpisodes(int max)
{
  cmyth_recordingrule_set_maxepisodes(*m_recordingrule_t, max);
}

bool  MythRecordingRule::NewExpiresOldRecord() const
{
  return (cmyth_recordingrule_maxnewest(*m_recordingrule_t) > 0);
}

void  MythRecordingRule::SetNewExpiresOldRecord(bool enable)
{
  cmyth_recordingrule_set_maxnewest(*m_recordingrule_t, (enable ? 1 :0));
}

unsigned int MythRecordingRule::Transcoder() const
{
  return cmyth_recordingrule_transcoder(*m_recordingrule_t);
}

void MythRecordingRule::SetTranscoder(unsigned int transcoder)
{
  cmyth_recordingrule_set_transcoder(*m_recordingrule_t, transcoder);
}

unsigned int MythRecordingRule::ParentID() const
{
  return cmyth_recordingrule_parentid(*m_recordingrule_t);
}

void MythRecordingRule::SetParentID(unsigned int parentid)
{
  cmyth_recordingrule_set_parentid(*m_recordingrule_t, parentid);
}

unsigned int MythRecordingRule::Filter() const
{
  return cmyth_recordingrule_filter(*m_recordingrule_t);
}

void MythRecordingRule::SetFilter(unsigned int filter)
{
  cmyth_recordingrule_set_filter(*m_recordingrule_t, filter);
}

CStdString MythRecordingRule::ProgramID() const
{
  char *buf = cmyth_recordingrule_programid(*m_recordingrule_t);
  CStdString retval(buf);
  ref_release(buf);
  return retval;
}

void MythRecordingRule::SetProgramID(const CStdString &programid)
{
  cmyth_recordingrule_set_programid(*m_recordingrule_t, const_cast<char*>(programid.c_str()));
}

CStdString MythRecordingRule::SeriesID() const
{
  char *buf = cmyth_recordingrule_seriesid(*m_recordingrule_t);
  CStdString retval(buf);
  ref_release(buf);
  return retval;
}

void MythRecordingRule::SetSeriesID(const CStdString &seriesid)
{
  cmyth_recordingrule_set_seriesid(*m_recordingrule_t, const_cast<char*>(seriesid.c_str()));
}
