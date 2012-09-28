#pragma once

#include "platform/util/StdString.h"
#include <boost/shared_ptr.hpp>
#include "MythPointer.h"

extern "C" {
#include <cmyth/cmyth.h>
};

class MythProgramInfo 
{
  friend class MythConnection;
  friend class MythDatabase;

public:
  typedef cmyth_proginfo_rec_status_t record_status;

  MythProgramInfo();
  MythProgramInfo(cmyth_proginfo_t cmyth_proginfo);
  CStdString ProgramID();
  CStdString Title(bool subtitleEncoded);
  CStdString Subtitle();
  CStdString Path();
  CStdString Description();
  CStdString ChannelName();
  int ChannelID();
  unsigned long RecordID();
  time_t RecStart();
  time_t StartTime();
  time_t EndTime();
  int Priority();
  record_status Status();
  int Duration();
  CStdString Category();
  CStdString RecordingGroup();
  bool IsWatched();
  bool IsDeletePending();
  long long uid();
  bool IsNull();
private:
  boost::shared_ptr< MythPointer< cmyth_proginfo_t > > m_proginfo_t;
};
