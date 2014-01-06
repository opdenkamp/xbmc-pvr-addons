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

#include "MythScheduleManager.h"
#include "../client.h"
#include "../tools.h"

using namespace ADDON;
using namespace PLATFORM;

enum
{
  METHOD_UNKNOWN = 0,
  METHOD_UPDATE_INACTIVE = 1,
  METHOD_CREATE_OVERRIDE = 2,
  METHOD_DELETE = 3,
  METHOD_DISCREET_UPDATE = 4,
  METHOD_FULL_UPDATE = 5
};

static uint_fast32_t hashvalue(uint_fast32_t maxsize, const char *value)
{
  uint_fast32_t h = 0, g;

  while (*value)
  {
    h = (h << 4) + *value++;
    if ((g = h & 0xF0000000L))
    {
      h ^= g >> 24;
    }
    h &= ~g;
  }

  return h % maxsize;
}


///////////////////////////////////////////////////////////////////////////////
////
//// MythRecordingRuleNode
////

MythRecordingRuleNode::MythRecordingRuleNode(const MythRecordingRule &rule)
  : m_rule(rule)
  , m_mainRule()
  , m_overrideRules()
{
}

bool MythRecordingRuleNode::IsOverrideRule() const
{
  return (m_rule.Type() == MythRecordingRule::RT_DontRecord || m_rule.Type() == MythRecordingRule::RT_OverrideRecord);
}

MythRecordingRule MythRecordingRuleNode::GetRule() const
{
  return m_rule;
}

MythRecordingRule MythRecordingRuleNode::GetMainRule() const
{
  if (this->IsOverrideRule())
    return m_mainRule;
  return m_rule;
}

bool MythRecordingRuleNode::HasOverrideRules() const
{
  return (!m_overrideRules.empty());
}

OverrideRuleList MythRecordingRuleNode::GetOverrideRules() const
{
  return m_overrideRules;
}

bool MythRecordingRuleNode::IsInactiveRule() const
{
  return m_rule.Inactive();
}


///////////////////////////////////////////////////////////////////////////////
////
//// MythScheduleManager
////

MythScheduleManager::MythScheduleManager()
  : m_con()
  , m_db()
  , m_dbSchemaVersion(0)
  , m_versionHelper(new MythScheduleHelperNoHelper())
  , m_showNotRecording(false)
{
}

MythScheduleManager::MythScheduleManager(MythConnection &con, MythDatabase &db)
  : m_con(con)
  , m_db(db)
  , m_dbSchemaVersion(0)
  , m_showNotRecording(false)
{
  this->Update();
}

void MythScheduleManager::Setup()
{
  int old = m_dbSchemaVersion;
  m_dbSchemaVersion = m_db.GetSchemaVersion();

  // On new DB connection the schema version could change
  if (m_dbSchemaVersion != old)
  {
    if (m_dbSchemaVersion >= 1309)
      m_versionHelper = boost::shared_ptr<VersionHelper>(new MythScheduleHelper1309(m_db));
    else if (m_dbSchemaVersion >= 1302)
      m_versionHelper = boost::shared_ptr<VersionHelper>(new MythScheduleHelper1302(m_db));
    else if (m_dbSchemaVersion >= 1276)
      m_versionHelper = boost::shared_ptr<VersionHelper>(new MythScheduleHelper1278(m_db));
    else if (m_dbSchemaVersion > 0)
      m_versionHelper = boost::shared_ptr<VersionHelper>(new MythScheduleHelper1226(m_db));
    else
      m_versionHelper = boost::shared_ptr<VersionHelper>(new MythScheduleHelperNoHelper());
  }
}

unsigned int MythScheduleManager::MakeIndex(MythProgramInfo& recording) const
{
  // Recordings must keep same identifier even after refreshing cache (cf Update).
  // Numeric hash of UID is used to make the constant numeric identifier.
  return (recording.RecordID() << 16) + hashvalue(0xFFFF, recording.UID().c_str());
}

ScheduleList MythScheduleManager::GetUpcomingRecordings()
{
  ScheduleList recordings;
  CLockObject lock(m_lock);
  for (RecordingList::iterator it = m_recordings.begin(); it != m_recordings.end(); ++it)
  {
    recordings.push_back(std::make_pair(it->first, it->second));
  }
  return recordings;
}

MythScheduleManager::MSM_ERROR MythScheduleManager::ScheduleRecording(const MythRecordingRule &rule)
{
  // Don't schedule nil
  if (rule.Type() == MythRecordingRule::RT_NotRecording)
    return MSM_ERROR_FAILED;

  if (!m_db.AddRecordingRule(rule))
    return MSM_ERROR_FAILED;

  if (!m_con.UpdateSchedules(rule.RecordID()))
    return MSM_ERROR_FAILED;

  return MSM_ERROR_SUCCESS;
}

MythScheduleManager::MSM_ERROR MythScheduleManager::DeleteRecording(unsigned int index)
{
  CLockObject lock(m_lock);

  boost::shared_ptr<MythProgramInfo> recording = this->FindUpComingByIndex(index);
  if (!recording)
    return MSM_ERROR_FAILED;

  boost::shared_ptr<MythRecordingRuleNode> node = this->FindRuleById(recording->RecordID());
  if (node)
  {
    XBMC->Log(LOG_DEBUG, "%s - %u : Found rule %u type %d", __FUNCTION__, index, node->m_rule.RecordID(), node->m_rule.Type());

    // Delete override rules
    if (node->HasOverrideRules())
    {
      for (OverrideRuleList::iterator ito = node->m_overrideRules.begin(); ito != node->m_overrideRules.end(); ++ito)
      {
        XBMC->Log(LOG_DEBUG, "%s - %u : Found override rule %u type %d", __FUNCTION__, index, ito->RecordID(), ito->Type());
        ScheduleList rec = this->FindUpComingByRuleId(ito->RecordID());
        for (ScheduleList::iterator itr = rec.begin(); itr != rec.end(); ++itr)
        {
          XBMC->Log(LOG_DEBUG, "%s - %u : Found override recording %s status %d", __FUNCTION__, index, itr->second->UID().c_str(), itr->second->Status());
          if (itr->second->Status() == RS_RECORDING || itr->second->Status() == RS_TUNING)
          {
            XBMC->Log(LOG_DEBUG, "%s - Stop recording %s", __FUNCTION__, itr->second->UID().c_str());
            m_con.StopRecording(*(itr->second));
          }
        }
        XBMC->Log(LOG_DEBUG, "%s - Delete recording rule %u (modifier of rule %u)", __FUNCTION__, ito->RecordID(), node->m_rule.RecordID());
        if (!m_db.DeleteRecordingRule(*ito))
          XBMC->Log(LOG_ERROR, "%s - Delete recording rule failed", __FUNCTION__);
      }
    }

    // Delete main rule
    ScheduleList rec = this->FindUpComingByRuleId(node->m_rule.RecordID());
    for (ScheduleList::iterator itr = rec.begin(); itr != rec.end(); ++itr)
    {
      XBMC->Log(LOG_DEBUG, "%s - %u : Found recording %s status %d", __FUNCTION__, index, itr->second->UID().c_str(), itr->second->Status());
      if (itr->second->Status() == RS_RECORDING || itr->second->Status() == RS_TUNING)
      {
        XBMC->Log(LOG_DEBUG, "%s - Stop recording %s", __FUNCTION__, itr->second->UID().c_str());
        m_con.StopRecording(*(itr->second));
      }
    }
    XBMC->Log(LOG_DEBUG, "%s - Delete recording rule %u", __FUNCTION__, node->m_rule.RecordID());
    if (!m_db.DeleteRecordingRule(node->m_rule))
      XBMC->Log(LOG_ERROR, "%s - Delete recording rule failed", __FUNCTION__);

    if (!m_con.UpdateSchedules(-1))
      return MSM_ERROR_FAILED;

    // Another client could delete the rule at the same time. Therefore always SUCCESS even if database delete fails.
    return MSM_ERROR_SUCCESS;
  }
  else
  {
    XBMC->Log(LOG_DEBUG, "%s - %u : No rule for recording %s status %d", __FUNCTION__, index, recording->UID().c_str(), recording->Status());
    if (recording->Status() == RS_RECORDING || recording->Status() == RS_TUNING)
    {
      XBMC->Log(LOG_DEBUG, "%s - Stop recording %s", __FUNCTION__, recording->UID().c_str());
      m_con.StopRecording(*recording);
      return MSM_ERROR_SUCCESS;
    }
  }

  return MSM_ERROR_NOT_IMPLEMENTED;
}

