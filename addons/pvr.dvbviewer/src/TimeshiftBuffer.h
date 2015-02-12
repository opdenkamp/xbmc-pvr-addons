#pragma once

#ifndef PVR_DVBVIEWER_TIMESHIFTBUFFER_H
#define PVR_DVBVIEWER_TIMESHIFTBUFFER_H

#include "platform/util/StdString.h"
#include "platform/threads/threads.h"

class TimeshiftBuffer
  : public PLATFORM::CThread
{
public:
  TimeshiftBuffer(CStdString streamUrl, CStdString bufferPath);
  ~TimeshiftBuffer(void);
  bool IsValid();
  int ReadData(unsigned char *buffer, unsigned int size);
  long long Seek(long long position, int whence);
  long long Position();
  long long Length();
  void Stop(void);
  time_t TimeStart();
  time_t TimeEnd();

private:
  virtual void *Process(void);

  CStdString m_bufferPath;
  void *m_streamHandle;
  void *m_filebufferReadHandle;
  void *m_filebufferWriteHandle;
  time_t m_start;
#ifndef TARGET_POSIX
  PLATFORM::CMutex m_mutex;
  uint64_t m_writePos;
#endif
};

#endif
