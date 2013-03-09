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
#include "recplayer.h"

#include <vdr/ringbuffer.h>
#include <vdr/remux.h>
#include <vdr/videodir.h>
#include <vdr/recording.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

class cVideoBufferSimple : public cVideoBuffer
{
friend class cVideoBuffer;
public:
  virtual void Put(uint8_t *buf, unsigned int size);
  virtual int ReadBlock(uint8_t **buf, unsigned int size);

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

int cVideoBufferSimple::ReadBlock(uint8_t **buf, unsigned int size)
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

class cVideoBufferTimeshift : public cVideoBuffer
{
friend class cVideoBuffer;
public:
  virtual size_t GetPosMin();
  virtual size_t GetPosMax();
  virtual size_t GetPosCur();
  virtual void GetPositions(size_t *cur, size_t *min, size_t *max);

protected:
  cVideoBufferTimeshift();
  virtual bool Init() = 0;
  virtual off_t Available();
  off_t m_BufferSize;
  off_t m_WritePtr;
  off_t m_ReadPtr;
  bool m_BufferFull;
  unsigned int m_Margin;
  unsigned int m_BytesConsumed;
  cMutex m_Mutex;
};

cVideoBufferTimeshift::cVideoBufferTimeshift()
{
  m_Margin = TS_SIZE*2;
  m_BufferFull = false;
  m_ReadPtr = 0;
  m_WritePtr = 0;
  m_BytesConsumed = 0;
}

size_t cVideoBufferTimeshift::GetPosMin()
{
  off_t ret;
  if (!m_BufferFull)
    return 0;

  ret = m_WritePtr + MARGIN * 2;
  if (ret >= m_BufferSize)
    ret -= m_BufferSize;

  return ret;
}

size_t cVideoBufferTimeshift::GetPosMax()
{
   size_t ret = m_WritePtr;
   if (ret < GetPosMin())
     ret += m_BufferSize;
   return ret;
}

size_t cVideoBufferTimeshift::GetPosCur()
{
  size_t ret = m_ReadPtr;
  if (ret < GetPosMin())
    ret += m_BufferSize;
  return ret;
}

void cVideoBufferTimeshift::GetPositions(size_t *cur, size_t *min, size_t *max)
{
  cMutexLock lock(&m_Mutex);

  *cur = GetPosCur();
  *min = GetPosMin();
  *min = (*min > *cur) ? *cur : *min;
  *max = GetPosMax();
}

off_t cVideoBufferTimeshift::Available()
{
  size_t ret;
  if (m_ReadPtr <= m_WritePtr)
    ret = m_WritePtr - m_ReadPtr;
  else
    ret = m_BufferSize - (m_ReadPtr - m_WritePtr);

  return ret;
}
//-----------------------------------------------------------------------------

class cVideoBufferRAM : public cVideoBufferTimeshift
{
friend class cVideoBuffer;
public:
  virtual void Put(uint8_t *buf, unsigned int size);
  virtual int ReadBlock(uint8_t **buf, unsigned int size);
  virtual void SetPos(size_t pos);

protected:
  cVideoBufferRAM();
  virtual ~cVideoBufferRAM();
  virtual bool Init();
  uint8_t *m_Buffer;
  uint8_t *m_BufferPtr;
};

cVideoBufferRAM::cVideoBufferRAM()
{
  m_Buffer = 0;
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

void cVideoBufferRAM::SetPos(size_t pos)
{
  cMutexLock lock(&m_Mutex);

  m_ReadPtr = pos;
  if (m_ReadPtr >= m_BufferSize)
    m_ReadPtr -= m_BufferSize;
  m_BytesConsumed = 0;
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
    buf += bytes;
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

int cVideoBufferRAM::ReadBlock(uint8_t **buf, unsigned int size)
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

#define CACHE_SIZE 4000

class cVideoBufferFile : public cVideoBufferTimeshift
{
friend class cVideoBuffer;
public:
  virtual void Put(uint8_t *buf, unsigned int size);
  virtual int ReadBlock(uint8_t **buf, unsigned int size);
  virtual void SetPos(size_t pos);

protected:
  cVideoBufferFile();
  cVideoBufferFile(int clientID);
  virtual ~cVideoBufferFile();
  virtual bool Init();
  int m_ClientID;
  cString m_Filename;
  int m_Fd;
  bool m_UseCache;
  uint8_t m_ReadCache[CACHE_SIZE];
  unsigned int m_ReadCachePtr;
  unsigned int m_ReadCacheSize;
};

cVideoBufferFile::cVideoBufferFile()
{

}

cVideoBufferFile::cVideoBufferFile(int clientID)
{
  m_ClientID = clientID;
  m_Fd = 0;
  m_ReadCacheSize = 0;
}

cVideoBufferFile::~cVideoBufferFile()
{
  if (m_Fd)
  {
    close(m_Fd);
    unlink(m_Filename);
    m_Fd = 0;
  }
}

bool cVideoBufferFile::Init()
{
  m_BufferSize = (size_t)TimeshiftBufferFileSize*1000*1000*1000;

  m_Filename = cString::sprintf("%s/Timeshift-%d.vnsi", VideoDirectory, m_ClientID);
  m_Fd = open(m_Filename, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU);
  if (m_Fd == -1)
  {
    ERRORLOG("Could not open file: %s", (const char*)m_Filename);
    return false;
  }
  m_WritePtr = lseek(m_Fd, m_BufferSize - 1, SEEK_SET);
  if (m_WritePtr == -1)
  {
    ERRORLOG("(Init) Could not seek file: %s", (const char*)m_Filename);
    return false;
  }
  char tmp = '0';
  if (safe_write(m_Fd, &tmp, 1) < 0)
  {
    ERRORLOG("(Init) Could not write to file: %s", (const char*)m_Filename);
    return false;
  }

  m_WritePtr = 0;
  m_ReadPtr = 0;
  m_ReadCacheSize = 0;
  return true;
}

void cVideoBufferFile::SetPos(size_t pos)
{
  cMutexLock lock(&m_Mutex);

  m_ReadPtr = pos;
  if (m_ReadPtr >= m_BufferSize)
    m_ReadPtr -= m_BufferSize;
  m_BytesConsumed = 0;
  m_ReadCacheSize = 0;
}

void cVideoBufferFile::Put(uint8_t *buf, unsigned int size)
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

    m_WritePtr = lseek(m_Fd, m_WritePtr, SEEK_SET);
    if (m_WritePtr != m_WritePtr)
    {
      ERRORLOG("Could not seek file: %s", (const char*)m_Filename);
      return;
    }
    if (safe_write(m_Fd, buf, bytes) < 0)
    {
      ERRORLOG("Could not write to file: %s", (const char*)m_Filename);
      return;
    }
    size -= bytes;
    buf += bytes;
    m_WritePtr = 0;
  }

  m_WritePtr = lseek(m_Fd, m_WritePtr, SEEK_SET);
  if (m_WritePtr != m_WritePtr)
  {
    ERRORLOG("Could not seek file: %s", (const char*)m_Filename);
    return;
  }
  if (safe_write(m_Fd, buf, size) < 0)
  {
    ERRORLOG("Could not write to file: %s", (const char*)m_Filename);
    return;
  }
  m_WritePtr += size;

  if (!m_BufferFull)
  {
    if ((m_WritePtr + 2*MARGIN) > m_BufferSize)
      m_BufferFull = true;
  }
}

int cVideoBufferFile::ReadBlock(uint8_t **buf, unsigned int size)
{
  cMutexLock lock(&m_Mutex);

  // move read pointer
  if (m_BytesConsumed)
  {
    m_ReadPtr += m_BytesConsumed;
    if (m_ReadPtr >= m_BufferSize)
      m_ReadPtr -= m_BufferSize;
    m_ReadCachePtr += m_BytesConsumed;
  }
  m_BytesConsumed = 0;

  // check if we have anything to read
  size_t readBytes;
  if (m_ReadCacheSize && ((m_ReadCachePtr + m_Margin) <= m_ReadCacheSize))
  {
    readBytes = m_ReadCacheSize - m_ReadCachePtr;
    *buf = m_ReadCache + m_ReadCachePtr;
  }
  else if ((readBytes = Available()) >= CACHE_SIZE)
  {
    m_ReadPtr = lseek(m_Fd, m_ReadPtr, SEEK_SET);
    if (m_ReadPtr != m_ReadPtr)
    {
      ERRORLOG("Could not seek file: %s", (const char*)m_Filename);
      return 0;
    }
    if (m_ReadPtr + CACHE_SIZE <= m_BufferSize)
    {
      m_ReadCacheSize = safe_read(m_Fd, m_ReadCache, CACHE_SIZE);
      if (m_ReadCacheSize < 0)
      {
        ERRORLOG("Could not read file: %s", (const char*)m_Filename);
        return 0;
      }
      if (m_ReadCacheSize < m_Margin)
      {
        ERRORLOG("Could not read file (margin): %s", (const char*)m_Filename);
        m_ReadCacheSize = 0;
        return 0;
      }
      readBytes = m_ReadCacheSize;
      *buf = m_ReadCache;
      m_ReadCachePtr = 0;
    }
    else
    {
      m_ReadCacheSize = safe_read(m_Fd, m_ReadCache, m_BufferSize - m_ReadPtr);
      if (m_ReadCacheSize != (m_BufferSize - m_ReadPtr))
      {
        ERRORLOG("Could not read file (end): %s", (const char*)m_Filename);
        m_ReadCacheSize = 0;
        return 0;
      }
      m_ReadPtr = lseek(m_Fd, 0, SEEK_SET);
      if (m_ReadPtr != m_ReadPtr)
      {
        ERRORLOG("Could not seek file: %s", (const char*)m_Filename);
        return 0;
      }
      readBytes = safe_read(m_Fd, m_ReadCache + m_ReadCacheSize, CACHE_SIZE - m_ReadCacheSize);
      if (readBytes < 0)
      {
        ERRORLOG("Could not read file (end): %s", (const char*)m_Filename);
        m_ReadCacheSize = 0;
        return 0;
      }
      m_ReadCacheSize += readBytes;
      if (m_ReadCacheSize < m_Margin)
      {
        ERRORLOG("Could not read file (margin): %s", (const char*)m_Filename);
        m_ReadCacheSize = 0;
        return 0;
      }
      readBytes = m_ReadCacheSize;
      *buf = m_ReadCache;
      m_ReadCachePtr = 0;
    }
  }
  else
    return 0;

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

class cVideoBufferRecording : public cVideoBufferFile
{
friend class cVideoBuffer;
public:
  virtual size_t GetPosMax();
  virtual void Put(uint8_t *buf, unsigned int size);
  virtual int ReadBlock(uint8_t **buf, unsigned int size);

protected:
  cVideoBufferRecording(cRecording *rec);
  virtual ~cVideoBufferRecording();
  virtual bool Init();
  virtual off_t Available();
  off_t GetPosEnd();
  cRecPlayer *m_RecPlayer;
  cRecording *m_Recording;
  cTimeMs m_ScanTimer;
};

cVideoBufferRecording::cVideoBufferRecording(cRecording *rec)
{
  m_Recording = rec;
  m_ReadCacheSize = 0;
}

cVideoBufferRecording::~cVideoBufferRecording()
{
  INFOLOG("delete cVideoBufferRecording");
  if (m_RecPlayer)
    delete m_RecPlayer;
}

size_t cVideoBufferRecording::GetPosMax()
{
  m_RecPlayer->reScan();
  m_WritePtr = m_RecPlayer->getLengthBytes();
  return cVideoBufferTimeshift::GetPosMax();
}

void cVideoBufferRecording::Put(uint8_t *buf, unsigned int size)
{

}

bool cVideoBufferRecording::Init()
{
  m_RecPlayer = new cRecPlayer(m_Recording, true);
  if (!m_RecPlayer)
    return false;

  m_WritePtr = 0;
  m_ReadPtr = 0;
  m_ReadCacheSize = 0;
  m_InputAttached = false;
  m_ScanTimer.Set(0);
  return true;
}

off_t cVideoBufferRecording::Available()
{
  if (m_ScanTimer.TimedOut())
  {
    m_RecPlayer->reScan();
    m_ScanTimer.Set(1000);
  }
  m_BufferSize = m_WritePtr = m_RecPlayer->getLengthBytes();
  return cVideoBufferTimeshift::Available();
}

int cVideoBufferRecording::ReadBlock(uint8_t **buf, unsigned int size)
{
  cMutexLock lock(&m_Mutex);

  // move read pointer
  if (m_BytesConsumed)
  {
    m_ReadPtr += m_BytesConsumed;
    if (m_ReadPtr >= m_BufferSize)
    {
      m_ReadPtr -= m_BufferSize;
      ERRORLOG("cVideoBufferRecording::ReadBlock - unknown error");
    }
    m_ReadCachePtr += m_BytesConsumed;
  }
  m_BytesConsumed = 0;

  // check if we have anything to read
  size_t readBytes;
  if (m_ReadCacheSize && ((m_ReadCachePtr + m_Margin) <= m_ReadCacheSize))
  {
    readBytes = m_ReadCacheSize - m_ReadCachePtr;
    *buf = m_ReadCache + m_ReadCachePtr;
  }
  else if ((readBytes = Available()) >= CACHE_SIZE)
  {
    if (m_ReadPtr + CACHE_SIZE <= m_BufferSize)
    {
      m_ReadCacheSize = m_RecPlayer->getBlock(m_ReadCache, m_ReadPtr, CACHE_SIZE);
      if (m_ReadCacheSize != CACHE_SIZE)
      {
        ERRORLOG("Could not read file, size:  %d", m_ReadCacheSize);
        m_ReadCacheSize = 0;
        return 0;
      }
      readBytes = m_ReadCacheSize;
      *buf = m_ReadCache;
      m_ReadCachePtr = 0;
    }
    else
    {
      ERRORLOG("cVideoBufferRecording::ReadBlock - unknown error");
      return 0;
    }
  }
  else
    return 0;

  // Make sure we are looking at a TS packet
  while (readBytes > TS_SIZE)
  {
    if ((*buf)[0] == TS_SYNC_BYTE && (*buf)[TS_SIZE] == TS_SYNC_BYTE)
      break;
    m_BytesConsumed++;
    (*buf)++;
    readBytes--;
    INFOLOG("------------ skip byte");
  }

  if ((*buf)[0] != TS_SYNC_BYTE)
  {
    INFOLOG("------------ marker 11");
    return 0;
  }

  m_BytesConsumed += TS_SIZE;
  return TS_SIZE;
}

//-----------------------------------------------------------------------------

class cVideoBufferTest : public cVideoBufferFile
{
friend class cVideoBuffer;
public:
  virtual size_t GetPosMax();
  virtual void Put(uint8_t *buf, unsigned int size);

protected:
  cVideoBufferTest(cString filename);
  virtual ~cVideoBufferTest();
  virtual bool Init();
  virtual off_t Available();
  off_t GetPosEnd();
};

cVideoBufferTest::cVideoBufferTest(cString filename)
{
  m_Filename = filename;
  m_Fd = 0;
  m_ReadCacheSize = 0;
}

cVideoBufferTest::~cVideoBufferTest()
{
  if (m_Fd)
  {
    close(m_Fd);
    m_Fd = 0;
  }
}

size_t cVideoBufferTest::GetPosMax()
{
  m_WritePtr = GetPosEnd();
  return cVideoBufferTimeshift::GetPosMax();
}

off_t cVideoBufferTest::GetPosEnd()
{
  off_t cur = lseek(m_Fd, 0, SEEK_CUR);
  off_t end = lseek(m_Fd, 0, SEEK_END);
  lseek(m_Fd, cur, SEEK_SET);
  return end;
}

void cVideoBufferTest::Put(uint8_t *buf, unsigned int size)
{

}

bool cVideoBufferTest::Init()
{
  m_Fd = open(m_Filename, O_RDONLY);
  if (m_Fd == -1)
  {
    ERRORLOG("Could not open file: %s", (const char*)m_Filename);
    return false;
  }

  m_WritePtr = 0;
  m_ReadPtr = 0;
  m_ReadCacheSize = 0;
  m_InputAttached = false;
  return true;
}

off_t cVideoBufferTest::Available()
{
  m_BufferSize = m_WritePtr = GetPosEnd();
  return cVideoBufferTimeshift::Available();
}

//-----------------------------------------------------------------------------

cVideoBuffer::cVideoBuffer()
{
  m_CheckEof = false;
  m_InputAttached = true;
}

cVideoBuffer::~cVideoBuffer()
{
}

cVideoBuffer* cVideoBuffer::Create(int clientID)
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
  // buffer in file
  else if (TimeshiftMode == 2)
  {
    cVideoBufferFile *buffer = new cVideoBufferFile(clientID);
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

cVideoBuffer* cVideoBuffer::Create(cString filename)
{
  INFOLOG("Open recording: %s", (const char*)filename);
  cVideoBufferTest *buffer = new cVideoBufferTest(filename);
  if (!buffer->Init())
  {
    delete buffer;
    return NULL;
  }
  else
    return buffer;
}

cVideoBuffer* cVideoBuffer::Create(cRecording *rec)
{
  INFOLOG("Open recording: %s", rec->FileName());
  cVideoBufferRecording *buffer = new cVideoBufferRecording(rec);
  if (!buffer->Init())
  {
    delete buffer;
    return NULL;
  }
  else
    return buffer;
}

int cVideoBuffer::Read(uint8_t **buf, unsigned int size)
{
  int count = ReadBlock(buf, size);

  // check for end of file
  if (!m_InputAttached && count != TS_SIZE)
  {
    if (m_CheckEof && m_Timer.TimedOut())
    {
      INFOLOG("Recoding - end of file");
      return -2;
    }
    else if (!m_CheckEof)
    {
      m_CheckEof = true;
      m_Timer.Set(3000);
    }
  }
  else
    m_CheckEof = false;

  return count;
}

void cVideoBuffer::AttachInput(bool attach)
{
  m_InputAttached = attach;
}