MythScheduleManager::MSM_ERROR MythScheduleManager::DisableRecording(unsigned int index)
{
  CLockObject lock(m_lock);

  boost::shared_ptr<MythProgramInfo> recording = this->FindUpComingByIndex(index);
  if (!recording)
    return MSM_ERROR_FAILED;

  if (recording->Status() == RS_INACTIVE || recording->Status() == RS_DONT_RECORD)
    return MSM_ERROR_SUCCESS;

  boost::shared_ptr<MythRecordingRuleNode> node = this->FindRuleById(recording->RecordID());
  if (node)
  {
    XBMC->Log(LOG_DEBUG, "%s - %u : Found rule %u type %d", __FUNCTION__, index, node->m_rule.RecordID(), node->m_rule.Type());
    int method = METHOD_UNKNOWN;
    MythRecordingRule handle = node->m_rule.DuplicateRecordingRule();

    // Not recording. Simply inactivate the rule
    if (recording->Status() == RS_UNKNOWN)
    {
      method = METHOD_UPDATE_INACTIVE;
    }
    else
    {
      // Method depends of its rule type
      switch (node->m_rule.Type())
      {
      case MythRecordingRule::RT_SingleRecord:
        if (recording->Status() == RS_RECORDING || recording->Status() == RS_TUNING)
          method = METHOD_DELETE;
        else
          method = METHOD_UPDATE_INACTIVE;
        break;
      case MythRecordingRule::RT_NotRecording:
        method = METHOD_UPDATE_INACTIVE;
        break;
      case MythRecordingRule::RT_OneRecord:
      case MythRecordingRule::RT_ChannelRecord:
      case MythRecordingRule::RT_AllRecord:
      case MythRecordingRule::RT_DailyRecord:
      case MythRecordingRule::RT_WeeklyRecord:
      case MythRecordingRule::RT_FindDailyRecord:
      case MythRecordingRule::RT_FindWeeklyRecord:
        method = METHOD_CREATE_OVERRIDE;
        break;
      case MythRecordingRule::RT_OverrideRecord:
        method = METHOD_DELETE;
        break;
      default:
        break;
      }
    }

    if (method == METHOD_UNKNOWN)
    {
      return MSM_ERROR_NOT_IMPLEMENTED;
    }
    if (method == METHOD_UPDATE_INACTIVE)
    {
      handle.SetInactive(true);
      if (!m_db.UpdateRecordingRule(handle))
        return MSM_ERROR_FAILED;
      node->m_rule = handle; // sync node
      if (!m_con.UpdateSchedules(handle.RecordID()))
        return MSM_ERROR_FAILED;
      return MSM_ERROR_SUCCESS;
    }
    if (method == METHOD_CREATE_OVERRIDE)
    {
      handle.SetRecordID(0);
      handle.SetInactive(false);
      handle.SetType(MythRecordingRule::RT_DontRecord);
      handle.SetStartTime(recording->StartTime());
      handle.SetEndTime(recording->EndTime());
      handle.SetParentID(node->m_rule.RecordID());
      if (!m_db.AddRecordingRule(handle))
        return MSM_ERROR_FAILED;
      node->m_overrideRules.push_back(handle); // sync node
      if (!m_con.UpdateSchedules(handle.RecordID()))
        return MSM_ERROR_FAILED;
      return MSM_ERROR_SUCCESS;
    }
    if (method == METHOD_DELETE)
    {
      return this->DeleteRecording(index);
    }
  }

  return MSM_ERROR_NOT_IMPLEMENTED;
}

MythScheduleManager::MSM_ERROR MythScheduleManager::EnableRecording(unsigned int index)
{
  CLockObject lock(m_lock);

  boost::shared_ptr<MythProgramInfo> recording = this->FindUpComingByIndex(index);
  if (!recording)
    return MSM_ERROR_FAILED;

  boost::shared_ptr<MythRecordingRuleNode> node = this->FindRuleById(recording->RecordID());
  if (node)
  {
    XBMC->Log(LOG_DEBUG, "%s - %u : Found rule %u type %d", __FUNCTION__, index, node->m_rule.RecordID(), node->m_rule.Type());
    int method = METHOD_UNKNOWN;
    MythRecordingRule handle = node->m_rule.DuplicateRecordingRule();

    if (recording->Status() == RS_UNKNOWN)
    {
      // Not recording. Simply activate the rule
      method = METHOD_UPDATE_INACTIVE;
    }
    else if (recording->Status() == RS_NEVER_RECORD)
    {
      // Add override to record anyway
      method = METHOD_CREATE_OVERRIDE;
    }
    else
    {
      // Method depends of its rule type
      switch (node->m_rule.Type())
      {
      case MythRecordingRule::RT_DontRecord:
      case MythRecordingRule::RT_OverrideRecord:
        method = METHOD_DELETE;
        break;
      case MythRecordingRule::RT_SingleRecord:
      case MythRecordingRule::RT_NotRecording:
      case MythRecordingRule::RT_OneRecord:
      case MythRecordingRule::RT_ChannelRecord:
      case MythRecordingRule::RT_AllRecord:
      case MythRecordingRule::RT_DailyRecord:
      case MythRecordingRule::RT_WeeklyRecord:
      case MythRecordingRule::RT_FindDailyRecord:
      case MythRecordingRule::RT_FindWeeklyRecord:
        // Is it inactive ? Try to enable rule
        method = METHOD_UPDATE_INACTIVE;
        break;
      default:
        break;
      }
    }

    if (method == METHOD_UNKNOWN)
    {
      return MSM_ERROR_NOT_IMPLEMENTED;
    }
    if (method == METHOD_UPDATE_INACTIVE)
    {
      handle.SetInactive(false);
      if (!m_db.UpdateRecordingRule(handle))
        return MSM_ERROR_FAILED;
      node->m_rule = handle; // sync node
      if (!m_con.UpdateSchedules(handle.RecordID()))
        return MSM_ERROR_FAILED;
      return MSM_ERROR_SUCCESS;
    }
    if (method == METHOD_CREATE_OVERRIDE)
    {
      handle.SetRecordID(0);
      handle.SetInactive(false);
      handle.SetType(MythRecordingRule::RT_OverrideRecord);
      handle.SetStartTime(recording->StartTime());
      handle.SetEndTime(recording->EndTime());
      handle.SetParentID(node->m_rule.RecordID());
      if (!m_db.AddRecordingRule(handle))
        return MSM_ERROR_FAILED;
      node->m_overrideRules.push_back(handle); // sync node
      if (!m_con.UpdateSchedules(handle.RecordID()))
        return MSM_ERROR_FAILED;
      return MSM_ERROR_SUCCESS;
    }
    if (method == METHOD_DELETE)
    {
      return this->DeleteRecording(index);
    }
  }

  return MSM_ERROR_NOT_IMPLEMENTED;
}

