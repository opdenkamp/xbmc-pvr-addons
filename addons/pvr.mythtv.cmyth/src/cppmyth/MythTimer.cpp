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

#include "MythTimer.h"

extern "C" {
#include <refmem/refmem.h>
};

MythTimer::MythTimer()
  : m_recordid(-1)
  , m_channelid(-1)
  , m_callsign("")
  , m_starttime(0)
  , m_endtime(0)
  , m_title("")
  , m_description("")
  , m_type(NotRecording)
  , m_category("")
  , m_subtitle("")
  , m_priority(0)
  , m_startoffset(0)
  , m_endoffset(0)
  , m_searchtype(NoSearch)
  , m_inactive(true)
  , m_dupmethod(CheckSubtitleAndDescription)
  , m_dupin(InAll)
  , m_recgroup("Default")
  , m_storegroup("Default")
  , m_playgroup("Default")
  , m_autotranscode(false)
  , m_userjobs(0)
  , m_autocommflag(false)
  , m_autoexpire(false)
  , m_maxepisodes(0)
  , m_maxnewest(0)
  , m_transcoder(0)
{
}

MythTimer::MythTimer(cmyth_timer_t cmyth_timer, bool release)
  : m_recordid(cmyth_timer_recordid(cmyth_timer))
  , m_channelid(cmyth_timer_chanid(cmyth_timer))
  , m_callsign("")
  , m_starttime(cmyth_timer_starttime(cmyth_timer))
  , m_endtime(cmyth_timer_endtime(cmyth_timer))
  , m_title("")
  , m_description("")
  , m_type(static_cast<TimerType>(cmyth_timer_type(cmyth_timer)))
  , m_category("")
  , m_subtitle("")
  , m_priority(cmyth_timer_priority(cmyth_timer))
  , m_startoffset(cmyth_timer_startoffset(cmyth_timer))
  , m_endoffset(cmyth_timer_endoffset(cmyth_timer))
  , m_searchtype(static_cast<TimerSearchType>(cmyth_timer_searchtype(cmyth_timer)))
  , m_inactive(cmyth_timer_inactive(cmyth_timer) != 0)
  , m_dupmethod(static_cast<TimerDuplicateControlMethod>(cmyth_timer_dup_method(cmyth_timer)))
  , m_dupin(static_cast<TimerCheckDuplicatesInType>(cmyth_timer_dup_in(cmyth_timer)))
  , m_recgroup("")
  , m_storegroup("")
  , m_playgroup("")
  , m_autotranscode(cmyth_timer_autotranscode(cmyth_timer) == 1)
  , m_userjobs(cmyth_timer_userjobs(cmyth_timer))
  , m_autocommflag(cmyth_timer_autocommflag(cmyth_timer) == 1)
  , m_autoexpire(cmyth_timer_autoexpire(cmyth_timer) == 1)
  , m_maxepisodes(cmyth_timer_maxepisodes(cmyth_timer))
  , m_maxnewest(cmyth_timer_maxnewest(cmyth_timer) == 1)
  , m_transcoder(cmyth_timer_transcoder(cmyth_timer))
{
  char *title = cmyth_timer_title(cmyth_timer);
  char *description = cmyth_timer_description(cmyth_timer);
  char *category = cmyth_timer_category(cmyth_timer);
  char *subtitle = cmyth_timer_subtitle(cmyth_timer);
  char *callsign = cmyth_timer_callsign(cmyth_timer);
  char *recgroup = cmyth_timer_rec_group(cmyth_timer);
  char *storegroup = cmyth_timer_store_group(cmyth_timer);
  char *playgroup = cmyth_timer_play_group(cmyth_timer);

  m_title = title;
  m_description = description;
  m_category = category;
  m_subtitle = subtitle;
  m_callsign = callsign;
  m_recgroup = recgroup;
  m_storegroup = storegroup;
  m_playgroup = playgroup;

  ref_release(title);
  ref_release(description);
  ref_release(category);
  ref_release(subtitle);
  ref_release(callsign);
  ref_release(recgroup);
  ref_release(storegroup);
  ref_release(playgroup);

  if (release)
    ref_release(cmyth_timer);
}

int MythTimer::RecordID() const
{
  return m_recordid;
}

void MythTimer::SetRecordID(int recordid)
{
  m_recordid = recordid;
}

int MythTimer::ChannelID() const
{
  return m_channelid;
}

void MythTimer::SetChannelID(int chanid)
{
  m_channelid = chanid;
}

CStdString MythTimer::Callsign() const
{
  return m_callsign;
}

void MythTimer::SetCallsign(const CStdString &channame)
{
  m_callsign = channame;
}

time_t MythTimer::StartTime() const
{
  return m_starttime;
}

void MythTimer::SetStartTime(time_t starttime)
{
  m_starttime = starttime;
}

time_t MythTimer::EndTime() const
{
  return m_endtime;
}

void MythTimer::SetEndTime(time_t endtime)
{
  m_endtime = endtime;
}

