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

  static CStdString MakeUID(int chanid, time_t recstart)
  {
    // Creates unique IDs from ChannelID and StartTime like "100_2011-12-10T12:00:00"
    char buf[37] = "";
    MythTimestamp time(recstart);
    sprintf(buf, "%d_%s", chanid, time.String().c_str());
    return CStdString(buf);
  }

  CStdString StrUID();
  long long UID();
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

  int ChannelID();
  CStdString ChannelName();

  RecordStatus Status();
  CStdString RecordingGroup();
  unsigned long RecordID();
  time_t RecordingStartTime();
  time_t RecordingEndTime();
  int Priority();

  void SetFramerate(const long long framerate);
  long long Framterate() const;

  CStdString IconPath();
  CStdString Coverart() const;
  CStdString Fanart() const;

private:
  boost::shared_ptr<MythPointer<cmyth_proginfo_t> > m_proginfo_t;

  // Cached PVR attributes
  long long m_framerate;

  // Artworks
  CStdString m_coverart;
  CStdString m_fanart;
};
