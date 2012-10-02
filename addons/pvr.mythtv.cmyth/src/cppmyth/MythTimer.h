#pragma once
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

#include "platform/util/StdString.h"

extern "C" {
#include <cmyth/cmyth.h>
};

class MythTimer
{
  friend class MythDatabase;

public:
  enum TimerType
  {
    NotRecording = 0,
    SingleRecord = 1,
    TimeslotRecord,
    ChannelRecord,
    AllRecord,
    WeekslotRecord,
    FindOneRecord,
    OverrideRecord,
    DontRecord,
    FindDailyRecord,
    FindWeeklyRecord
  };

  enum TimerSearchType
  {
    NoSearch = 0,
    PowerSearch,
    TitleSearch,
    KeywordSearch,
    PeopleSearch,
    ManualSearch
  };

  enum TimerDuplicateControlMethod
  {
    CheckNone                    = 0x01,
    CheckSubtitle                = 0x02,
    CheckDescription             = 0x04,
    CheckSubtitleAndDescription  = 0x06,
    CheckSubtitleThenDescription = 0x08
  };

  enum TimerCheckDuplicatesInType
  {
    InRecorded    = 0x01,
    InOldRecorded = 0x02,
    InAll         = 0x0F,
    NewEpi        = 0x10
  };

  MythTimer();
  MythTimer(cmyth_timer_t cmyth_timer, bool release = true);

  int RecordID() const;
  void SetRecordID(int recordid);

  int ChannelID() const;
  void SetChannelID(int channelid);

  CStdString Callsign() const;
  void SetCallsign(const CStdString &callsign);

  time_t StartTime() const;
  void SetStartTime(time_t starttime);

  time_t EndTime() const;
  void SetEndTime(time_t endtime);

  CStdString Title(bool subtitleEncoded) const;
  void SetTitle(const CStdString &title, bool subtitleEncoded);

  CStdString Subtitle() const;
  void SetSubtitle(const CStdString &subtitle);

  CStdString Description() const;
  void SetDescription(const CStdString &description);

  TimerType Type() const;
  void SetType(TimerType type);

  CStdString Category() const;
  void SetCategory(const CStdString &category);

  int StartOffset() const;
  void SetStartOffset(int startoffset);

  int EndOffset() const;
  void SetEndOffset(int endoffset);

  int Priority() const;
  void SetPriority(int priority);

  bool Inactive() const;
  void SetInactive(bool inactive);

  TimerSearchType SearchType() const;
  void SetSearchType(TimerSearchType searchtype);

  TimerDuplicateControlMethod DuplicateControlMethod() const;
  void SetDuplicateControlMethod(TimerDuplicateControlMethod method);

  TimerCheckDuplicatesInType CheckDuplicatesInType() const;
  void SetCheckDuplicatesInType(TimerCheckDuplicatesInType in);

  CStdString RecordingGroup() const;
  void SetRecordingGroup(const CStdString &group);

  CStdString StorageGroup() const;
  void SetStorageGroup(const CStdString &group);

  CStdString PlaybackGroup() const;
  void SetPlaybackGroup(const CStdString &group);

  bool AutoTranscode() const;
  void SetAutoTranscode(bool enable);

  bool UserJob(int jobnumber) const;
  void SetUserJob(int jobnumber, bool enable);

  int UserJobs() const;
  void SetUserJobs(int jobs);

  bool AutoCommFlag() const;
  void SetAutoCommFlag(bool enable);

  bool AutoExpire() const;
  void SetAutoExpire(bool enable);

  int MaxEpisodes() const;
  void SetMaxEpisodes(int max);

  bool NewExpiresOldRecord() const;
  void SetNewExpiresOldRecord(bool enable);

  int Transcoder() const;
  void SetTranscoder(int transcoder);

private:
  int m_recordid;
  int m_channelid;
  CStdString m_callsign;
  time_t m_starttime;
  time_t m_endtime;
  CStdString m_title;
  CStdString m_description;
  TimerType m_type;
  CStdString m_category;
  CStdString m_subtitle;
  int m_priority;
  int m_startoffset;
  int m_endoffset;
  TimerSearchType m_searchtype;
  bool m_inactive;
  TimerDuplicateControlMethod m_dupmethod;
  TimerCheckDuplicatesInType m_dupin;
  CStdString m_recgroup;
  CStdString m_storegroup;
  CStdString m_playgroup;
  bool m_autotranscode;
  int m_userjobs;
  bool m_autocommflag;
  bool m_autoexpire;
  int m_maxepisodes;
  bool m_maxnewest;
  int m_transcoder;
};
