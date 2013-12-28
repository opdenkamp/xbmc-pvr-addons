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

#include <stdlib.h>
#include <assert.h>
#include "config.h"
#include "bitstream.h"

#include "parser_MPEGVideo.h"

using namespace std;

#define MPEG_PICTURE_START      0x00000100
#define MPEG_SEQUENCE_START     0x000001b3
#define MPEG_SEQUENCE_EXTENSION 0x000001b5
#define MPEG_SLICE_S            0x00000101
#define MPEG_SLICE_E            0x000001af

/**
 * MPEG2VIDEO frame duration table (in 90kHz clock domain)
 */
const unsigned int mpeg2video_framedurations[16] = {
  0,
  3753,
  3750,
  3600,
  3003,
  3000,
  1800,
  1501,
  1500,
};

cParserMPEG2Video::cParserMPEG2Video(int pID, cTSStream *stream, sPtsWrap *ptsWrap, bool observePtsWraps)
 : cParser(pID, stream, ptsWrap, observePtsWraps)
{
  m_FrameDuration     = 0;
  m_vbvDelay          = -1;
  m_vbvSize           = 0;
  m_Height            = 0;
  m_Width             = 0;
  m_Dar               = 0.0;
  m_FpsScale          = 0;
  m_PesBufferInitialSize  = 80000;
  m_IsVideo = true;
  Reset();
}

cParserMPEG2Video::~cParserMPEG2Video()
{
}

void cParserMPEG2Video::Parse(sStreamPacket *pkt)
{
  if (m_PesBufferPtr < 4)
    return;

  int p = m_PesParserPtr;
  uint32_t startcode = m_StartCode;
  bool frameComplete = false;
  int l;
  while ((l = m_PesBufferPtr - p) > 3)
  {
    if ((startcode & 0xffffff00) == 0x00000100)
    {
      if (Parse_MPEG2Video(startcode, p, frameComplete) < 0)
      {
        break;
      }
    }
    startcode = startcode << 8 | m_PesBuffer[p++];
  }
  m_PesParserPtr = p;
  m_StartCode = startcode;

  if (frameComplete)
  {
    if (!m_NeedSPS && !m_NeedIFrame)
    {
      if (m_FpsScale == 0)
      {
        if (m_FrameDuration != DVD_NOPTS_VALUE)
          m_FpsScale = m_Stream->Rescale(m_FrameDuration, DVD_TIME_BASE, 90000);
        else
          m_FpsScale = 40000;
      }
      bool streamChange = m_Stream->SetVideoInformation(m_FpsScale, DVD_TIME_BASE, m_Height, m_Width, m_Dar);

      pkt->id       = m_pID;
      pkt->size     = m_PesNextFramePtr;
      pkt->data     = m_PesBuffer;
      pkt->dts      = m_DTS;
      pkt->pts      = m_PTS;
      pkt->duration = m_FrameDuration;
      pkt->streamChange = streamChange;
    }
    m_StartCode = 0xffffffff;
    m_PesParserPtr = 0;
    m_FoundFrame = false;
  }
}

void cParserMPEG2Video::Reset()
{
  cParser::Reset();
  m_StartCode = 0xffffffff;
  m_NeedIFrame = true;
  m_NeedSPS = true;
}

int cParserMPEG2Video::Parse_MPEG2Video(uint32_t startcode, int buf_ptr, bool &complete)
{
  int len = m_PesBufferPtr - buf_ptr;
  uint8_t *buf = m_PesBuffer + buf_ptr;

  switch (startcode & 0xFF)
  {
  case 0: // picture start
  {
    if (m_NeedSPS)
    {
      m_FoundFrame = true;
      return 0;
    }
    if (m_FoundFrame)
    {
      complete = true;
      m_PesNextFramePtr = buf_ptr - 4;
      return -1;
    }
    if (len < 4)
      return -1;
    if (!Parse_MPEG2Video_PicStart(buf))
      return 0;

    if (!m_FoundFrame)
    {
      m_AuPrevDTS = m_AuDTS;
      if (buf_ptr - 4 >= m_PesTimePos)
      {
        m_AuDTS = m_curDTS != DVD_NOPTS_VALUE ? m_curDTS : m_curPTS;
        m_AuPTS = m_curPTS;
      }
      else
      {
        m_AuDTS = m_prevDTS != DVD_NOPTS_VALUE ? m_prevDTS : m_prevPTS;;
        m_AuPTS = m_prevPTS;
      }
    }
    if (m_AuPrevDTS == m_AuDTS)
    {
      m_DTS = m_AuDTS + m_PicNumber*m_FrameDuration;
      m_PTS = m_AuPTS + (m_TemporalReference-m_TrLastTime)*m_FrameDuration;
    }
    else
    {
      m_PTS = m_AuPTS;
      m_DTS = m_AuDTS;
      m_PicNumber = 0;
      m_TrLastTime = m_TemporalReference;
    }

    m_PicNumber++;
    m_FoundFrame = true;
    break;
  }

  case 0xb3: // Sequence start code
  {
    if (m_FoundFrame)
    {
      complete = true;
      m_PesNextFramePtr = buf_ptr - 4;
      return -1;
    }
    if (len < 8)
      return -1;
    if (!Parse_MPEG2Video_SeqStart(buf))
      return 0;

    break;
  }

  case 0xb7: // sequence end
  {
    if (m_FoundFrame)
    {
      complete = true;
      m_PesNextFramePtr = buf_ptr;
      return -1;
    }
    break;
  }

  default:
    break;
  }

  return 0;
}

bool cParserMPEG2Video::Parse_MPEG2Video_SeqStart(uint8_t *buf)
{
  cBitstream bs(buf, 8 * 8);

  m_Width         = bs.readBits(12);
  m_Height        = bs.readBits(12);

  // figure out Display Aspect Ratio
  uint8_t aspect = bs.readBits(4);

  switch(aspect)
  {
    case 1:
      m_Dar = 1.0;
      break;
    case 2:
      m_Dar = 4.0/3.0;
      break;
    case 3:
      m_Dar = 16.0/9.0;
      break;
    case 4:
      m_Dar = 2.21;
      break;
    default:
      ERRORLOG("invalid / forbidden DAR in sequence header !");
      return false;
  }

  m_FrameDuration = mpeg2video_framedurations[bs.readBits(4)];
  bs.skipBits(18);
  bs.skipBits(1);

  m_vbvSize = bs.readBits(10) * 16 * 1024 / 8;
  m_NeedSPS = false;

  return true;
}

bool cParserMPEG2Video::Parse_MPEG2Video_PicStart(uint8_t *buf)
{
  cBitstream bs(buf, 4 * 8);

  m_TemporalReference = bs.readBits(10); /* temporal reference */

  int pct = bs.readBits(3);
  if (pct < PKT_I_FRAME || pct > PKT_B_FRAME)
    return true; /* Illegal picture_coding_type */

  if (pct == PKT_I_FRAME)
    m_NeedIFrame = false;

  int vbvDelay = bs.readBits(16); /* vbv_delay */
  if (vbvDelay  == 0xffff)
    m_vbvDelay = -1;
  else
    m_vbvDelay = vbvDelay;

  return true;
}
