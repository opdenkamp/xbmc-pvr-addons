#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
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
#include "MythProgramInfo.h"
#include "MythConnection.h"
#include "MythDatabase.h"
#include "MythEPGInfo.h"

#include <platform/threads/mutex.h>

#include <vector>
#include <list>
#include <map>

typedef std::vector<MythRecordingRule> OverrideRuleList;

// Schedule element is pair < index of schedule , program info of schedule >
typedef std::vector<std::pair<unsigned int, boost::shared_ptr<MythProgramInfo> > > ScheduleList;

typedef struct
{
  bool        isRepeating;
  int         weekDays;
  const char* marker;
} RuleMetadata;

class MythRecordingRuleNode
{
public:
  friend class MythScheduleManager;

  MythRecordingRuleNode(const MythRecordingRule &rule);

  bool IsOverrideRule() const;
  MythRecordingRule GetRule() const;
  MythRecordingRule GetMainRule() const;

  bool HasOverrideRules() const;
  OverrideRuleList GetOverrideRules() const;

  bool IsInactiveRule() const;

private:
  MythRecordingRule m_rule;
  MythRecordingRule m_mainRule;
  std::vector<MythRecordingRule> m_overrideRules;
};

class MythScheduleManager
{
public:
  enum MSM_ERROR {
    MSM_ERROR_FAILED = -1,
    MSM_ERROR_NOT_IMPLEMENTED = 0,
    MSM_ERROR_SUCCESS = 1
  };

  MythScheduleManager();
  MythScheduleManager(MythConnection &con, MythDatabase &db);

  // Called by GetTimers
  ScheduleList GetUpcomingRecordings();

  // Called by AddTimer
  MSM_ERROR ScheduleRecording(const MythRecordingRule &rule);

  // Called by DeleteTimer
  MSM_ERROR DeleteRecording(unsigned int index);

  MSM_ERROR DisableRecording(unsigned int index);
  MSM_ERROR EnableRecording(unsigned int index);
  MSM_ERROR UpdateRecording(unsigned int index, MythRecordingRule &newrule);

  boost::shared_ptr<MythRecordingRuleNode> FindRuleById(unsigned int recordID) const;
  ScheduleList FindUpComingByRuleId(unsigned int recordID) const;
  boost::shared_ptr<MythProgramInfo> FindUpComingByIndex(unsigned int index) const;

  void Update();

  class VersionHelper
  {
  public:
    friend class MythScheduleManager;

    VersionHelper() {}
    virtual ~VersionHelper();
    virtual bool SameTimeslot(MythRecordingRule &first, MythRecordingRule &second) const = 0;
    virtual RuleMetadata GetMetadata(const MythRecordingRule &rule) const = 0;
    virtual MythRecordingRule NewFromTemplate(MythEPGInfo &epgInfo) = 0;
    virtual MythRecordingRule NewSingleRecord(MythEPGInfo &epgInfo) = 0;
    virtual MythRecordingRule NewDailyRecord(MythEPGInfo &epgInfo) = 0;
    virtual MythRecordingRule NewWeeklyRecord(MythEPGInfo &epgInfo) = 0;
    virtual MythRecordingRule NewChannelRecord(MythEPGInfo &epgInfo) = 0;
    virtual MythRecordingRule NewOneRecord(MythEPGInfo &epgInfo) = 0;
  };

  RuleMetadata GetMetadata(const MythRecordingRule &rule) const;
  MythRecordingRule NewFromTemplate(MythEPGInfo &epgInfo);
  MythRecordingRule NewSingleRecord(MythEPGInfo &epgInfo);
  MythRecordingRule NewDailyRecord(MythEPGInfo &epgInfo);
  MythRecordingRule NewWeeklyRecord(MythEPGInfo &epgInfo);
  MythRecordingRule NewChannelRecord(MythEPGInfo &epgInfo);
  MythRecordingRule NewOneRecord(MythEPGInfo &epgInfo);

  bool ToggleShowNotRecording();

private:
  mutable PLATFORM::CMutex m_lock;
  MythConnection m_con;
  MythDatabase m_db;

  int m_dbSchemaVersion;
  boost::shared_ptr<VersionHelper> m_versionHelper;
  void Setup();