MythScheduleManager::MSM_ERROR MythScheduleManager::UpdateRecording(unsigned int index, MythRecordingRule &newrule)
{
  CLockObject lock(m_lock);

  boost::shared_ptr<MythProgramInfo> recording = this->FindUpComingByIndex(index);
  if (!recording)
    return MSM_ERROR_FAILED;

  boost::shared_ptr<MythRecordingRuleNode> node = this->FindRuleById(recording->RecordID());
  if (node)
  {
    XBMC->Log(LOG_DEBUG, "%s - %u : Found rule %u type %d", __FUNCTION__, index, node->m_rule.RecordID(), node->m_rule.Type());
    int method = METHOD_UNKNOWN;
    MythRecordingRule handle = node->m_rule.DuplicateRecordingRule();

    // Rule update method depends of current rule type:
    // - Updating override rule is limited.
    // - Enabled repeating rule must to be overriden.
    // - All others could be fully updated until it is recording.
    switch (node->m_rule.Type())
    {
    case MythRecordingRule::RT_DontRecord:
    case MythRecordingRule::RT_NotRecording:
    case MythRecordingRule::RT_TemplateRecord:
      // Deny update
      method = METHOD_UNKNOWN;
      break;
    case MythRecordingRule::RT_AllRecord:
    case MythRecordingRule::RT_ChannelRecord:
    case MythRecordingRule::RT_OneRecord:
    case MythRecordingRule::RT_FindDailyRecord:
    case MythRecordingRule::RT_FindWeeklyRecord:
    case MythRecordingRule::RT_DailyRecord:
    case MythRecordingRule::RT_WeeklyRecord:
      // When inactive we can replace with the new rule
      if (handle.Inactive())
      {
        method = METHOD_FULL_UPDATE;
      }
      // When active we create override rule
      else
      {
        // Only priority can be overriden
        if (newrule.Priority() != handle.Priority())
        {
          handle.SetPriority(newrule.Priority());
          method = METHOD_CREATE_OVERRIDE;
        }
        else
          method = METHOD_UNKNOWN;
      }
      break;
    case MythRecordingRule::RT_OverrideRecord:
      // Only priority can be overriden
      handle.SetPriority(newrule.Priority());
      method = METHOD_DISCREET_UPDATE;
        break;
      case MythRecordingRule::RT_SingleRecord:
        if (recording->Status() == RS_RECORDING || recording->Status() == RS_TUNING)
        {
          // Discreet update
          handle.SetEndTime(newrule.EndTime());
          handle.SetEndOffset(newrule.EndOffset());
          method = METHOD_DISCREET_UPDATE;
        }
        else
        {
          method = METHOD_FULL_UPDATE;
        }
        break;
      default:
        break;
    }

    if (method == METHOD_UNKNOWN)
    {
      return MSM_ERROR_NOT_IMPLEMENTED;
    }
    if (method == METHOD_DISCREET_UPDATE)
    {
      if (!m_db.UpdateRecordingRule(handle))
        return MSM_ERROR_FAILED;
      node->m_rule = handle; // sync node
      if (!m_con.UpdateSchedules(handle.RecordID()))
        return MSM_ERROR_FAILED;
      return MSM_ERROR_SUCCESS;
    }
    if (method == METHOD_CREATE_OVERRIDE)
    {
      handle.SetRecordID(0);
      handle.SetInactive(false);
      handle.SetType(MythRecordingRule::RT_OverrideRecord);
      handle.SetStartTime(recording->StartTime());
      handle.SetEndTime(recording->EndTime());
      handle.SetParentID(node->m_rule.RecordID());
      if (!m_db.AddRecordingRule(handle))
        return MSM_ERROR_FAILED;
      node->m_overrideRules.push_back(handle); // sync node
      if (!m_con.UpdateSchedules(handle.RecordID()))
        return MSM_ERROR_FAILED;
      return MSM_ERROR_SUCCESS;
    }
    if (method == METHOD_FULL_UPDATE)
    {
      handle = newrule;
      handle.SetRecordID(node->m_rule.RecordID());
      if (!m_db.UpdateRecordingRule(handle))
        return MSM_ERROR_FAILED;
      node->m_rule = handle; // sync node
      if (!m_con.UpdateSchedules(handle.RecordID()))
        return MSM_ERROR_FAILED;
      return MSM_ERROR_SUCCESS;
    }
  }

  return MSM_ERROR_NOT_IMPLEMENTED;
}

boost::shared_ptr<MythRecordingRuleNode> MythScheduleManager::FindRuleById(unsigned int recordID) const
{
  CLockObject lock(m_lock);

  NodeById::const_iterator it = m_rulesById.find(recordID);
  if (it != m_rulesById.end())
    return it->second;
  return boost::shared_ptr<MythRecordingRuleNode>();
}

ScheduleList MythScheduleManager::FindUpComingByRuleId(unsigned int recordID) const
{
  CLockObject lock(m_lock);

  ScheduleList found;
  std::pair<RecordingIndexByRuleId::const_iterator, RecordingIndexByRuleId::const_iterator> range = m_recordingIndexByRuleId.equal_range(recordID);
  if (range.first != m_recordingIndexByRuleId.end())
  {
    for (RecordingIndexByRuleId::const_iterator it = range.first; it != range.second; ++it)
    {
      RecordingList::const_iterator recordingIt = m_recordings.find(it->second);
      if (recordingIt != m_recordings.end())
        found.push_back(std::make_pair(it->second, recordingIt->second));
    }
  }
  return found;
}

boost::shared_ptr<MythProgramInfo> MythScheduleManager::FindUpComingByIndex(unsigned int index) const
{
  CLockObject lock(m_lock);

  RecordingList::const_iterator it = m_recordings.find(index);
  if (it != m_recordings.end())
    return it->second;
  else
    return boost::shared_ptr<MythProgramInfo>();
}

