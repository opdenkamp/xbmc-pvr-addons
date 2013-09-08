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

#include "MythTimestamp.h"

#include "platform/util/StdString.h"

#include <boost/shared_ptr.hpp>

extern "C" {
#include <cmyth/cmyth.h>
};

template <class T> class MythPointer;

class MythProgramInfo
{
public:
  friend class MythConnection;
  friend class MythDatabase;
  typedef cmyth_proginfo_rec_status_t RecordStatus;

  MythProgramInfo();
  MythProgramInfo(cmyth_proginfo_t cmyth_proginfo);

  bool IsNull() const;

  CStdString UID();
  CStdString ProgramID();
  CStdString Title();
  CStdString Subtitle();
  CStdString BaseName();
  CStdString Description();
  int Duration();
  CStdString Category();
  time_t StartTime();
  time_t EndTime();
  bool IsWatched();
  bool IsDeletePending();
  bool HasBookmark();
  bool IsVisible();

  unsigned int ChannelID();
  CStdString ChannelName();

  RecordStatus Status();
  CStdString RecordingGroup();
  unsigned int RecordID();
  time_t RecordingStartTime();
  time_t RecordingEndTime();
  int Priority();
  CStdString StorageGroup();

  void SetFrameRate(const long long frameRate);
  long long FrameRate() const;

  CStdString IconPath();
  CStdString Coverart() const;
  CStdString Fanart() const;

private:
  boost::shared_ptr<MythPointer<cmyth_proginfo_t> > m_proginfo_t;

  // Cached PVR attributes
  long long m_frameRate;

  // Artworks
  CStdString m_coverart;
  CStdString m_fanart;
};
