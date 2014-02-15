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

#include <vdr/thread.h>

class cLivePatFilter;
class cLiveReceiver;
class cVideoBuffer;
class cChannel;
class cDevice;

class cVideoInput : public cThread
{
friend class cLivePatFilter;
friend class cLiveReceiver;
public:
  cVideoInput(cCondVar &condVar, cMutex &mutex, bool &retune);
  virtual ~cVideoInput();
  bool Open(const cChannel *channel, int priority, cVideoBuffer *videoBuffer);
  void Close();
  bool IsOpen();

protected:
  virtual void Action(void);
  void PmtChange(int pidChange);
  cChannel *PmtChannel();
  void Receive(uchar *data, int length);
  void Retune();
  cDevice          *m_Device;
  cLivePatFilter   *m_PatFilter;
  cLiveReceiver    *m_Receiver;
  cLiveReceiver    *m_Receiver0;
  const cChannel   *m_Channel;
  cVideoBuffer     *m_VideoBuffer;
  int               m_Priority;
  bool              m_PmtChange;
  bool              m_SeenPmt;
  cCondVar          &m_Event;
  cMutex            &m_Mutex;
  bool              &m_IsRetune;
};