void MythScheduleManager::Update()
{
  CLockObject lock(m_lock);

  // Setup VersionHelper for the new set
  this->Setup();
  RecordingRuleMap rules = m_db.GetRecordingRules();
  m_rules.clear();
  m_rulesById.clear();
  for (RecordingRuleMap::iterator it = rules.begin(); it != rules.end(); ++it)
  {
    boost::shared_ptr<MythRecordingRuleNode> node = boost::shared_ptr<MythRecordingRuleNode>(new MythRecordingRuleNode(it->second));
    m_rules.push_back(node);
    m_rulesById.insert(NodeById::value_type(it->second.RecordID(), node));
  }

  for (NodeList::iterator it = m_rules.begin(); it != m_rules.end(); ++it)
    // Is override rule ? Then find main rule and link to it
    if ((*it)->IsOverrideRule())
    {
      // First check parentid. Then fallback searching the same timeslot
      NodeById::iterator itp = m_rulesById.find((*it)->m_rule.ParentID());
      if (itp != m_rulesById.end())
      {
        itp->second->m_overrideRules.push_back((*it)->m_rule);
        (*it)->m_mainRule = itp->second->m_rule;
      }
      else
      {
        for (NodeList::iterator itm = m_rules.begin(); itm != m_rules.end(); ++itm)
          if (!(*itm)->IsOverrideRule() && m_versionHelper->SameTimeslot((*it)->m_rule, (*itm)->m_rule))
          {
            (*itm)->m_overrideRules.push_back((*it)->m_rule);
            (*it)->m_mainRule = (*itm)->m_rule;
          }

      }
    }

  // Add pending programs to upcoming recordings
  ProgramInfoMap pending = m_con.GetPendingPrograms();
  m_recordings.clear();
  m_recordingIndexByRuleId.clear();
  for (ProgramInfoMap::iterator it = pending.begin(); it != pending.end(); ++it)
  {
    boost::shared_ptr<MythProgramInfo> rec = boost::shared_ptr<MythProgramInfo>(new MythProgramInfo());
    *rec = it->second;
    unsigned int index = MakeIndex(it->second);
    m_recordings.insert(RecordingList::value_type(index, rec));
    m_recordingIndexByRuleId.insert(std::make_pair(it->second.RecordID(), index));
  }

  // Add missed programs (NOT RECORDING) to upcoming recordings. User could delete them as needed.
  if (m_showNotRecording)
  {
    ProgramInfoMap schedule = m_con.GetScheduledPrograms();
    for (ProgramInfoMap::iterator it = schedule.begin(); it != schedule.end(); ++it)
    {
      if (m_recordingIndexByRuleId.count(it->second.RecordID()) == 0)
      {
        NodeById::const_iterator itr = m_rulesById.find(it->second.RecordID());
        if (itr != m_rulesById.end() && !itr->second->HasOverrideRules())
        {
          boost::shared_ptr<MythProgramInfo> rec = boost::shared_ptr<MythProgramInfo>(new MythProgramInfo());
          *rec = it->second;
          unsigned int index = MakeIndex(it->second);
          m_recordings.insert(RecordingList::value_type(index, rec));
          m_recordingIndexByRuleId.insert(std::make_pair(it->second.RecordID(), index));
        }
      }
    }
  }

  if (g_bExtraDebug)
  {
    for (NodeList::iterator it = m_rules.begin(); it != m_rules.end(); ++it)
      XBMC->Log(LOG_DEBUG, "%s - Rule node - recordid: %u, parentid: %u, type: %d, overriden: %s", __FUNCTION__, (*it)->m_rule.RecordID(), (*it)->m_rule.ParentID(), (*it)->m_rule.Type(), ((*it)->HasOverrideRules() ? "Yes" : "No"));
    for (RecordingList::iterator it = m_recordings.begin(); it != m_recordings.end(); ++it)
      XBMC->Log(LOG_DEBUG, "%s - Recording - recordid: %u, index: %d, status: %d, title: %s", __FUNCTION__, it->second->RecordID(), it->first, it->second->Status(), it->second->Title().c_str());
  }
}

RuleMetadata MythScheduleManager::GetMetadata(const MythRecordingRule &rule) const
{
  return m_versionHelper->GetMetadata(rule);
}

MythRecordingRule MythScheduleManager::NewFromTemplate(MythEPGInfo &epgInfo)
{
  return m_versionHelper->NewFromTemplate(epgInfo);
}

MythRecordingRule MythScheduleManager::NewSingleRecord(MythEPGInfo &epgInfo)
{
  return m_versionHelper->NewSingleRecord(epgInfo);
}

MythRecordingRule MythScheduleManager::NewDailyRecord(MythEPGInfo &epgInfo)
{
  return m_versionHelper->NewDailyRecord(epgInfo);
}

MythRecordingRule MythScheduleManager::NewWeeklyRecord(MythEPGInfo &epgInfo)
{
  return m_versionHelper->NewWeeklyRecord(epgInfo);
}

MythRecordingRule MythScheduleManager::NewChannelRecord(MythEPGInfo &epgInfo)
{
  return m_versionHelper->NewChannelRecord(epgInfo);
}

MythRecordingRule MythScheduleManager::NewOneRecord(MythEPGInfo &epgInfo)
{
  return m_versionHelper->NewOneRecord(epgInfo);
}

bool MythScheduleManager::ToggleShowNotRecording()
{
  m_showNotRecording ^= true;
  return m_showNotRecording;
}

///////////////////////////////////////////////////////////////////////////////
////
//// Version Helper for unknown version (no helper)
////

bool MythScheduleHelperNoHelper::SameTimeslot(MythRecordingRule &first, MythRecordingRule &second) const
{
  (void)first;
  (void)second;
  return false;
}

RuleMetadata MythScheduleHelperNoHelper::GetMetadata(const MythRecordingRule &rule) const
{
  RuleMetadata meta;
  (void)rule;
  meta.isRepeating = false;
  meta.weekDays = 0;
  meta.marker = "";
  return meta;
}

MythRecordingRule MythScheduleHelperNoHelper::NewFromTemplate(MythEPGInfo &epgInfo)
{
  (void)epgInfo;
  return MythRecordingRule();
}

MythRecordingRule MythScheduleHelperNoHelper::NewSingleRecord(MythEPGInfo &epgInfo)
{
  (void)epgInfo;
  return MythRecordingRule();
}

MythRecordingRule MythScheduleHelperNoHelper::NewDailyRecord(MythEPGInfo &epgInfo)
{
  (void)epgInfo;
  return MythRecordingRule();
}

MythRecordingRule MythScheduleHelperNoHelper::NewWeeklyRecord(MythEPGInfo &epgInfo)
{
  (void)epgInfo;
  return MythRecordingRule();
}

MythRecordingRule MythScheduleHelperNoHelper::NewChannelRecord(MythEPGInfo &epgInfo)
{
  (void)epgInfo;
  return MythRecordingRule();
}

