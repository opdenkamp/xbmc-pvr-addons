#pragma once
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

#ifndef LiveSlip_H
#define LiveSlip_H

#include "libXBMC_addon.h"
#include <string>
#include "platform/os.h"
#include "Socket.h"

class LiveShiftSource
{
public:
  LiveShiftSource(NextPVR::Socket *pSocket);
  ~LiveShiftSource(void);


  unsigned int Read(unsigned char *buffer, unsigned int length);

  long long GetLength();
  long long GetPosition();
  void Seek(long long offset);

  void Close();

private:
  void LOG(char const *fmt, ... );

  NextPVR::Socket *m_pSocket;
  long long m_lastKnownLength;
  long long m_position;
  int m_currentWindowSize;
  bool m_doingStartup;

  FILE *m_log;
  int m_requestNumber;

  int m_startCacheBytes;
  unsigned char *m_startupCache;
};

#endif /* LiveSlip_H */
