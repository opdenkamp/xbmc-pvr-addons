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

#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <vdr/tools.h>

class cRecording;

class cVideoBuffer
{
public:
  virtual ~cVideoBuffer();
  static cVideoBuffer* Create(int clientID, uint8_t timeshift);
  static cVideoBuffer* Create(cString filename);
  static cVideoBuffer* Create(cRecording *rec);
  virtual void Put(uint8_t *buf, unsigned int size) = 0;
  virtual int ReadBlock(uint8_t **buf, unsigned int size, time_t &endTime, time_t &wrapTime) = 0;
  virtual off_t GetPosMin() { return 0; };
  virtual off_t GetPosMax() { return 0; };
  virtual off_t GetPosCur() { return 0; };
  virtual void GetPositions(off_t *cur, off_t *min, off_t *max) {};
  virtual void SetPos(off_t pos) {};
  virtual void SetCache(bool on) {};
  virtual bool HasBuffer() { return false; };
  virtual time_t GetRefTime();
  int Read(uint8_t **buf, unsigned int size, time_t &endTime, time_t &wrapTime);
  void AttachInput(bool attach);
protected:
  cVideoBuffer();
  cTimeMs m_Timer;
  bool m_CheckEof;
  bool m_InputAttached;
  time_t m_bufferEndTime;
  time_t m_bufferWrapTime;
};
