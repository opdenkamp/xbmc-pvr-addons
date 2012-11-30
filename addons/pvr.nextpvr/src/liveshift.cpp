/*
 *      Copyright (C) 2005-2011 Team XBMC
 *      http://www.xbmc.org
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
#include "liveshift.h"
#include "platform/util/timeutils.h"

const int MAX_WINDOW_SIZE = 6;
const int STARTUP_CACHE_SIZE = 5000000;

LiveShiftSource::LiveShiftSource(NextPVR::Socket *pSocket)
{
  m_requestNumber = 0;
  m_position = 0;
  m_currentWindowSize = 0;
  m_lastKnownLength = (188*4000); // some smallish non-zero fake size until we receive a real stream length
  m_pSocket = pSocket;
  m_doingStartup = true;

  m_startCacheBytes = 0;
  m_startupCache = new unsigned char[STARTUP_CACHE_SIZE];

  m_log = NULL;

  // protocol logging code I need to enable sometimes for debugging
  if (m_log == NULL)
  {
#if defined(TARGET_WINDOWS)
    bool doProtocolLogging = false;
    if (doProtocolLogging)
    {
      m_log = fopen("liveshift.log", "wt");
    }
#else
    m_log = NULL;
#endif
  }

  // pre-request some blocks to satisfy ffmpeg avformat_find_stream_info stage
  int length = 32768;
  for (int i=0; i<75; i++)
  {
    long long blockOffset = 0 + (i * length);
    char request[48];
    memset(request, 0, sizeof(request));
    snprintf(request, sizeof(request), "Range: bytes=%llu-%llu-%d", blockOffset, (blockOffset+length), m_requestNumber);
    LOG("sending request: %s\n", request);
    if (m_pSocket->send(request, sizeof(request)) != sizeof(request))
    {
      LOG("NOT ALL BYTES SENT!");
    }

    m_requestNumber++;
    m_currentWindowSize++;
  }
}


LiveShiftSource::~LiveShiftSource(void)
{
  if (m_log != NULL)
  {
    fclose(m_log);
    m_log = NULL;
  }

  if (m_startupCache != NULL)
  {
    delete []m_startupCache;
    m_startupCache = NULL;
  }
}

void LiveShiftSource::LOG(char const *fmt, ... )
{
  if (m_log != NULL)
  {
#if defined(TARGET_WINDOWS)
    // determine the current local time
    SYSTEMTIME utcSystemTime;
    GetSystemTime(&utcSystemTime);
    SYSTEMTIME systemTime;
    SystemTimeToTzSpecificLocalTime(NULL, &utcSystemTime, &systemTime);
    
    // log nicely formatted log message
    fprintf(m_log, "%02d:%02d:%02d.%03d\t", systemTime.wHour, systemTime.wMinute, systemTime.wSecond, systemTime.wMilliseconds);

    // log passed-in string
    va_list ap; 
    va_start (ap, fmt); 
    vfprintf(m_log, fmt, ap); 
    va_end(ap); 

    fflush(m_log);
#else
    // log passed-in string
    va_list ap; 
    va_start (ap, fmt); 
    vfprintf(m_log, fmt, ap); 
    va_end(ap); 

    fflush(m_log);
#endif
  }
} 


unsigned int LiveShiftSource::Read(unsigned char *buffer, unsigned int length)
{
  LOG("LiveShiftSource::Read(%d bytes from %llu)\n", length, m_position);

  int bytesRead = 0;

  // can it be read from the cache?
  if (m_startupCache != NULL && ((m_position + length) < m_startCacheBytes))
  {
    LOG("LiveShiftSource::Read()@exit, returning %d bytes from cache\n", m_startCacheBytes);
    memcpy(buffer, &m_startupCache[m_position], length);
    m_position += length;

    // we'll need to later request more data
    m_currentWindowSize = 0;

    return length;
  }


  // send read request (using a basic sliding window protocol)
  if (m_currentWindowSize < 0)
    m_currentWindowSize = 0;

  for (int i=m_currentWindowSize; i<MAX_WINDOW_SIZE; i++)
  {
    long long blockOffset = m_position + (i * length);
    char request[48];
    memset(request, 0, sizeof(request));
    snprintf(request, sizeof(request), "Range: bytes=%llu-%llu-%d", blockOffset, (blockOffset+length), m_requestNumber);
    LOG("sending request: %s\n", request);
    int sent = m_pSocket->send(request, sizeof(request));
    if (sent != sizeof(request))
    {
      LOG("NOT ALL BYTES SENT! Only sent %d bytes\n", sent);
      return -1;
    }

    m_requestNumber++;
    m_currentWindowSize++;
  }

  // read the response, usually the first one
  int read_timeouts = 0;
  LOG("about to wait for block with offset: %llu\n", m_position);
  while (true)
  {
    if (!m_pSocket->is_valid())
    {
      LOG("about to call receive(), socket is invalid\n");
      return -1;
    }

    if (m_pSocket->read_ready())
    {
      // read response header
      char response[128];
      memset(response, 0, sizeof(response));
      int responseByteCount = m_pSocket->receive(response, sizeof(response), sizeof(response));
      if (responseByteCount > 0)
      {
        LOG("got: %s\n", response);
      }

      // drop out if response header looks incorrect
      if (responseByteCount != sizeof(response))
      {
        return -1;
      }

      // parse response header
      long long payloadOffset;
      int payloadSize;
      long long fileSize;
      int dummy;
      sscanf(response, "%llu:%d %llu %d", &payloadOffset, &payloadSize, &fileSize, &dummy);
      m_lastKnownLength = fileSize;

      // read response payload
      bytesRead = m_pSocket->receive((char *)buffer, length, payloadSize); 

      // if it's from the start of the file, then cache it
      if (m_startupCache != NULL && ((payloadOffset + payloadSize) < STARTUP_CACHE_SIZE))
      {
        memcpy(&m_startupCache[payloadOffset], buffer, payloadSize);
        if ((payloadOffset + payloadSize) > m_startCacheBytes)
        {
          m_startCacheBytes = payloadOffset + payloadSize;
        }
      }

      // was this the packet we were interested in?
      if (payloadOffset == m_position)
      {
        // yep, hit - update info
        m_position += payloadSize;
        LOG("read block:  %llu:%d %llu\n", payloadOffset, payloadSize, fileSize);

        // read one response
        m_currentWindowSize--;
        break;
      }
      else
      {
        // no, miss
        LOG("read block:  %llu:%d %llu  (not the one we want.... (offset==%llu))\n", payloadOffset, payloadSize, fileSize, m_position);
      }
    }
    else
    {
      usleep(50000);
      read_timeouts++;

      // is it taking too long?
      if (read_timeouts > 100)
      {
        LOG("closing socket after 100 timeouts (m_currentWindowSize=%d)\n", m_currentWindowSize);
        m_currentWindowSize = 0;
        m_pSocket->close();
        return -1;
      }

    }
  }

  LOG("LiveShiftSource::Read()@exit\n");
  return bytesRead;
}

long long LiveShiftSource::GetLength()
{
  LOG("LiveShiftSource::GetLength() returning %llu\n", m_lastKnownLength);
  return m_lastKnownLength;
}

long long LiveShiftSource::GetPosition()
{
  LOG("LiveShiftSource::GetPosition() returning %llu\n", m_position);
  return m_position;
}

void LiveShiftSource::Seek(long long offset)
{
  LOG("LiveShiftSource::Seek(%llu)\n", offset);
  m_position = offset;

  if ((m_doingStartup && offset != 0) || (m_doingStartup == false))
  {
    m_doingStartup = false;

    // pending sliding windows results will be for incorrect location, so resend new requests
    m_currentWindowSize = 0;
  }
}
