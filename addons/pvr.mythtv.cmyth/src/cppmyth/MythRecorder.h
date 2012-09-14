#pragma once

#include "../libcmyth.h"
#include "platform/util/StdString.h"
#include <boost/shared_ptr.hpp>
#include "MythPointer.h"
#include "MythConnection.h"
#include "../../../../lib/platform/threads/threads.h"

class MythProgramInfo;
class MythChannel;
  
class MythRecorder 
{
public:
  MythRecorder();
  MythRecorder(cmyth_recorder_t cmyth_recorder,MythConnection conn);
  bool SpawnLiveTV(MythChannel &channel);
  bool LiveTVChainUpdate(CStdString chainID);
  bool IsNull();
  bool IsRecording();
  bool IsTunable(MythChannel &channel);
  int ID();
  bool CheckChannel(MythChannel &channel);
  bool SetChannel(MythChannel &channel);
  int ReadLiveTV(void* buffer,long long length);
  MythProgramInfo GetCurrentProgram();
  long long LiveTVSeek(long long offset, int whence);
  long long LiveTVDuration();
  bool Stop();
private:
  boost::shared_ptr< MythPointerThreadSafe< cmyth_recorder_t > > m_recorder_t;
  static void prog_update_callback(cmyth_proginfo_t prog);
  boost::shared_ptr< int > livechainupdated;
  MythConnection m_conn;
};