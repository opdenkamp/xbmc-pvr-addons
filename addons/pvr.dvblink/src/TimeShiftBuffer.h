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
 *  the Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301  USA
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
 
#pragma once

#include "libXBMC_addon.h"
#include "platform/util/StdString.h"
#include "libdvblinkremote/dvblinkremote.h"
#include "platform/util/util.h"

class TimeShiftBuffer
{
public:
  TimeShiftBuffer(ADDON::CHelper_libXBMC_addon * XBMC, std::string streampath);
  ~TimeShiftBuffer(void);

  int ReadData(unsigned char *pBuffer, unsigned int iBufferSize);
  long long Seek(long long iPosition, int iWhence);
  long long Position();
  long long Length();
  void Stop();

  time_t GetPlayingTime();
  time_t GetBufferTimeStart();
  time_t GetBufferTimeEnd();

private:
  bool ExecuteServerRequest(const std::string& url, std::vector<std::string>& response_values);
  bool GetBufferParams(long long& length, time_t& duration, long long& cur_pos);

  void * m_streamHandle;
  ADDON::CHelper_libXBMC_addon * XBMC;
  std::string streampath_;
  time_t last_pos_req_time_;
  time_t last_pos_;
};