MythRecordingRule MythScheduleHelperNoHelper::NewOneRecord(MythEPGInfo &epgInfo)
{
  (void)epgInfo;
  return MythRecordingRule();
}

///////////////////////////////////////////////////////////////////////////////
////
//// Version Helper for database up to 1226 (0.24))
////

bool MythScheduleHelper1226::SameTimeslot(MythRecordingRule &first, MythRecordingRule &second) const
{
  time_t first_st = first.StartTime();
  time_t second_st = second.StartTime();

  switch (first.Type())
  {
  case MythRecordingRule::RT_NotRecording:
  case MythRecordingRule::RT_SingleRecord:
  case MythRecordingRule::RT_OverrideRecord:
  case MythRecordingRule::RT_DontRecord:
    return
    second_st == first_st &&
            second.EndTime() == first.EndTime() &&
            second.ChannelID() == first.ChannelID();

  case MythRecordingRule::RT_OneRecord: // FindOneRecord
    return
    second.Title() == first.Title() &&
            second.ChannelID() == first.ChannelID();

  case MythRecordingRule::RT_DailyRecord: // TimeslotRecord
    return
    second.Title() == first.Title() &&
            daytime(&first_st) == daytime(&second_st) &&
            second.ChannelID() == first.ChannelID();

  case MythRecordingRule::RT_WeeklyRecord: // WeekslotRecord
    return
    second.Title() == first.Title() &&
            daytime(&first_st) == daytime(&second_st) &&
            weekday(&first_st) == weekday(&second_st) &&
            second.ChannelID() == first.ChannelID();

  case MythRecordingRule::RT_FindDailyRecord:
    return
    second.Title() == first.Title() &&
            second.ChannelID() == first.ChannelID();

  case MythRecordingRule::RT_FindWeeklyRecord:
    return
    second.Title() == first.Title() &&
            weekday(&first_st) == weekday(&second_st) &&
            second.ChannelID() == first.ChannelID();

  case MythRecordingRule::RT_ChannelRecord:
    return
    second.Title() == first.Title() &&
            second.ChannelID() == first.ChannelID();

  case MythRecordingRule::RT_AllRecord:
    return
    second.Title() == first.Title();

  default:
    break;
  }
  return false;
}

RuleMetadata MythScheduleHelper1226::GetMetadata(const MythRecordingRule &rule) const
{
  RuleMetadata meta;
  time_t st = rule.StartTime();
  meta.isRepeating = false;
  meta.weekDays = 0;
  meta.marker = "";
  switch (rule.Type())
  {
    case MythRecordingRule::RT_DailyRecord:
    case MythRecordingRule::RT_FindDailyRecord:
      meta.isRepeating = true;
      meta.weekDays = 0x7F;
      meta.marker = "d";
      break;
    case MythRecordingRule::RT_WeeklyRecord:
    case MythRecordingRule::RT_FindWeeklyRecord:
      meta.isRepeating = true;
      meta.weekDays = 1 << ((weekday(&st) + 6) % 7);
      meta.marker = "w";
      break;
    case MythRecordingRule::RT_ChannelRecord:
      meta.isRepeating = true;
      meta.weekDays = 0x7F;
      meta.marker = "C";
      break;
    case MythRecordingRule::RT_AllRecord:
      meta.isRepeating = true;
      meta.weekDays = 0x7F;
      meta.marker = "A";
      break;
    case MythRecordingRule::RT_OneRecord:
      meta.isRepeating = false;
      meta.weekDays = 0;
      meta.marker = "1";
      break;
    case MythRecordingRule::RT_DontRecord:
      meta.isRepeating = false;
      meta.weekDays = 0;
      meta.marker = "x";
      break;
    case MythRecordingRule::RT_OverrideRecord:
      meta.isRepeating = false;
      meta.weekDays = 0;
      meta.marker = "o";
      break;
    default:
      break;
  }
  return meta;
}

MythRecordingRule MythScheduleHelper1226::NewFromTemplate(MythEPGInfo &epgInfo)
{
  MythRecordingRule rule;
  // Load rule template from selected provider
  switch (g_iRecTemplateType)
  {
  case 1: // Template provider is 'MythTV', then load the template from backend.
    rule = m_db.LoadRecordingRuleTemplate("", "");
    break;
  case 0: // Template provider is 'Internal', then set rule with settings
    rule.SetAutoCommFlag(g_bRecAutoCommFlag);
    rule.SetAutoTranscode(g_bRecAutoTranscode);
    rule.SetUserJob(1, g_bRecAutoRunJob1);
    rule.SetUserJob(2, g_bRecAutoRunJob2);
    rule.SetUserJob(3, g_bRecAutoRunJob3);
    rule.SetUserJob(4, g_bRecAutoRunJob4);
    rule.SetAutoExpire(g_bRecAutoExpire);
    rule.SetTranscoder(g_iRecTranscoder);
  }

  // Category override
  if (!epgInfo.IsNull())
  {
    CStdString overTimeCategory = m_db.GetSetting("OverTimeCategory");
    if (!overTimeCategory.IsEmpty() && (overTimeCategory.Equals(epgInfo.Category()) || overTimeCategory.Equals(epgInfo.CategoryType())))
    {
      CStdString categoryOverTime = m_db.GetSetting("CategoryOverTime");
      XBMC->Log(LOG_DEBUG, "Overriding end offset for category %s: +%s", overTimeCategory.c_str(), categoryOverTime.c_str());
      rule.SetEndOffset(atoi(categoryOverTime));
    }
  }
  return rule;
}

MythRecordingRule MythScheduleHelper1226::NewSingleRecord(MythEPGInfo &epgInfo)
{
  MythRecordingRule rule = this->NewFromTemplate(epgInfo);

  rule.SetType(MythRecordingRule::RT_SingleRecord);

  if (!epgInfo.IsNull())
  {
    rule.SetChannelID(epgInfo.ChannelID());
    rule.SetStartTime(epgInfo.StartTime());
    rule.SetEndTime(epgInfo.EndTime());
    rule.SetSearchType(MythRecordingRule::ST_NoSearch);
    rule.SetTitle(epgInfo.Title());
    rule.SetSubtitle(epgInfo.Subtitle());
    rule.SetCategory(epgInfo.Category());
    rule.SetDescription(epgInfo.Description());
    rule.SetCallsign(epgInfo.Callsign());
    rule.SetProgramID(epgInfo.ProgramID());
    rule.SetSeriesID(epgInfo.SeriesID());
  }
  else
  {
    // kManualSearch = http://www.gossamer-threads.com/lists/mythtv/dev/155150?search_string=kManualSearch;#155150
    rule.SetSearchType(MythRecordingRule::ST_ManualSearch);
  }
  rule.SetDuplicateControlMethod(MythRecordingRule::DM_CheckNone);
  rule.SetCheckDuplicatesInType(MythRecordingRule::DI_InAll);
  rule.SetInactive(false);
  return rule;
}

