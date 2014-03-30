#pragma once

#ifndef PVR_DVBVIEWER_TIMESHIFTBUFFER_H
#define PVR_DVBVIEWER_TIMESHIFTBUFFER_H

#include "platform/util/StdString.h"
#include "platform/threads/threads.h"

#define STREAM_READ_BUFFER_SIZE   8192
#define BUFFER_READ_TIMEOUT       10000
#define BUFFER_READ_WAITTIME      50

class TimeshiftBuffer
  : public PLATFORM::CThread
{
public:
  TimeshiftBuffer(CStdString streamPath, CStdString bufferPath);
  ~TimeshiftBuffer(void);
  int ReadData(unsigned char *buffer, unsigned int size);
  bool IsValid();
  long long Seek(long long position, int whence);
  long long Position();
  long long Length();
  void Stop(void);

private:
  virtual void *Process(void);

  CStdString m_bufferPath;
  void *m_streamHandle;
  void *m_filebufferReadHandle;
  void *m_filebufferWriteHandle;
  bool m_shifting;
};

#endif
