#pragma once

#include "../libcmyth.h"
#include "platform/util/StdString.h"
#include <boost/shared_ptr.hpp>
#include "MythPointer.h"
#include "MythFile.h"

class MythRecorder;
class MythSignal;

class MythEventHandler 
{
public:
  MythEventHandler();
  MythEventHandler(CStdString, unsigned short port);
  void SetRecorder(MythRecorder &rec);
  MythSignal GetSignal();
  void Stop();
  void PreventLiveChainUpdate();
  void AllowLiveChainUpdate();
  void SetRecordingListener(MythFile &file, CStdString recId);
  bool TryReconnect();
private:
  class ImpMythEventHandler;
  boost::shared_ptr< ImpMythEventHandler > m_imp;
  int m_retry_count;
};
