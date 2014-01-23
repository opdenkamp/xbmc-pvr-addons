#pragma once

#include "libXBMC_addon.h"
#include "platform/util/StdString.h"
#include "platform/threads/threads.h"
#include "platform/util/util.h"

using namespace ADDON;

#define STREAM_READ_BUFFER_SIZE   8192
#define BUFFER_READ_TIMEOUT       10000
#define BUFFER_READ_WAITTIME      50

class TimeshiftBuffer : public PLATFORM::CThread
{
public:
  TimeshiftBuffer(std::string streampath, std::string bufferpath);
  ~TimeshiftBuffer(void);
  int ReadData(unsigned char *pBuffer, unsigned int iBufferSize);
  bool IsValid();
  long long Seek(long long iPosition, int iWhence);
  long long Position();
  long long Length();
  void Stop(void);

private:
  virtual void *Process(void);

  std::string m_bufferPath;
  void *m_streamHandle;
  void *m_filebufferReadHandle;
  void *m_filebufferWriteHandle;
  bool m_shifting;
};
