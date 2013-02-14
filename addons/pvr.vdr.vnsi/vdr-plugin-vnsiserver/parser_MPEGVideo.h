/*
 *      vdr-plugin-vnsi - XBMC server plugin for VDR
 *
 *      Copyright (C) 2010 Alwin Esch (Team XBMC)
 *
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

#ifndef VNSI_DEMUXER_MPEGVIDEO_H
#define VNSI_DEMUXER_MPEGVIDEO_H

#include <deque>
#include "parser.h"

class cBitstream;

// --- cParserMPEG2Video -------------------------------------------------

class cParserMPEG2Video : public cParser
{
private:
  uint32_t        m_StartCode;
  bool            m_NeedIFrame;
  bool            m_NeedSPS;
  int             m_FrameDuration;
  int             m_vbvDelay;       /* -1 if CBR */
  int             m_vbvSize;        /* Video buffer size (in bytes) */
  int             m_Width;
  int             m_Height;
  float           m_Dar;
  int64_t         m_DTS;
  int64_t         m_PTS;

  int Parse_MPEG2Video(uint32_t startcode, int buf_ptr, bool &complete);
  bool Parse_MPEG2Video_SeqStart(uint8_t *buf);
  bool Parse_MPEG2Video_PicStart(uint8_t *buf);

public:
  cParserMPEG2Video(int pID, cTSStream *stream);
  virtual ~cParserMPEG2Video();

  virtual void Parse(sStreamPacket *pkt);
  virtual void Reset();
};

#endif // VNSI_DEMUXER_MPEGVIDEO_H