MythRecordingRule MythScheduleHelper1226::NewDailyRecord(MythEPGInfo &epgInfo)
{
  MythRecordingRule rule = this->NewFromTemplate(epgInfo);

  rule.SetType(MythRecordingRule::RT_DailyRecord);

  if (!epgInfo.IsNull())
  {
    rule.SetSearchType(MythRecordingRule::ST_NoSearch);
    rule.SetChannelID(epgInfo.ChannelID());
    rule.SetStartTime(epgInfo.StartTime());
    rule.SetEndTime(epgInfo.EndTime());
    rule.SetTitle(epgInfo.Title());
    rule.SetSubtitle(epgInfo.Subtitle());
    rule.SetCategory(epgInfo.Category());
    rule.SetDescription(epgInfo.Description());
    rule.SetCallsign(epgInfo.Callsign());
    rule.SetProgramID(epgInfo.ProgramID());
    rule.SetSeriesID(epgInfo.SeriesID());
  }
  else
  {
    // kManualSearch = http://www.gossamer-threads.com/lists/mythtv/dev/155150?search_string=kManualSearch;#155150
    rule.SetSearchType(MythRecordingRule::ST_ManualSearch);
  }
  rule.SetDuplicateControlMethod(MythRecordingRule::DM_CheckSubtitleAndDescription);
  rule.SetCheckDuplicatesInType(MythRecordingRule::DI_InAll);
  rule.SetInactive(false);
  return rule;
}

MythRecordingRule MythScheduleHelper1226::NewWeeklyRecord(MythEPGInfo &epgInfo)
{
  MythRecordingRule rule = this->NewFromTemplate(epgInfo);

  rule.SetType(MythRecordingRule::RT_WeeklyRecord);

  if (!epgInfo.IsNull())
  {
    rule.SetSearchType(MythRecordingRule::ST_NoSearch);
    rule.SetChannelID(epgInfo.ChannelID());
    rule.SetStartTime(epgInfo.StartTime());
    rule.SetEndTime(epgInfo.EndTime());
    rule.SetTitle(epgInfo.Title());
    rule.SetSubtitle(epgInfo.Subtitle());
    rule.SetCategory(epgInfo.Category());
    rule.SetDescription(epgInfo.Description());
    rule.SetCallsign(epgInfo.Callsign());
    rule.SetProgramID(epgInfo.ProgramID());
    rule.SetSeriesID(epgInfo.SeriesID());
  }
  else
  {
    // kManualSearch = http://www.gossamer-threads.com/lists/mythtv/dev/155150?search_string=kManualSearch;#155150
    rule.SetSearchType(MythRecordingRule::ST_ManualSearch);
  }
  rule.SetDuplicateControlMethod(MythRecordingRule::DM_CheckSubtitleAndDescription);
  rule.SetCheckDuplicatesInType(MythRecordingRule::DI_InAll);
  rule.SetInactive(false);
  return rule;
}

MythRecordingRule MythScheduleHelper1226::NewChannelRecord(MythEPGInfo &epgInfo)
{
  MythRecordingRule rule = this->NewFromTemplate(epgInfo);

  rule.SetType(MythRecordingRule::RT_ChannelRecord);

  if (!epgInfo.IsNull())
  {
    rule.SetSearchType(MythRecordingRule::ST_TitleSearch);
    rule.SetChannelID(epgInfo.ChannelID());
    rule.SetStartTime(epgInfo.StartTime());
    rule.SetEndTime(epgInfo.EndTime());
    rule.SetTitle(epgInfo.Title());
    // Backend use the description to find program by keywords or title
    rule.SetSubtitle("");
    rule.SetDescription(epgInfo.Title());
    rule.SetCategory(epgInfo.Category());
    rule.SetCallsign(epgInfo.Callsign());
    rule.SetProgramID(epgInfo.ProgramID());
    rule.SetSeriesID(epgInfo.SeriesID());
  }
  else
  {
    // Not feasible
    rule.SetType(MythRecordingRule::RT_NotRecording);
  }
  rule.SetDuplicateControlMethod(MythRecordingRule::DM_CheckSubtitleAndDescription);
  rule.SetCheckDuplicatesInType(MythRecordingRule::DI_InAll);
  rule.SetInactive(false);
  return rule;
}

MythRecordingRule MythScheduleHelper1226::NewOneRecord(MythEPGInfo &epgInfo)
{
  MythRecordingRule rule = this->NewFromTemplate(epgInfo);

  rule.SetType(MythRecordingRule::RT_OneRecord);

  if (!epgInfo.IsNull())
  {
    rule.SetSearchType(MythRecordingRule::ST_TitleSearch);
    rule.SetChannelID(epgInfo.ChannelID());
    rule.SetStartTime(epgInfo.StartTime());
    rule.SetEndTime(epgInfo.EndTime());
    rule.SetTitle(epgInfo.Title());
    // Backend use the description to find program by keywords or title
    rule.SetSubtitle("");
    rule.SetDescription(epgInfo.Title());
    rule.SetCategory(epgInfo.Category());
    rule.SetCallsign(epgInfo.Callsign());
    rule.SetProgramID(epgInfo.ProgramID());
    rule.SetSeriesID(epgInfo.SeriesID());
  }
  else
  {
    // Not feasible
    rule.SetType(MythRecordingRule::RT_NotRecording);
  }
  rule.SetDuplicateControlMethod(MythRecordingRule::DM_CheckSubtitleAndDescription);
  rule.SetCheckDuplicatesInType(MythRecordingRule::DI_InAll);
  rule.SetInactive(false);
  return rule;
}

///////////////////////////////////////////////////////////////////////////////
////
//// Version Helper for database up to 1278 (0.25)
////
//// New filter mask (1276)
//// New autometadata flag (1278)
////

bool MythScheduleHelper1278::SameTimeslot(MythRecordingRule &first, MythRecordingRule &second) const
{
  time_t first_st = first.StartTime();
  time_t second_st = second.StartTime();

  switch (first.Type())
  {
  case MythRecordingRule::RT_NotRecording:
  case MythRecordingRule::RT_SingleRecord:
  case MythRecordingRule::RT_OverrideRecord:
  case MythRecordingRule::RT_DontRecord:
    return
    second_st == first_st &&
            second.EndTime() == first.EndTime() &&
            second.ChannelID() == first.ChannelID() &&
            second.Filter() == first.Filter();

  case MythRecordingRule::RT_OneRecord: // FindOneRecord
    return
    second.Title() == first.Title() &&
            second.ChannelID() == first.ChannelID() &&
            second.Filter() == first.Filter();

  case MythRecordingRule::RT_DailyRecord: // TimeslotRecord
    return
    second.Title() == first.Title() &&
            daytime(&first_st) == daytime(&second_st) &&
            second.ChannelID() == first.ChannelID() &&
            second.Filter() == first.Filter();

  case MythRecordingRule::RT_WeeklyRecord: // WeekslotRecord
    return
    second.Title() == first.Title() &&
            daytime(&first_st) == daytime(&second_st) &&
            weekday(&first_st) == weekday(&second_st) &&
            second.ChannelID() == first.ChannelID() &&
            second.Filter() == first.Filter();

  case MythRecordingRule::RT_FindDailyRecord:
    return
    second.Title() == first.Title() &&
            second.ChannelID() == first.ChannelID() &&
            second.Filter() == first.Filter();

  case MythRecordingRule::RT_FindWeeklyRecord:
    return
    second.Title() == first.Title() &&
            weekday(&first_st) == weekday(&second_st) &&
            second.ChannelID() == first.ChannelID() &&
            second.Filter() == first.Filter();

  case MythRecordingRule::RT_ChannelRecord:
    return
    second.Title() == first.Title() &&
            second.ChannelID() == first.ChannelID() &&
            second.Filter() == first.Filter();

  case MythRecordingRule::RT_AllRecord:
    return
    second.Title() == first.Title() &&
            second.Filter() == first.Filter();

  default:
    break;
  }
  return false;
}

