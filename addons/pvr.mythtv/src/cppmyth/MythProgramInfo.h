#pragma once
/*
 *      Copyright (C) 2005-2014 Team XBMC
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

#include <mythtypes.h>

class MythProgramInfo;
typedef std::map<std::string, MythProgramInfo> ProgramInfoMap;

class MythProgramInfo
{
public:
  typedef Myth::RS_t RecordStatus;

  MythProgramInfo();
  MythProgramInfo(Myth::ProgramPtr proginfo);

  bool IsNull() const;
  Myth::ProgramPtr GetPtr() const;
  bool operator ==(const MythProgramInfo &other);
  bool operator !=(const MythProgramInfo &other);

  std::string UID() const;
  std::string ProgramID() const;
  std::string Title() const;
  std::string Subtitle() const;
  std::string HostName() const;
  std::string FileName() const;
  std::string Description() const;
  int Duration() const;
  std::string Category() const;
  time_t StartTime() const;
  time_t EndTime() const;
  bool IsWatched() const;
  bool IsDeletePending() const;
  bool HasBookmark() const;
  bool IsVisible() const;
  bool IsLiveTV() const;

  uint32_t ChannelID() const;
  std::string ChannelName() const;

  RecordStatus Status() const;
  std::string RecordingGroup() const;
  uint32_t RecordID() const;
  time_t RecordingStartTime() const;
  time_t RecordingEndTime() const;
  int Priority() const;
  std::string StorageGroup() const;

  std::string Inetref() const;
  uint16_t Season() const;
  bool HasCoverart() const;
  bool HasFanart() const;

private:
  Myth::ProgramPtr m_proginfo;
  mutable int32_t m_flags;

  bool IsSetup() const;
};
