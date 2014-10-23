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

class LiveStreamerBase
{
public:
    LiveStreamerBase(ADDON::CHelper_libXBMC_addon * XBMC);
    virtual ~LiveStreamerBase();

    virtual bool Start(std::string& streampath);
    virtual void Stop();
    virtual int ReadData(unsigned char *pBuffer, unsigned int iBufferSize);

    virtual long long Seek(long long iPosition, int iWhence){ return -1; }
    virtual long long Position(){ return -1; }
    virtual long long Length(){ return -1; }

    virtual time_t GetPlayingTime(){ return 0; }
    virtual time_t GetBufferTimeStart(){ return 0; }
    virtual time_t GetBufferTimeEnd(){ return 0; }

    virtual dvblinkremote::StreamRequest* GetStreamRequest(long dvblink_channel_id, const std::string& client_id, const std::string& host_name,
        bool use_transcoder, int width, int height, int bitrate, std::string audiotrack) {return NULL;}

protected:
    void * m_streamHandle;
    ADDON::CHelper_libXBMC_addon * XBMC;
    std::string streampath_;
};

class LiveTVStreamer : public LiveStreamerBase
{
public:
    LiveTVStreamer(ADDON::CHelper_libXBMC_addon * XBMC);

    virtual dvblinkremote::StreamRequest* GetStreamRequest(long dvblink_channel_id, const std::string& client_id, const std::string& host_name,
        bool use_transcoder, int width, int height, int bitrate, std::string audiotrack);
};


class TimeShiftBuffer : public LiveStreamerBase
{
public:
  TimeShiftBuffer(ADDON::CHelper_libXBMC_addon * XBMC);
  ~TimeShiftBuffer(void);

  virtual long long Seek(long long iPosition, int iWhence);
  virtual long long Position();
  virtual long long Length();

  virtual time_t GetPlayingTime();
  virtual time_t GetBufferTimeStart();
  virtual time_t GetBufferTimeEnd();

  virtual dvblinkremote::StreamRequest* GetStreamRequest(long dvblink_channel_id, const std::string& client_id, const std::string& host_name,
      bool use_transcoder, int width, int height, int bitrate, std::string audiotrack);

protected:
  bool ExecuteServerRequest(const std::string& url, std::vector<std::string>& response_values);
  bool GetBufferParams(long long& length, time_t& duration, long long& cur_pos);

  time_t last_pos_req_time_;
  time_t last_pos_;
};

