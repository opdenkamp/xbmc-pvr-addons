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
class MythTimer;
class MythProgramInfo;

template <class T> class MythPointerThreadSafe;

typedef cmyth_program_t MythProgram;
typedef std::vector<MythProgram> ProgramList;

typedef std::map<int, MythChannel> ChannelMap;
typedef std::pair<CStdString, std::vector<int> > MythChannelGroup;
typedef std::map<CStdString, std::vector<int> > ChannelGroupMap;

typedef std::map<int, std::vector<int> > SourceMap;
typedef std::map<int, MythTimer> TimerMap;

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
  MythDatabase(const CStdString &server, const CStdString &database, const CStdString &user, const CStdString &password);

  bool IsNull() const;

  bool TestConnection(CStdString *msg);

  bool FindProgram(time_t starttime, int channelid, const CStdString &title, MythProgram* pprogram);
  ProgramList GetGuide(time_t starttime, time_t endtime);

  ChannelMap GetChannels();
  ChannelGroupMap GetChannelGroups();
  SourceMap GetSources();

  TimerMap GetTimers();
  int AddTimer(const MythTimer &timer);
  bool UpdateTimer(const MythTimer &timer);
  bool DeleteTimer(int recordid);

  RecordingProfileList GetRecordingProfiles();

  int SetWatchedStatus(const MythProgramInfo &recording, bool watched);
  long long GetBookmarkMark(const MythProgramInfo &recording, long long bk, int mode);

private:
  boost::shared_ptr<MythPointerThreadSafe<cmyth_database_t> > m_database_t;
};
