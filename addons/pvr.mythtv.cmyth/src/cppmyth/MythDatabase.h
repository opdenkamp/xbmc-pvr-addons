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

#include <vector>
#include <map>

#include <boost/shared_ptr.hpp>

extern "C" {
#include <cmyth/cmyth.h>
};

class MythChannel;
class MythEPGInfo;
class MythRecordingRule;
class MythProgramInfo;

template <class T> class MythPointerThreadSafe;

typedef std::map<time_t, MythEPGInfo> EPGInfoMap;

typedef std::map<int, MythChannel> ChannelIdMap;
typedef std::multimap<CStdString, MythChannel> ChannelNumberMap;
typedef std::map<CStdString, std::vector<int> > ChannelGroupMap;

typedef std::vector<std::pair<unsigned int, unsigned int> > RecorderSourceList;

typedef std::map<int, MythRecordingRule> RecordingRuleMap;

// TODO: Rework MythRecordingProfile
class MythRecordingProfile : public CStdString
{
public:
  std::map<int, CStdString> profile;
};

typedef std::vector<MythRecordingProfile> RecordingProfileList;

class MythDatabase
{
public:
  MythDatabase();
  MythDatabase(const CStdString &server, const CStdString &database, const CStdString &user, const CStdString &password, unsigned short port);

  bool IsNull() const;

  bool TestConnection(CStdString *msg);

  int GetSchemaVersion();

  CStdString GetSetting(const CStdString &setting);

  bool FindProgram(time_t starttime, int channelid, const CStdString &title, MythEPGInfo &epgInfo);
  bool FindCurrentProgram(time_t attime, int channelid, MythEPGInfo &epgInfo);
  EPGInfoMap GetGuide(int channelid, time_t starttime, time_t endtime);

  ChannelIdMap GetChannels();
  ChannelGroupMap GetChannelGroups();

  RecorderSourceList GetLiveTVRecorderSourceList(const CStdString &channum);

  RecordingRuleMap GetRecordingRules();
  bool AddRecordingRule(const MythRecordingRule &rule);
  bool UpdateRecordingRule(const MythRecordingRule &rule);
  bool DeleteRecordingRule(const MythRecordingRule &rule);
  MythRecordingRule LoadRecordingRuleTemplate(const CStdString &category, const CStdString &category_type);

  RecordingProfileList GetRecordingProfiles();

  bool SetWatchedStatus(const MythProgramInfo &recording, bool watched);
  long long GetBookmarkMark(const MythProgramInfo &recording, long long bk, int mode);

  long long GetRecordingMarkup(const MythProgramInfo &recording, cmyth_recording_markup_t type);
  long long GetRecordingFrameRate(const MythProgramInfo &recording);
  int GetRecordingSeekOffset(const MythProgramInfo &recording, cmyth_recording_markup_t type, int64_t mark, int64_t *psoffset, int64_t *nsoffset);

  bool FillRecordingArtwork(MythProgramInfo &recording);
  bool KeepLiveTVRecording(MythProgramInfo &recording, bool keep);

private:
  boost::shared_ptr<MythPointerThreadSafe<cmyth_database_t> > m_database_t;
};
