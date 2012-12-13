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

#ifndef VNSI_DEMUXER_AC3_H
#define VNSI_DEMUXER_AC3_H

#include "demuxer.h"

// --- cParserAC3 -------------------------------------------------

class cParserAC3 : public cParser
{
private:
  cTSDemuxer *m_demuxer;
  bool        m_firstPUSIseen;
  bool        m_PESStart;
  int         m_SampleRate;
  int         m_Channels;
  int         m_BitRate;

  int64_t     m_PTS;                /* pts of the current frame */
  int64_t     m_DTS;                /* dts of the current frame */
  int64_t     m_NextDTS;

  int         m_FrameSize;
  bool        m_HeaderFound;

#define AC3_MAX_CODED_FRAME_SIZE 1920*2
  uint8_t     m_AC3Buffer[AC3_MAX_CODED_FRAME_SIZE];    /* input buffer */
  int         m_AC3BufferSize;
  int         m_AC3BufferPtr;

  int FindHeaders(uint8_t **poutbuf, int *poutbuf_size,
                  uint8_t *buf, int buf_size);

public:
  cParserAC3(cTSDemuxer *demuxer, cLiveStreamer *streamer, int pID);
  virtual ~cParserAC3();

  virtual void Parse(unsigned char *data, int size, bool pusi);
};


#endif // VNSI_DEMUXER_AC3_H