  unsigned int MakeIndex(MythProgramInfo &recording) const;

  // The list of rule nodes
  typedef std::list<boost::shared_ptr<MythRecordingRuleNode> > NodeList;
  // To find a rule node by its key (recordID)
  typedef std::map<unsigned int, boost::shared_ptr<MythRecordingRuleNode> > NodeById;
  // Store and find up coming recordings by index
  typedef std::map<unsigned int, boost::shared_ptr<MythProgramInfo> > RecordingList;
  // To find all indexes of schedule by rule Id : pair < Rule Id , index of schedule >
  typedef std::multimap<unsigned int, unsigned int> RecordingIndexByRuleId;

  NodeList m_rules;
  NodeById m_rulesById;
  RecordingList m_recordings;
  RecordingIndexByRuleId m_recordingIndexByRuleId;

  bool m_showNotRecording;
};


///////////////////////////////////////////////////////////////////////////////
////
//// VersionHelper
////

inline MythScheduleManager::VersionHelper::~VersionHelper() {
}

// No helper

class MythScheduleHelperNoHelper : public MythScheduleManager::VersionHelper {
public:
  virtual bool SameTimeslot(MythRecordingRule &first, MythRecordingRule &second) const;
  virtual RuleMetadata GetMetadata(const MythRecordingRule &rule) const;
  virtual MythRecordingRule NewFromTemplate(MythEPGInfo &epgInfo);
  virtual MythRecordingRule NewSingleRecord(MythEPGInfo &epgInfo);
  virtual MythRecordingRule NewDailyRecord(MythEPGInfo &epgInfo);
  virtual MythRecordingRule NewWeeklyRecord(MythEPGInfo &epgInfo);
  virtual MythRecordingRule NewChannelRecord(MythEPGInfo &epgInfo);
  virtual MythRecordingRule NewOneRecord(MythEPGInfo &epgInfo);
};

// Base 0.24

class MythScheduleHelper1226 : public MythScheduleHelperNoHelper {
public:

  MythScheduleHelper1226(MythDatabase &db) : m_db(db) {
  }
  virtual bool SameTimeslot(MythRecordingRule &first, MythRecordingRule &second) const;
  virtual RuleMetadata GetMetadata(const MythRecordingRule &rule) const;
  virtual MythRecordingRule NewFromTemplate(MythEPGInfo &epgInfo);
  virtual MythRecordingRule NewSingleRecord(MythEPGInfo &epgInfo);
  virtual MythRecordingRule NewDailyRecord(MythEPGInfo &epgInfo);
  virtual MythRecordingRule NewWeeklyRecord(MythEPGInfo &epgInfo);
  virtual MythRecordingRule NewChannelRecord(MythEPGInfo &epgInfo);
  virtual MythRecordingRule NewOneRecord(MythEPGInfo &epgInfo);
protected:
  MythDatabase m_db;
};

// News in 0.25

class MythScheduleHelper1278 : public MythScheduleHelper1226 {
public:

  MythScheduleHelper1278(MythDatabase &db) : MythScheduleHelper1226(db) {
  }
  virtual bool SameTimeslot(MythRecordingRule &first, MythRecordingRule &second) const;
  virtual MythRecordingRule NewFromTemplate(MythEPGInfo &epgInfo);
};

// News in 0.26

class MythScheduleHelper1302 : public MythScheduleHelper1278 {
public:

  MythScheduleHelper1302(MythDatabase &db) : MythScheduleHelper1278(db) {
  }
  virtual MythRecordingRule NewFromTemplate(MythEPGInfo &epgInfo);
};

// News in 0.27

class MythScheduleHelper1309 : public MythScheduleHelper1302 {
public:

  MythScheduleHelper1309(MythDatabase &db) : MythScheduleHelper1302(db) {
  }
  virtual RuleMetadata GetMetadata(const MythRecordingRule &rule) const;
  virtual MythRecordingRule NewDailyRecord(MythEPGInfo &epgInfo);
  virtual MythRecordingRule NewWeeklyRecord(MythEPGInfo &epgInfo);
  virtual MythRecordingRule NewChannelRecord(MythEPGInfo &epgInfo);
  virtual MythRecordingRule NewOneRecord(MythEPGInfo &epgInfo);
};
