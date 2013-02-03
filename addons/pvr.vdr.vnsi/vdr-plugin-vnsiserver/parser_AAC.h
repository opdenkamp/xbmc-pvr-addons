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

#ifndef VNSI_DEMUXER_AAC_H
#define VNSI_DEMUXER_AAC_H

#include "parser.h"
#include "bitstream.h"

// --- cParserAAC -------------------------------------------------

class cParserAAC : public cParser
{
private:
  int         m_SampleRate;
  int         m_Channels;
  int         m_BitRate;
  int         m_FrameSize;

  int64_t     m_PTS;                /* pts of the current frame */
  int64_t     m_DTS;                /* dts of the current frame */

  bool        m_Configured;
  int         m_AudioMuxVersion_A;
  int         m_FrameLengthType;

  int FindHeaders(uint8_t *buf, int buf_size);
  bool ParseLATMAudioMuxElement(cBitstream *bs);
  void ReadStreamMuxConfig(cBitstream *bs);
  void ReadAudioSpecificConfig(cBitstream *bs);
  uint32_t LATMGetValue(cBitstream *bs) { return bs->readBits(bs->readBits(2) * 8); }

public:
  cParserAAC(int pID, cTSStream *stream);
  virtual ~cParserAAC();
  virtual void Parse(sStreamPacket *pkt);
  virtual void Reset();
};

#endif // VNSI_DEMUXER_AAC_H
