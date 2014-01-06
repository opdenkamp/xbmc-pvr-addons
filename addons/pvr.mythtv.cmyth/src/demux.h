#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "cppmyth/MythRecorder.h"
#include "demuxer/tsDemuxer.h"
#include "client.h"

#include <platform/threads/threads.h>
#include <platform/threads/mutex.h>
#include <platform/util/buffer.h>
#include <xbmc_stream_utils.hpp>

#include <map>
#include <set>

#define AV_BUFFER_SIZE          131072

class Demux : public TSDemuxer, PLATFORM::CThread
{
public:
  Demux(MythRecorder &recorder);
  ~Demux();

  const unsigned char* ReadAV(uint64_t pos, size_t n);

  void* Process();

  bool GetStreamProperties(PVR_STREAM_PROPERTIES* props);
  void Flush();
  void Abort();
  DemuxPacket* Read();
  bool SeekTime(int time, bool backwards, double* startpts);

  int GetPlayingTime();

private:
  MythRecorder m_recorder;
  uint16_t m_channel;
  PLATFORM::SyncedBuffer<DemuxPacket*> m_demuxPacketBuffer;
  PLATFORM::CMutex m_mutex;
  ADDON::XbmcStreamProperties m_streams;

  bool get_stream_data(ElementaryStream::STREAM_PKT* pkt);
  void reset_posmap();

  // PVR interfaces
  void populate_pvr_streams();
  bool update_pvr_stream(uint16_t pid);
  void push_stream_change();
  DemuxPacket* stream_pvr_data(ElementaryStream::STREAM_PKT* pkt);
  void push_stream_data(DemuxPacket* dxp);

  // AV raw buffer
  size_t m_av_buf_size;         ///< size of av buffer
  uint64_t m_av_pos;            ///< absolute position in av
  unsigned char* m_av_buf;      ///< buffer
  unsigned char* m_av_rbs;      ///< raw data start in buffer
  unsigned char* m_av_rbe;      ///< raw data end in buffer

  // Playback context
  AVContext* m_AVContext;
  uint16_t m_mainStreamPID;     ///< PID of main stream
  uint64_t m_DTS;               ///< absolute decode time of main stream
  uint64_t m_PTS;               ///< absolute presentation time of main stream
  int64_t m_pinTime;            ///< pinned relative position (90Khz)
  int64_t m_curTime;            ///< current relative position (90Khz)
  int64_t m_endTime;            ///< last relative marked position (90Khz))
  typedef struct
  {
    uint64_t av_pts;
    uint64_t av_pos;
  } AV_POSMAP_ITEM;
  std::map<int64_t, AV_POSMAP_ITEM> m_posmap;

  bool m_isChangePlaced;
  std::set<uint16_t> m_nosetup;
};
