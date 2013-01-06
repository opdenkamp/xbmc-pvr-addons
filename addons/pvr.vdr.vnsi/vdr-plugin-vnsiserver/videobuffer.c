/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "videobuffer.h"
#include "config.h"
#include "vnsi.h"

#include <vdr/ringbuffer.h>
#include <vdr/remux.h>

class cVideoBufferSimple : public cVideoBuffer
{
friend class cVideoBuffer;
public:
  virtual void Put(uint8_t *buf, unsigned int size);
  virtual int Read(uint8_t **buf, unsigned int size);

protected:
  cVideoBufferSimple();
  virtual ~cVideoBufferSimple();
  cRingBufferLinear *m_Buffer;
  int m_BytesConsumed;
};

cVideoBufferSimple::cVideoBufferSimple()
{
  m_Buffer = new cRingBufferLinear(MEGABYTE(3), TS_SIZE * 2, false);
  m_Buffer->SetTimeouts(0, 100);
  m_BytesConsumed = 0;
}

cVideoBufferSimple::~cVideoBufferSimple()
{
  if (m_Buffer)
    delete m_Buffer;
}

void cVideoBufferSimple::Put(uint8_t *buf, unsigned int size)
{
  m_Buffer->Put(buf, size);
}

int cVideoBufferSimple::Read(uint8_t **buf, unsigned int size)
{
  int  readBytes;
  if (m_BytesConsumed)
  {
    m_Buffer->Del(m_BytesConsumed);
  }
  m_BytesConsumed = 0;
  *buf = m_Buffer->Get(readBytes);
  if (!(*buf) || readBytes < TS_SIZE)
  {
    usleep(100);
    return 0;
  }
  /* Make sure we are looking at a TS packet */
  while (readBytes > TS_SIZE)
  {
    if ((*buf)[0] == TS_SYNC_BYTE && (*buf)[TS_SIZE] == TS_SYNC_BYTE)
      break;
    m_BytesConsumed++;
    (*buf)++;
    readBytes--;
  }

  if ((*buf)[0] != TS_SYNC_BYTE)
  {
    m_Buffer->Del(m_BytesConsumed);
    m_BytesConsumed = 0;
    return 0;
  }

  m_BytesConsumed += TS_SIZE;
  return TS_SIZE;
}

//-----------------------------------------------------------------------------

#define MARGIN 40000

class cVideoBufferRAM : public cVideoBuffer
{
friend class cVideoBuffer;
public:
  virtual void Put(uint8_t *buf, unsigned int size);
  virtual int Read(uint8_t **buf, unsigned int size);
  virtual size_t GetPosMin();
  virtual size_t GetPosMax();
  virtual size_t GetPosCur();
  virtual void GetPositions(size_t *cur, size_t *min, size_t *max);
  virtual void SetPos(size_t pos);

protected:
  cVideoBufferRAM();
  virtual ~cVideoBufferRAM();
  bool Init();
  size_t Available();
  uint8_t *m_Buffer;
  uint8_t *m_BufferPtr;
  size_t m_BufferSize;
  size_t m_WritePtr;
  size_t m_ReadPtr;
  bool m_BufferFull;
  unsigned int m_Margin;
  unsigned int m_BytesConsumed;
  cMutex m_Mutex;
};

cVideoBufferRAM::cVideoBufferRAM()
{
  m_Buffer = 0;
  m_Margin = TS_SIZE*2;
  m_BufferFull = false;
  m_ReadPtr = 0;
  m_WritePtr = 0;
  m_BytesConsumed = 0;
}

cVideoBufferRAM::~cVideoBufferRAM()
{
  if (m_Buffer)
    free(m_Buffer);
}

bool cVideoBufferRAM::Init()
{
  m_BufferSize = (size_t)TimeshiftBufferSize*100*1000*1000;
  INFOLOG("allocated timeshift buffer with size: %ld", m_BufferSize);
  m_Buffer = (uint8_t*)malloc(m_BufferSize + m_Margin);
  m_BufferPtr = m_Buffer + m_Margin;
  if (!m_Buffer)
    return false;
  else
    return true;
}

size_t cVideoBufferRAM::GetPosMin()
{
  size_t ret;
  if (!m_BufferFull)
    return 0;

  ret = m_WritePtr + MARGIN * 2;
  if (ret >= m_BufferSize)
    ret -= m_BufferSize;

  return ret;
}

