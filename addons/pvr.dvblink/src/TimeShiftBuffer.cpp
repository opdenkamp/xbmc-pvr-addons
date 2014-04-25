/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://xbmc.org
 
 *      Copyright (C) 2012 Palle Ehmsen(Barcode Madness)
 *      http://www.barcodemadness.com
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
 

#include "TimeShiftBuffer.h"
using namespace ADDON;

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
  if (XBMC->FileExists(m_bufferPath.c_str(), true))
  {
    XBMC->DeleteFile(m_bufferPath.c_str());
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
