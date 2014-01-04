/*
 *      Copyright (C) 2013 Jean-Luc Barriere
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
 *  along with this program; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifndef ELEMENTARYSTREAM_H
#define ELEMENTARYSTREAM_H

#include "common.h"

enum STREAM_TYPE
{
  STREAM_TYPE_UNKNOWN = 0,
  STREAM_TYPE_VIDEO_MPEG1,
  STREAM_TYPE_VIDEO_MPEG2,
  STREAM_TYPE_AUDIO_MPEG1,
  STREAM_TYPE_AUDIO_MPEG2,
  STREAM_TYPE_PRIVATE_DATA,
  STREAM_TYPE_AUDIO_AAC,
  STREAM_TYPE_VIDEO_MPEG4,
  STREAM_TYPE_VIDEO_H264,
  STREAM_TYPE_VIDEO_VC1,
  STREAM_TYPE_AUDIO_LPCM,
  STREAM_TYPE_AUDIO_AC3,
  STREAM_TYPE_AUDIO_EAC3,
  STREAM_TYPE_AUDIO_DTS,
  STREAM_TYPE_DVB_TELETEXT,
  STREAM_TYPE_DVB_SUBTITLE
};

class ElementaryStream
{
public:
  ElementaryStream(uint16_t pes_pid);
  virtual ~ElementaryStream();
  virtual void Reset();
  void ClearBuffer();
  int Append(const unsigned char* buf, size_t len, bool new_pts = false);
  const char* GetStreamCodecName() const;
  static const char* GetStreamCodecName(STREAM_TYPE stream_type);

  uint16_t pid;
  STREAM_TYPE stream_type;
  uint64_t c_dts;               ///< current MPEG stream DTS (decode time for video)
  uint64_t c_pts;               ///< current MPEG stream PTS (presentation time for audio and video)
  uint64_t p_dts;               ///< previous MPEG stream DTS (decode time for video)
  uint64_t p_pts;               ///< previous MPEG stream PTS (presentation time for audio and video)

  bool has_stream_info;         ///< true if stream info is completed else it requires parsing of iframe

  struct STREAM_INFO
  {
    char                  language[4];
    int                   composition_id;
    int                   ancillary_id;
    int                   fps_scale;
    int                   fps_rate;
    int                   height;
    int                   width;
    float                 aspect;
    int                   channels;
    int                   sample_rate;
    int                   block_align;
    int                   bit_rate;
    int                   bits_Per_sample;
  } stream_info;

  typedef struct
  {
    uint16_t              pid;
    size_t                size;
    const unsigned char*  data;
    uint64_t              dts;
    uint64_t              pts;
    uint64_t              duration;
    bool                  streamChange;
  } STREAM_PKT;

  bool GetStreamPacket(STREAM_PKT* pkt);
  virtual void Parse(STREAM_PKT* pkt);

protected:
  void ResetStreamPacket(STREAM_PKT* pkt);
  uint64_t Rescale(uint64_t a, uint64_t b, uint64_t c);
  bool SetVideoInformation(int FpsScale, int FpsRate, int Height, int Width, float Aspect);
  bool SetAudioInformation(int Channels, int SampleRate, int BitRate, int BitsPerSample, int BlockAlign);

  size_t es_alloc_init;         ///< Initial allocation of memory for buffer
  unsigned char* es_buf;        ///< The Pointer to buffer
  size_t es_alloc;              ///< Allocated size of memory for buffer
  size_t es_len;                ///< Size of data in buffer
  size_t es_consumed;           ///< Consumed payload. Will be erased on next append
  size_t es_pts_pointer;        ///< Position in buffer where current PTS becomes applicable
  size_t es_parsed;             ///< Parser: Last processed position in buffer
  bool   es_found_frame;        ///< Parser: Found frame
};

#endif /* ELEMENTARYSTREAM_H */
