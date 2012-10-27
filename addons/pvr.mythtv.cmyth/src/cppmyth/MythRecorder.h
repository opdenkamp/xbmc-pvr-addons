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

#include "MythConnection.h"

#include "platform/util/StdString.h"

#include <boost/shared_ptr.hpp>

extern "C" {
#include <cmyth/cmyth.h>
};

class MythProgramInfo;
class MythChannel;

template <class T> class MythPointer;

class MythRecorder
{
public:
  MythRecorder();
  MythRecorder(cmyth_recorder_t cmyth_recorder, const MythConnection &conn);

  bool IsNull() const;
  int ID();
  MythProgramInfo GetCurrentProgram();

  bool IsRecording();
  bool IsTunable(MythChannel &channel);
  bool CheckChannel(MythChannel &channel);

  bool SpawnLiveTV(MythChannel &channel);
  bool SetChannel(MythChannel &channel);
  bool LiveTVWatch(const CStdString &msg);
  bool LiveTVDoneRecording(const CStdString &msg);
  bool LiveTVChainUpdate(const CStdString &chainid);
  int ReadLiveTV(void *buffer, unsigned long length);
  long long LiveTVSeek(long long offset, int whence);
  long long LiveTVDuration();

  bool Stop();

private:
  boost::shared_ptr<MythPointerThreadSafe<cmyth_recorder_t> > m_recorder_t;
  boost::shared_ptr<int> m_liveChainUpdated;
  MythConnection m_conn;

  static void prog_update_callback(cmyth_proginfo_t prog);
};
