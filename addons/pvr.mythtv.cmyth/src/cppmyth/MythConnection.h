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

#include <map>

#include <boost/shared_ptr.hpp>

extern "C" {
#include <cmyth/cmyth.h>
};

class MythRecorder;
class MythFile;
class MythProgramInfo;
class MythEventHandler;
class MythRecordingRule;
class MythStorageGroupFile;
class MythTimestamp;

template <class T> class MythPointer;
template <class T> class MythPointerThreadSafe;

typedef std::map<CStdString, MythProgramInfo> ProgramInfoMap;

typedef cmyth_commbreak MythEdlEntry;
typedef std::vector<MythEdlEntry> Edl;

class MythConnection
{
public:
  MythConnection();
  MythConnection(const CStdString &server, unsigned short port, bool playback);

  MythEventHandler *CreateEventHandler();

  bool IsNull() const;
  void Lock();
  void Unlock();

  // Server
  bool IsConnected();
  bool TryReconnect();
  CStdString GetBackendName() const;
  CStdString GetHostname();
  CStdString GetBackendHostname();
  int GetProtocolVersion();
  bool GetDriveSpace(long long &total, long long &used);
  CStdString GetSettingOnHost(const CStdString &setting, const CStdString &hostname);
  bool SetSetting(const CStdString &hostname, const CStdString &setting, const CStdString &value);

  // Recorders
  MythRecorder GetFreeRecorder();
  MythRecorder GetRecorder(int n);

  // Recordings
  bool DeleteRecording(MythProgramInfo &recording);
  bool DeleteAndForgetRecording(MythProgramInfo &recording);
  ProgramInfoMap GetRecordedPrograms();
  MythProgramInfo GetRecordedProgram(const CStdString &basename);
  MythProgramInfo GetRecordedProgram(int chanid, const MythTimestamp &recstartts);

  // Timers
  ProgramInfoMap GetPendingPrograms();
  ProgramInfoMap GetScheduledPrograms();
  bool UpdateSchedules(int id);
  bool StopRecording(const MythProgramInfo &recording);

  // Files
  MythFile ConnectFile(MythProgramInfo &recording);
  MythFile ConnectPath(const CStdString &filename, const CStdString &storageGroup);
  MythStorageGroupFile GetStorageGroupFile(const CStdString &storageGroup, const CStdString &filename);

  // Bookmarks
  long long GetBookmark(MythProgramInfo &recording);
  bool SetBookmark(MythProgramInfo &recording, long long bookmark);

  // Edl
  Edl GetCutList(MythProgramInfo &recording);
  Edl GetCommbreakList(MythProgramInfo &recording);

private:
  boost::shared_ptr<MythPointerThreadSafe<cmyth_conn_t> > m_conn_t;
  CStdString m_server;
  unsigned short m_port;
  bool m_playback;

  MythEventHandler *m_pEventHandler;
};
