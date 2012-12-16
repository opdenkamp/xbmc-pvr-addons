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

#include <vdr/ringbuffer.h>
#include <vdr/remux.h>

class cVideoBufferSimple : public cVideoBuffer
{
friend class cVideoBuffer;
public:
  virtual void Put(uint8_t *buf, int size);
  virtual int Read(uint8_t **buf, int size);

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

void cVideoBufferSimple::Put(uint8_t *buf, int size)
{
  m_Buffer->Put(buf, size);
}

int cVideoBufferSimple::Read(uint8_t **buf, int size)
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

cVideoBuffer::cVideoBuffer()
{
}

cVideoBuffer::~cVideoBuffer()
{
}

cVideoBuffer* cVideoBuffer::Create()
{
  cVideoBuffer *buffer = new cVideoBufferSimple();
  return buffer;
}
