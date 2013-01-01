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

#include <boost/shared_ptr.hpp>

extern "C" {
#include <cmyth/cmyth.h>
};

template <class T> class MythPointer;

class MythRecordingRule
{
  friend class MythDatabase;

public:
  enum RuleType
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

  enum RuleSearchType
  {
    NoSearch = 0,
    PowerSearch,
    TitleSearch,
    KeywordSearch,
    PeopleSearch,
    ManualSearch
  };

  enum RuleDuplicateControlMethod
  {
    CheckNone                    = 0x01,
    CheckSubtitle                = 0x02,
    CheckDescription             = 0x04,
    CheckSubtitleAndDescription  = 0x06,
    CheckSubtitleThenDescription = 0x08
  };

  enum RuleCheckDuplicatesInType
  {
    InRecorded    = 0x01,
    InOldRecorded = 0x02,
    InAll         = 0x0F,
    NewEpi        = 0x10
  };

  MythRecordingRule();
  MythRecordingRule(cmyth_recordingrule_t cmyth_recordingrule);

  bool IsNull() const;

  unsigned int RecordID() const;
  void SetRecordID(unsigned int recordid);

  unsigned int ChannelID() const;
  void SetChannelID(unsigned int channelid);

  CStdString Callsign() const;
  void SetCallsign(const CStdString &callsign);

  time_t StartTime() const;
  void SetStartTime(time_t starttime);

  time_t EndTime() const;
  void SetEndTime(time_t endtime);

  CStdString Title() const;
  void SetTitle(const CStdString &title);

  CStdString Subtitle() const;
  void SetSubtitle(const CStdString &subtitle);

  CStdString Description() const;
  void SetDescription(const CStdString &description);

  RuleType Type() const;
  void SetType(RuleType type);

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

  RuleSearchType SearchType() const;
  void SetSearchType(RuleSearchType searchtype);

  RuleDuplicateControlMethod DuplicateControlMethod() const;
  void SetDuplicateControlMethod(RuleDuplicateControlMethod method);

  RuleCheckDuplicatesInType CheckDuplicatesInType() const;
  void SetCheckDuplicatesInType(RuleCheckDuplicatesInType in);

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

  unsigned short UserJobs() const;
  void SetUserJobs(unsigned short jobs);

  bool AutoCommFlag() const;
  void SetAutoCommFlag(bool enable);

  bool AutoExpire() const;
  void SetAutoExpire(bool enable);

  int MaxEpisodes() const;
  void SetMaxEpisodes(int max);

  bool NewExpiresOldRecord() const;
  void SetNewExpiresOldRecord(bool enable);

  unsigned int Transcoder() const;
  void SetTranscoder(unsigned int transcoder);

private:
  boost::shared_ptr<MythPointer<cmyth_recordingrule_t> > m_recordingrule_t;
};
