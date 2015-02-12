#include "TimeshiftBuffer.h"
#include "client.h"
#include "platform/util/util.h"

#define STREAM_READ_BUFFER_SIZE   32768
#define BUFFER_READ_TIMEOUT       10000
#define BUFFER_READ_WAITTIME      50

using namespace ADDON;

TimeshiftBuffer::TimeshiftBuffer(CStdString streamUrl, CStdString bufferPath)
  : m_bufferPath(bufferPath)
{
  m_streamHandle = XBMC->OpenFile(streamUrl, READ_NO_CACHE);
  m_bufferPath += "/tsbuffer.ts";
  m_filebufferWriteHandle = XBMC->OpenFileForWrite(m_bufferPath, true);
#ifndef TARGET_POSIX
  m_writePos = 0;
#endif
  Sleep(100);
  m_filebufferReadHandle = XBMC->OpenFile(m_bufferPath, READ_NO_CACHE);
  m_start = time(NULL);
  CreateThread();
}

TimeshiftBuffer::~TimeshiftBuffer(void)
{
  Stop();
  if (IsRunning())
    StopThread();

  if (m_filebufferWriteHandle)
    XBMC->CloseFile(m_filebufferWriteHandle);
  if (m_filebufferReadHandle)
    XBMC->CloseFile(m_filebufferReadHandle);
  if (m_streamHandle)
    XBMC->CloseFile(m_streamHandle);
}

bool TimeshiftBuffer::IsValid()
{
  return (m_streamHandle != NULL
      && m_filebufferWriteHandle != NULL
      && m_filebufferReadHandle != NULL);
}

void TimeshiftBuffer::Stop()
{
  m_start = 0;
}

void *TimeshiftBuffer::Process()
{
  XBMC->Log(LOG_DEBUG, "Timeshift: thread started");
  byte buffer[STREAM_READ_BUFFER_SIZE];

  while (m_start)
  {
    unsigned int read = XBMC->ReadFile(m_streamHandle, buffer, sizeof(buffer));
    XBMC->WriteFile(m_filebufferWriteHandle, buffer, read);

#ifndef TARGET_POSIX
    m_mutex.Lock();
    m_writePos += read;
    m_mutex.Unlock();
#endif
  }
  XBMC->Log(LOG_DEBUG, "Timeshift: thread stopped");
  return NULL;
}

long long TimeshiftBuffer::Seek(long long position, int whence)
{
  return XBMC->SeekFile(m_filebufferReadHandle, position, whence);
}

long long TimeshiftBuffer::Position()
{
  return XBMC->GetFilePosition(m_filebufferReadHandle);
}

long long TimeshiftBuffer::Length()
{
  // We can't use GetFileLength here as it's value will be cached
  // by XBMC until we read or seek above it.
  // see xbm/xbmc/filesystem/HDFile.cpp CHDFile::GetLength()
  //return XBMC->GetFileLength(m_filebufferReadHandle);

  int64_t writePos = 0;
#ifdef TARGET_POSIX
  /* refresh write position */
  XBMC->SeekFile(m_filebufferWriteHandle, 0L, SEEK_CUR);
  writePos = XBMC->GetFilePosition(m_filebufferWriteHandle);
#else
  m_mutex.Lock();
  writePos = m_writePos;
  m_mutex.Unlock();
#endif
  return writePos;
}

int TimeshiftBuffer::ReadData(unsigned char *buffer, unsigned int size)
{
  /* make sure we never read above the current write position */
  int64_t readPos = XBMC->GetFilePosition(m_filebufferReadHandle);
  unsigned int timeWaited = 0;
  while (readPos + size > Length())
  {
    if (timeWaited > BUFFER_READ_TIMEOUT)
    {
      XBMC->Log(LOG_DEBUG, "Timeshift: Read timed out; waited %u", timeWaited);
      return -1;
    }
    Sleep(BUFFER_READ_WAITTIME);
    timeWaited += BUFFER_READ_WAITTIME;
  }

  return XBMC->ReadFile(m_filebufferReadHandle, buffer, size);
}

time_t TimeshiftBuffer::TimeStart()
{
  return m_start;
}

time_t TimeshiftBuffer::TimeEnd()
{
  return (m_start) ? time(NULL) : 0;
}
