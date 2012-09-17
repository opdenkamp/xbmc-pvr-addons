#pragma once

#include "libcmyth.h"
#include "utils/StdString.h"
#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>
#include "MythPointer.h"

class MythRecorder;
class MythFile;
class MythProgramInfo;
class MythEventHandler;
class MythTimer;
class MythSGFile;


class MythConnection 
{
public:
  MythConnection();
  MythConnection(CStdString server,unsigned short port);
  MythRecorder GetFreeRecorder();
  MythRecorder GetRecorder(int n);
  MythEventHandler * CreateEventHandler();
  boost::unordered_map< CStdString, MythProgramInfo > GetRecordedPrograms();
  boost::unordered_map< CStdString, MythProgramInfo > GetPendingPrograms();
  boost::unordered_map< CStdString, MythProgramInfo > GetScheduledPrograms();
  bool DeleteRecording(MythProgramInfo &recording);
  bool IsConnected();
  bool TryReconnect();
  CStdString GetServer();
  int GetProtocolVersion();
  bool GetDriveSpace(long long &total,long long &used);
  bool UpdateSchedules(int id);
  MythFile ConnectFile(MythProgramInfo &recording);
  bool IsNull();
  void Lock();
  void Unlock();
  CStdString GetSetting(CStdString hostname,CStdString setting);
  bool SetSetting(CStdString hostname,CStdString setting,CStdString value);
  CStdString GetHostname();
  CStdString GetBackendHostname();
  void DefaultTimer(MythTimer &timer);
  MythFile ConnectPath(CStdString filename, CStdString storageGroup);
  std::vector< CStdString > GetStorageGroupFileList_(CStdString sgGetList);
  std::vector< MythSGFile > GetStorageGroupFileList(CStdString storagegroup);
  
private:
  boost::shared_ptr< MythPointerThreadSafe< cmyth_conn_t > > m_conn_t;
  CStdString m_server;
  unsigned short m_port;
  int m_retry_count;
  static MythEventHandler * m_pEventHandler;
};