size_t cVideoBufferRAM::GetPosMax()
{
   size_t ret = m_WritePtr;
   if (ret < GetPosMin())
     ret += m_BufferSize;
   return ret;
}

size_t cVideoBufferRAM::GetPosCur()
{
  size_t ret = m_ReadPtr;
  if (ret < GetPosMin())
    ret += m_BufferSize;
  return ret;
}

void cVideoBufferRAM::GetPositions(size_t *cur, size_t *min, size_t *max)
{
  cMutexLock lock(&m_Mutex);

  *cur = GetPosCur();
  *min = GetPosMin();
  *min = (*min > *cur) ? *cur : *min;
  *max = GetPosMax();
}

void cVideoBufferRAM::SetPos(size_t pos)
{
  cMutexLock lock(&m_Mutex);

  m_ReadPtr = pos;
  if (m_ReadPtr >= m_BufferSize)
    m_ReadPtr -= m_BufferSize;
  m_BytesConsumed = 0;
}

size_t cVideoBufferRAM::Available()
{
  size_t ret;
  if (m_ReadPtr <= m_WritePtr)
    ret = m_WritePtr - m_ReadPtr;
  else
    ret = m_BufferSize - (m_ReadPtr - m_WritePtr);

  return ret;
}

void cVideoBufferRAM::Put(uint8_t *buf, unsigned int size)
{
  cMutexLock lock(&m_Mutex);

  if (Available() + MARGIN >= m_BufferSize)
  {
    ERRORLOG("------------- skipping data");
    return;
  }

  if ((m_BufferSize - m_WritePtr) <= size)
  {
    int bytes = m_BufferSize - m_WritePtr;
    memcpy(m_BufferPtr+m_WritePtr, buf, bytes);
    size -= bytes;
    m_WritePtr = 0;
  }

  memcpy(m_BufferPtr+m_WritePtr, buf, size);
  m_WritePtr += size;

  if (!m_BufferFull)
  {
    if ((m_WritePtr + 2*MARGIN) > m_BufferSize)
      m_BufferFull = true;
  }
}

int cVideoBufferRAM::Read(uint8_t **buf, unsigned int size)
{
  cMutexLock lock(&m_Mutex);

  // move read pointer
  if (m_BytesConsumed)
  {
    m_ReadPtr += m_BytesConsumed;
    if (m_ReadPtr >= m_BufferSize)
      m_ReadPtr -= m_BufferSize;
  }
  m_BytesConsumed = 0;

  // check if we have anything to read
  size_t readBytes = Available();
  if (readBytes < m_Margin)
  {
    return 0;
  }

  // if we are close to end, copy margin to front
  if (m_ReadPtr > (m_BufferSize - m_Margin))
  {
    int bytesToCopy = m_BufferSize - m_ReadPtr;
    memmove(m_Buffer + (m_Margin - bytesToCopy), m_Buffer + m_ReadPtr, bytesToCopy);
    *buf = m_Buffer + (m_Margin - bytesToCopy);
  }
  else
    *buf = m_BufferPtr + m_ReadPtr;

  // Make sure we are looking at a TS packet
  while (readBytes > TS_SIZE)
  {
    if ((*buf)[0] == TS_SYNC_BYTE && (*buf)[TS_SIZE] == TS_SYNC_BYTE)
      break;
    m_BytesConsumed++;
    (*buf)++;
    readBytes--;
  }

  if ((*buf)[0] != TS_SYNC_BYTE)
  {
    return 0;
  }

  m_BytesConsumed += TS_SIZE;
  return TS_SIZE;
}

//-----------------------------------------------------------------------------

cVideoBuffer::cVideoBuffer()
{
}

cVideoBuffer::~cVideoBuffer()
{
}

cVideoBuffer* cVideoBuffer::Create()
{
  // no time shift
  if (TimeshiftMode == 0)
  {
    cVideoBufferSimple *buffer = new cVideoBufferSimple();
    return buffer;
  }
  // buffer in ram
  else if (TimeshiftMode == 1)
  {
    cVideoBufferRAM *buffer = new cVideoBufferRAM();
    if (!buffer->Init())
    {
      delete buffer;
      return NULL;
    }
    else
      return buffer;
  }
  else
    return NULL;
}