MythRecordingRule MythScheduleHelper1278::NewFromTemplate(MythEPGInfo &epgInfo)
{
  MythRecordingRule rule;
  // Load rule template from selected provider
  switch (g_iRecTemplateType)
  {
    case 1: // Template provider is 'MythTV', then load the template from backend.
      rule = m_db.LoadRecordingRuleTemplate("", "");
      break;
    case 0: // Template provider is 'Internal', then set rule with settings
      rule.SetAutoCommFlag(g_bRecAutoCommFlag);
      rule.SetAutoMetadata(g_bRecAutoMetadata);
      rule.SetAutoTranscode(g_bRecAutoTranscode);
      rule.SetUserJob(1, g_bRecAutoRunJob1);
      rule.SetUserJob(2, g_bRecAutoRunJob2);
      rule.SetUserJob(3, g_bRecAutoRunJob3);
      rule.SetUserJob(4, g_bRecAutoRunJob4);
      rule.SetAutoExpire(g_bRecAutoExpire);
      rule.SetTranscoder(g_iRecTranscoder);
  }

  // Category override
  if (!epgInfo.IsNull())
  {
    CStdString overTimeCategory = m_db.GetSetting("OverTimeCategory");
    if (!overTimeCategory.IsEmpty() && (overTimeCategory.Equals(epgInfo.Category()) || overTimeCategory.Equals(epgInfo.CategoryType())))
    {
      CStdString categoryOverTime = m_db.GetSetting("CategoryOverTime");
      XBMC->Log(LOG_DEBUG, "Overriding end offset for category %s: +%s", overTimeCategory.c_str(), categoryOverTime.c_str());
      rule.SetEndOffset(atoi(categoryOverTime));
    }
  }
  return rule;
}

///////////////////////////////////////////////////////////////////////////////
////
//// Version helper for database up to 1302 (0.26)
////
//// TemplateRecord replaces static settings
////

MythRecordingRule MythScheduleHelper1302::NewFromTemplate(MythEPGInfo &epgInfo)
{
  MythRecordingRule rule;
  // Load rule template from selected provider
  switch (g_iRecTemplateType)
  {
  case 1: // Template provider is 'MythTV', then load the template from backend.
    if (!epgInfo.IsNull())
      rule = m_db.LoadRecordingRuleTemplate(epgInfo.Category(), epgInfo.CategoryType());
    else
      rule = m_db.LoadRecordingRuleTemplate("", "");
    break;
  case 0: // Template provider is 'Internal', then set rule with settings
    rule.SetAutoCommFlag(g_bRecAutoCommFlag);
    rule.SetAutoMetadata(g_bRecAutoMetadata);
    rule.SetAutoTranscode(g_bRecAutoTranscode);
    rule.SetUserJob(1, g_bRecAutoRunJob1);
    rule.SetUserJob(2, g_bRecAutoRunJob2);
    rule.SetUserJob(3, g_bRecAutoRunJob3);
    rule.SetUserJob(4, g_bRecAutoRunJob4);
    rule.SetAutoExpire(g_bRecAutoExpire);
    rule.SetTranscoder(g_iRecTranscoder);
  }

  // Category override
  if (!epgInfo.IsNull())
  {
    CStdString overTimeCategory = m_db.GetSetting("OverTimeCategory");
    if (!overTimeCategory.IsEmpty() && (overTimeCategory.Equals(epgInfo.Category()) || overTimeCategory.Equals(epgInfo.CategoryType())))
    {
      CStdString categoryOverTime = m_db.GetSetting("CategoryOverTime");
      XBMC->Log(LOG_DEBUG, "Overriding end offset for category %s: +%s", overTimeCategory.c_str(), categoryOverTime.c_str());
      rule.SetEndOffset(atoi(categoryOverTime));
    }
  }
  return rule;
}

///////////////////////////////////////////////////////////////////////////////
////
//// Version helper for database up to 1309 (0.27)
////
//// Remove the Timeslot and Weekslot recording rule types. These rule
//// types are too rigid and don't work when a broadcaster shifts the
//// starting time of a program by a few minutes. Users should now use
//// Channel recording rules in place of Timeslot and Weekslot rules. To
//// approximate the old functionality, two new schedule filters have been
//// added. In addition, the new "This time" and "This day and time"
//// filters are less strict and match any program starting within 10
//// minutes of the recording rule time.
//// Restrict the use of the FindDaily? and FindWeekly? recording rule types
//// (now simply called Daily and Weekly) to search and manual recording
//// rules. These rule types are rarely needed and limiting their use to
//// the most powerful cases simplifies the user interface for the more
//// common cases. Users should now use Daily and Weekly, custom search
//// rules in place of FindDaily? and FindWeekly? rules.
//// Any existing recording rules using the no longer supported or allowed
//// types are automatically converted to the suggested alternatives.
////

RuleMetadata MythScheduleHelper1309::GetMetadata(const MythRecordingRule &rule) const
{
  RuleMetadata meta;
  time_t st = rule.StartTime();
  meta.isRepeating = false;
  meta.weekDays = 0;
  meta.marker = "";
  switch (rule.Type())
  {
    case MythRecordingRule::RT_DailyRecord:
    case MythRecordingRule::RT_FindDailyRecord:
      meta.isRepeating = true;
      meta.weekDays = 0x7F;
      meta.marker = "d";
      break;
    case MythRecordingRule::RT_WeeklyRecord:
    case MythRecordingRule::RT_FindWeeklyRecord:
      meta.isRepeating = true;
      meta.weekDays = 1 << ((weekday(&st) + 6) % 7);
      meta.marker = "w";
      break;
    case MythRecordingRule::RT_ChannelRecord:
      meta.isRepeating = true;
      meta.weekDays = 0x7F;
      meta.marker = "C";
      break;
    case MythRecordingRule::RT_AllRecord:
      meta.isRepeating = true;
      if ((rule.Filter() & MythRecordingRule::FM_ThisDayAndTime))
      {
        meta.weekDays = 1 << ((weekday(&st) + 6) % 7);
        meta.marker = "w";
      }
      else if ((rule.Filter() & MythRecordingRule::FM_ThisTime))
      {
        meta.weekDays = 0x7F;
        meta.marker = "d";
      }
      else
      {
        meta.weekDays = 0x7F;
        meta.marker = "A";
      }
      break;
    case MythRecordingRule::RT_OneRecord:
      meta.isRepeating = false;
      meta.weekDays = 0;
      meta.marker = "1";
      break;
    case MythRecordingRule::RT_DontRecord:
      meta.isRepeating = false;
      meta.weekDays = 0;
      meta.marker = "x";
      break;
    case MythRecordingRule::RT_OverrideRecord:
      meta.isRepeating = false;
      meta.weekDays = 0;
      meta.marker = "o";
      break;
    default:
      break;
  }
  return meta;
}

