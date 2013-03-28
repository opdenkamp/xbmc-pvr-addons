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

#ifndef VNSI_DEMUXER_MPEGAUDIO_H
#define VNSI_DEMUXER_MPEGAUDIO_H

#include "parser.h"

// --- cParserMPEG2Audio -------------------------------------------------

class cParserMPEG2Audio : public cParser
{
private:
  int         m_SampleRate;
  int         m_Channels;
  int         m_BitRate;
  int         m_FrameSize;

  int64_t     m_PTS;
  int64_t     m_DTS;

  int FindHeaders(uint8_t *buf, int buf_size);

public:
  cParserMPEG2Audio(int pID, cTSStream *stream, sPtsWrap *ptsWrap, bool observePtsWraps);
  virtual ~cParserMPEG2Audio();

  virtual void Parse(sStreamPacket *pkt);
};


#endif // VNSI_DEMUXER_MPEGAUDIO_H
