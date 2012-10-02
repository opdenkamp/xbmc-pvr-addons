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

#include <boost/unordered_map.hpp>
#include <boost/shared_ptr.hpp>

extern "C" {
#include <cmyth/cmyth.h>
};

class MythChannel;
class MythTimer;
class MythProgramInfo;

typedef cmyth_program_t MythProgram;

template <class T> class MythPointerThreadSafe;
typedef std::pair<CStdString, std::vector<int> > MythChannelGroup;
typedef std::vector<MythChannel> MythChannelList;

// TODO: Rework MythRecordingProfile
class MythRecordingProfile : public CStdString
{
public:
  std::map<int, CStdString> profile;
};

class MythDatabase
{
public:
  MythDatabase();
  MythDatabase(const CStdString &server, const CStdString &database, const CStdString &user, const CStdString &password);

  bool IsNull() const;

  bool TestConnection(CStdString *msg);

  bool FindProgram(time_t starttime, int channelid, const CStdString &title, MythProgram* pprogram);
  std::vector<MythProgram> GetGuide(time_t starttime, time_t endtime);

  std::map<int , MythChannel> ChannelList();
  boost::unordered_map<CStdString, std::vector<int> > GetChannelGroups();
  std::map<int, std::vector<int> > SourceList();

  std::map<int, MythTimer> GetTimers();
  int AddTimer(const MythTimer &timer);
  bool UpdateTimer(const MythTimer &timer);
  bool DeleteTimer(int recordid);

  std::vector<MythRecordingProfile > GetRecordingProfiles();

  int SetWatchedStatus(const MythProgramInfo &recording, bool watched);
  long long GetBookmarkMark(const MythProgramInfo &recording, long long bk, int mode);

private:
  boost::shared_ptr<MythPointerThreadSafe<cmyth_database_t> > m_database_t;
};