MythRecordingRule MythScheduleHelper1309::NewDailyRecord(MythEPGInfo &epgInfo)
{
  unsigned int filter;
  MythRecordingRule rule = this->NewFromTemplate(epgInfo);

  rule.SetType(MythRecordingRule::RT_AllRecord);
  filter = MythRecordingRule::FM_ThisChannel + MythRecordingRule::FM_ThisTime;
  rule.SetFilter(filter);

  if (!epgInfo.IsNull())
  {
    rule.SetSearchType(MythRecordingRule::ST_NoSearch);
    rule.SetChannelID(epgInfo.ChannelID());
    rule.SetStartTime(epgInfo.StartTime());
    rule.SetEndTime(epgInfo.EndTime());
    rule.SetTitle(epgInfo.Title());
    rule.SetSubtitle(epgInfo.Subtitle());
    rule.SetCategory(epgInfo.Category());
    rule.SetDescription(epgInfo.Description());
    rule.SetCallsign(epgInfo.Callsign());
    rule.SetProgramID(epgInfo.ProgramID());
    rule.SetSeriesID(epgInfo.SeriesID());
  }
  else
  {
    // No EPG! Create custom daily for this channel
    rule.SetType(MythRecordingRule::RT_DailyRecord);
    rule.SetFilter(MythRecordingRule::FM_ThisChannel);
    // kManualSearch = http://www.gossamer-threads.com/lists/mythtv/dev/155150?search_string=kManualSearch;#155150
    rule.SetSearchType(MythRecordingRule::ST_ManualSearch);
  }
  rule.SetDuplicateControlMethod(MythRecordingRule::DM_CheckSubtitleAndDescription);
  rule.SetCheckDuplicatesInType(MythRecordingRule::DI_InAll);
  rule.SetInactive(false);
  return rule;
}

MythRecordingRule MythScheduleHelper1309::NewWeeklyRecord(MythEPGInfo &epgInfo)
{
  unsigned int filter;
  MythRecordingRule rule = this->NewFromTemplate(epgInfo);

  rule.SetType(MythRecordingRule::RT_AllRecord);
  filter = MythRecordingRule::FM_ThisChannel + MythRecordingRule::FM_ThisDayAndTime;
  rule.SetFilter(filter);

  if (!epgInfo.IsNull())
  {
    rule.SetSearchType(MythRecordingRule::ST_NoSearch);
    rule.SetChannelID(epgInfo.ChannelID());
    rule.SetStartTime(epgInfo.StartTime());
    rule.SetEndTime(epgInfo.EndTime());
    rule.SetTitle(epgInfo.Title());
    rule.SetSubtitle(epgInfo.Subtitle());
    rule.SetCategory(epgInfo.Category());
    rule.SetDescription(epgInfo.Description());
    rule.SetCallsign(epgInfo.Callsign());
    rule.SetProgramID(epgInfo.ProgramID());
    rule.SetSeriesID(epgInfo.SeriesID());
  }
  else
  {
    // No EPG! Create custom weekly for this channel
    rule.SetType(MythRecordingRule::RT_WeeklyRecord);
    rule.SetFilter(MythRecordingRule::FM_ThisChannel);
    // kManualSearch = http://www.gossamer-threads.com/lists/mythtv/dev/155150?search_string=kManualSearch;#155150
    rule.SetSearchType(MythRecordingRule::ST_ManualSearch);
  }
  rule.SetDuplicateControlMethod(MythRecordingRule::DM_CheckSubtitleAndDescription);
  rule.SetCheckDuplicatesInType(MythRecordingRule::DI_InAll);
  rule.SetInactive(false);
  return rule;
}

MythRecordingRule MythScheduleHelper1309::NewChannelRecord(MythEPGInfo &epgInfo)
{
  unsigned int filter;
  MythRecordingRule rule = this->NewFromTemplate(epgInfo);

  rule.SetType(MythRecordingRule::RT_AllRecord);
  filter = MythRecordingRule::FM_ThisChannel;
  rule.SetFilter(filter);

  if (!epgInfo.IsNull())
  {
    rule.SetSearchType(MythRecordingRule::ST_NoSearch);
    rule.SetChannelID(epgInfo.ChannelID());
    rule.SetStartTime(epgInfo.StartTime());
    rule.SetEndTime(epgInfo.EndTime());
    rule.SetTitle(epgInfo.Title());
    rule.SetSubtitle(epgInfo.Subtitle());
    rule.SetCategory(epgInfo.Category());
    rule.SetDescription(epgInfo.Description());
    rule.SetCallsign(epgInfo.Callsign());
    rule.SetProgramID(epgInfo.ProgramID());
    rule.SetSeriesID(epgInfo.SeriesID());
  }
  else
  {
    // Not feasible
    rule.SetType(MythRecordingRule::RT_NotRecording);
  }
  rule.SetDuplicateControlMethod(MythRecordingRule::DM_CheckSubtitleAndDescription);
  rule.SetCheckDuplicatesInType(MythRecordingRule::DI_InAll);
  rule.SetInactive(false);
  return rule;
}

MythRecordingRule MythScheduleHelper1309::NewOneRecord(MythEPGInfo &epgInfo)
{
  unsigned int filter;
  MythRecordingRule rule = this->NewFromTemplate(epgInfo);

  rule.SetType(MythRecordingRule::RT_OneRecord);
  filter = MythRecordingRule::FM_ThisEpisode;
  rule.SetFilter(filter);

  if (!epgInfo.IsNull())
  {
    rule.SetSearchType(MythRecordingRule::ST_NoSearch);
    rule.SetChannelID(epgInfo.ChannelID());
    rule.SetStartTime(epgInfo.StartTime());
    rule.SetEndTime(epgInfo.EndTime());
    rule.SetTitle(epgInfo.Title());
    rule.SetSubtitle(epgInfo.Subtitle());
    rule.SetCategory(epgInfo.Category());
    rule.SetDescription(epgInfo.Description());
    rule.SetCallsign(epgInfo.Callsign());
    rule.SetProgramID(epgInfo.ProgramID());
    rule.SetSeriesID(epgInfo.SeriesID());
  }
  else
  {
    // Not feasible
    rule.SetType(MythRecordingRule::RT_NotRecording);
  }
  rule.SetDuplicateControlMethod(MythRecordingRule::DM_CheckSubtitleAndDescription);
  rule.SetCheckDuplicatesInType(MythRecordingRule::DI_InAll);
  rule.SetInactive(false);
  return rule;
}
