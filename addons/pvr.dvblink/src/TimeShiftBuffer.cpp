#include "TimeShiftBuffer.h"


TimeShiftBuffer::TimeShiftBuffer(CHelper_libXBMC_addon  * XBMC, std::string streampath, std::string bufferpath)
{
  this->XBMC = XBMC;
  m_bufferPath = bufferpath;
  m_streamHandle = XBMC->OpenFile(streampath.c_str(), 0);
  m_filebufferWriteHandle = XBMC->OpenFileForWrite(m_bufferPath.c_str(),true);
  Sleep(100);
  m_filebufferReadHandle = XBMC->OpenFile(m_bufferPath.c_str(), 0);
  m_shifting = true;
  CreateThread();
}


bool TimeShiftBuffer::IsValid()
{
  return (m_streamHandle != NULL);
}

void TimeShiftBuffer::Stop()
{
  m_shifting = false;
}

TimeShiftBuffer::~TimeShiftBuffer(void)
{
  Stop();
  if (IsRunning())
  {
    StopThread();
  }

  if (m_filebufferWriteHandle)
  {
    XBMC->CloseFile(m_filebufferWriteHandle);
  }
  if (m_filebufferReadHandle)
  {
    XBMC->CloseFile(m_filebufferReadHandle);
  }
  if (m_streamHandle)
  {
    XBMC->CloseFile(m_streamHandle);
  }
}

void *TimeShiftBuffer::Process()
{
  XBMC->Log(LOG_DEBUG, "TimeShiftProcess:: thread started");
  byte buffer[STREAM_READ_BUFFER_SIZE];
  int bytesRead = STREAM_READ_BUFFER_SIZE;

  while (m_shifting)
  {
    bytesRead = XBMC->ReadFile(m_streamHandle, buffer, sizeof(buffer));
    XBMC->WriteFile(m_filebufferWriteHandle,buffer,bytesRead);
  }
  XBMC->Log(LOG_DEBUG, "TimeShiftProcess:: thread stopped");
  return NULL;
}


long long TimeShiftBuffer::Seek(long long iPosition, int iWhence)
{
  if (m_filebufferReadHandle)
    return XBMC->SeekFile(m_filebufferReadHandle,iPosition,iWhence);
  return 0;
}

long long TimeShiftBuffer::Position()
{
  if (m_filebufferReadHandle)
    return XBMC->GetFilePosition(m_filebufferReadHandle);
  return 0;
}

long long TimeShiftBuffer::Length()
{
  if (m_filebufferReadHandle)
    return XBMC->GetFileLength(m_filebufferReadHandle);
  return 0;
}

int TimeShiftBuffer::ReadData(unsigned char *pBuffer, unsigned int iBufferSize)
{
  unsigned int totalReadBytes = 0;
  unsigned int totalTimeWaited = 0;
  if (m_filebufferReadHandle)
  {
    unsigned int read = XBMC->ReadFile(m_filebufferReadHandle, pBuffer,iBufferSize);
    totalReadBytes += read;

    while (read < iBufferSize && totalTimeWaited < BUFFER_READ_TIMEOUT)
    {
      Sleep(BUFFER_READ_WAITTIME);
      totalTimeWaited += BUFFER_READ_WAITTIME;
      read = XBMC->ReadFile(m_filebufferReadHandle, pBuffer,iBufferSize - totalReadBytes);
      totalReadBytes += read;
    }

    if(totalTimeWaited > BUFFER_READ_TIMEOUT)
    {
      XBMC->Log(LOG_DEBUG, "Timeshifterbuffer timed out, waited : %d", totalTimeWaited);
    }
  }
  return totalReadBytes;
}
