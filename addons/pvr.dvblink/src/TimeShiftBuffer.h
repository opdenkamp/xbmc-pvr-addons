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
 
#pragma once

#ifdef TARGET_WINDOWS
#define DeleteFile  DeleteFileA
#endif

#include "libXBMC_addon.h"
#include "platform/util/StdString.h"
#include "libdvblinkremote/dvblinkremote.h"
#include "platform/threads/threads.h"
#include "platform/util/util.h"

#define STREAM_READ_BUFFER_SIZE   8192
#define BUFFER_READ_TIMEOUT       10000
#define BUFFER_READ_WAITTIME      50

class TimeShiftBuffer : public PLATFORM::CThread
{
public:
  TimeShiftBuffer(ADDON::CHelper_libXBMC_addon * XBMC, std::string streampath, std::string bufferpath);
  ~TimeShiftBuffer(void);
  int ReadData(unsigned char *pBuffer, unsigned int iBufferSize);
  bool IsValid();
  long long Seek(long long iPosition, int iWhence);
  long long Position();
  long long Length();
  void Stop(void);
private:
  virtual void * Process(void);

  std::string m_bufferPath;
  void * m_streamHandle;
  void * m_filebufferReadHandle;
  void * m_filebufferWriteHandle;
  bool m_shifting;
  ADDON::CHelper_libXBMC_addon * XBMC;
};

