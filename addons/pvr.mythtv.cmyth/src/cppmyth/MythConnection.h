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
class MythTimer;
class MythStorageGroupFile;

template <class T> class MythPointer;
template <class T> class MythPointerThreadSafe;

typedef std::map<CStdString, MythProgramInfo> ProgramInfoMap;
typedef std::vector<MythStorageGroupFile> StorageGroupFileList;

class MythConnection 
{
public:
  MythConnection();
  MythConnection(const CStdString &server, unsigned short port);

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
  CStdString GetSetting(const CStdString &hostname, const CStdString &setting);
  bool SetSetting(const CStdString &hostname, const CStdString &setting, const CStdString &value);

  // Recorders
  MythRecorder GetFreeRecorder();
  MythRecorder GetRecorder(int n);

  // Recordings
  bool DeleteRecording(MythProgramInfo &recording);
  ProgramInfoMap GetRecordedPrograms();

  // Timers
  ProgramInfoMap GetPendingPrograms();
  ProgramInfoMap GetScheduledPrograms();
  bool UpdateSchedules(int id);
  void DefaultTimer(MythTimer &timer);

  // Files
  MythFile ConnectFile(MythProgramInfo &recording);
  MythFile ConnectPath(const CStdString &filename, const CStdString &storageGroup);
  StorageGroupFileList GetStorageGroupFileList(const CStdString &storageGroup);

  // Bookmarks
  long long GetBookmark(MythProgramInfo &recording);
  int SetBookmark(MythProgramInfo &recording, long long bookmark);

private:
  boost::shared_ptr<MythPointerThreadSafe<cmyth_conn_t> > m_conn_t;
  CStdString m_server;
  unsigned short m_port;
  int m_retryCount;

  MythEventHandler *m_pEventHandler;
};