CStdString MythTimer::Title(bool subtitleEncoded) const
{
  if (subtitleEncoded && !m_subtitle.empty())
  {
    CStdString retval;
    retval.Format("%s::%s", m_title, m_subtitle);
    return retval;
  }
  return m_title;
}

void MythTimer::SetTitle(const CStdString &title, bool subtitleEncoded)
{
  if (subtitleEncoded)
  {
    size_t seppos = title.find("::");
    if (seppos != CStdString::npos)
    {
      m_title = title.substr(0, seppos);
      m_subtitle = title.substr(seppos + 2);
      return;
    }
  }
  m_title = title;
}

CStdString MythTimer::Subtitle() const
{
  return m_subtitle;
}

void MythTimer::SetSubtitle(const CStdString &subtitle)
{
  m_subtitle = subtitle;
}

CStdString MythTimer::Description() const
{
  return m_description;
}

void MythTimer::SetDescription(const CStdString &description)
{
  m_description = description;
}

MythTimer::TimerType MythTimer::Type() const
{
  return m_type;
}

void MythTimer::SetType(MythTimer::TimerType type)
{
  m_type = type;
}

CStdString MythTimer::Category() const
{
  return m_category;
}

void MythTimer::SetCategory(const CStdString &category)
{
  m_category = category;
}

int MythTimer::StartOffset() const
{
  return m_startoffset;
}

void MythTimer::SetStartOffset(int startoffset)
{
  m_startoffset = startoffset;
}

int MythTimer::EndOffset() const
{
  return m_endoffset;
}

void MythTimer::SetEndOffset(int endoffset)
{
  m_endoffset = endoffset;
}

int MythTimer::Priority() const
{
  return m_priority;
}

void MythTimer::SetPriority(int priority)
{
  m_priority = priority;
}

bool MythTimer::Inactive() const
{
  return m_inactive;
}

void MythTimer::SetInactive(bool inactive)
{
  m_inactive = inactive;
}

MythTimer::TimerSearchType MythTimer::SearchType() const
{
  return m_searchtype;
}

void MythTimer::SetSearchType(MythTimer::TimerSearchType searchtype)
{
  m_searchtype = searchtype;
}

MythTimer::TimerDuplicateControlMethod  MythTimer::DuplicateControlMethod() const
{
  return m_dupmethod;
}

void  MythTimer::SetDuplicateControlMethod( MythTimer::TimerDuplicateControlMethod method)
{
  m_dupmethod = method;
}

MythTimer::TimerCheckDuplicatesInType  MythTimer::CheckDuplicatesInType() const
{
  return m_dupin;
}

void  MythTimer::SetCheckDuplicatesInType(MythTimer::TimerCheckDuplicatesInType in)
{
  m_dupin = in;
}

CStdString  MythTimer::RecordingGroup() const
{
  return m_recgroup;
}

void MythTimer::SetRecordingGroup(const CStdString &group)
{
  m_recgroup = group;
}

CStdString  MythTimer::StorageGroup() const
{
  return m_storegroup;
}

void MythTimer::SetStorageGroup(const CStdString &group)
{
  m_storegroup = group;
}

CStdString  MythTimer::PlaybackGroup() const
{
  return m_playgroup;
}

void  MythTimer::SetPlaybackGroup(const CStdString &group)
{
  m_playgroup = group;
}

bool  MythTimer::AutoTranscode() const
{
  return m_autotranscode;
}

void  MythTimer::SetAutoTranscode(bool enable)
{
  m_autotranscode = enable;
}

bool MythTimer::UserJob(int jobnumber) const
{
  if (jobnumber < 1 || jobnumber > 4)
    return false;
  return (m_userjobs & (1 << (jobnumber - 1))) == 1;
}

void MythTimer::SetUserJob(int jobnumber, bool enable)
{
  if (jobnumber < 1 || jobnumber > 4)
    return;
  if (enable)
    m_userjobs |= 1 << (jobnumber - 1);
  else
    m_userjobs &= ~(1 << (jobnumber - 1));
}

int MythTimer::UserJobs() const
{
  return m_userjobs;
}

void MythTimer::SetUserJobs(int jobs)
{
  m_userjobs = jobs;
}

bool  MythTimer::AutoCommFlag() const
{
  return m_autocommflag;
}
  
void  MythTimer::SetAutoCommFlag(bool enable)
{
  m_autocommflag = enable;
}

bool  MythTimer::AutoExpire() const
{
  return m_autoexpire;
}

void  MythTimer::SetAutoExpire(bool enable)
{
  m_autoexpire = enable;
}

int  MythTimer::MaxEpisodes() const
{
  return m_maxepisodes;
}

void  MythTimer::SetMaxEpisodes(int max)
{
  m_maxepisodes = max;
}

bool  MythTimer::NewExpiresOldRecord() const
{
  return m_maxnewest;
}

void  MythTimer::SetNewExpiresOldRecord(bool enable)
{
  m_maxnewest = enable;
}

int MythTimer::Transcoder() const
{
  return m_transcoder;
}

void MythTimer::SetTranscoder(int transcoder)
{
  m_transcoder = transcoder;
}
