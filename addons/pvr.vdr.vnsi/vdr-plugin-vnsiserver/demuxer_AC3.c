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

#include <stdlib.h>
#include <assert.h>
#include "config.h"

#include "demuxer_AC3.h"
#include "bitstream.h"

#define AC3_HEADER_SIZE 7

/** Channel mode (audio coding mode) */
typedef enum
{
  AC3_CHMODE_DUALMONO = 0,
  AC3_CHMODE_MONO,
  AC3_CHMODE_STEREO,
  AC3_CHMODE_3F,
  AC3_CHMODE_2F1R,
  AC3_CHMODE_3F1R,
  AC3_CHMODE_2F2R,
  AC3_CHMODE_3F2R
} AC3ChannelMode;

/* possible frequencies */
const uint16_t AC3SampleRateTable[3] = { 48000, 44100, 32000 };

/* possible bitrates */
const uint16_t AC3BitrateTable[19] = {
    32, 40, 48, 56, 64, 80, 96, 112, 128,
    160, 192, 224, 256, 320, 384, 448, 512, 576, 640
};

const uint8_t AC3ChannelsTable[8] = {
    2, 1, 2, 3, 3, 4, 4, 5
};

const uint16_t AC3FrameSizeTable[38][3] = {
    { 64,   69,   96   },
    { 64,   70,   96   },
    { 80,   87,   120  },
    { 80,   88,   120  },
    { 96,   104,  144  },
    { 96,   105,  144  },
    { 112,  121,  168  },
    { 112,  122,  168  },
    { 128,  139,  192  },
    { 128,  140,  192  },
    { 160,  174,  240  },
    { 160,  175,  240  },
    { 192,  208,  288  },
    { 192,  209,  288  },
    { 224,  243,  336  },
    { 224,  244,  336  },
    { 256,  278,  384  },
    { 256,  279,  384  },
    { 320,  348,  480  },
    { 320,  349,  480  },
    { 384,  417,  576  },
    { 384,  418,  576  },
    { 448,  487,  672  },
    { 448,  488,  672  },
    { 512,  557,  768  },
    { 512,  558,  768  },
    { 640,  696,  960  },
    { 640,  697,  960  },
    { 768,  835,  1152 },
    { 768,  836,  1152 },
    { 896,  975,  1344 },
    { 896,  976,  1344 },
    { 1024, 1114, 1536 },
    { 1024, 1115, 1536 },
    { 1152, 1253, 1728 },
    { 1152, 1254, 1728 },
    { 1280, 1393, 1920 },
    { 1280, 1394, 1920 },
};

const uint8_t EAC3Blocks[4] = {
  1, 2, 3, 6
};

typedef enum {
  EAC3_FRAME_TYPE_INDEPENDENT = 0,
  EAC3_FRAME_TYPE_DEPENDENT,
  EAC3_FRAME_TYPE_AC3_CONVERT,
  EAC3_FRAME_TYPE_RESERVED
} EAC3FrameType;

cParserAC3::cParserAC3(cTSDemuxer *demuxer, cLiveStreamer *streamer, int pID)
 : cParser(streamer, pID)
{
  m_PTS                       = 0;
  m_DTS                       = 0;
  m_FrameSize                 = 0;
  m_SampleRate                = 0;
  m_Channels                  = 0;
  m_BitRate                   = 0;
  m_demuxer                   = demuxer;
  m_firstPUSIseen             = false;
  m_PESStart                  = false;
  m_NextDTS                   = 0;
  m_AC3BufferPtr              = 0;
  m_HeaderFound               = false;
}

cParserAC3::~cParserAC3()
{
}

void cParserAC3::Parse(unsigned char *data, int size, bool pusi)
{
  if (pusi)
  {
    /* Payload unit start */
    m_PESStart      = true;
    m_firstPUSIseen = true;
  }

  /* Wait for first pusi */
  if (!m_firstPUSIseen)
    return;

  if (m_PESStart)
  {
    if (!PesIsPS1Packet(data))
    {
      ERRORLOG("AC3 PES packet contains no valid private stream 1, ignored this packet");
      m_firstPUSIseen = false;
      return;
    }

    int hlen = ParsePESHeader(data, size);
    if (hlen <= 0)
      return;

    data += hlen;
    size -= hlen;

    m_PESStart = false;

    assert(size >= 0);
    if(size == 0)
      return;
  }

  while (size > 0)
  {
    uint8_t *outbuf;
    int      outlen;

    int rlen = FindHeaders(&outbuf, &outlen, data, size);
    if (rlen < 0)
      break;

    if (outlen)
    {
      sStreamPacket pkt;
      pkt.id       = m_pID;
      pkt.data     = outbuf;
      pkt.size     = outlen;
      pkt.duration = 90000 * 1536 / m_SampleRate;
      pkt.dts      = m_DTS;
      pkt.pts      = pkt.dts;
      m_NextDTS    = pkt.dts + pkt.duration;

      m_demuxer->SetAudioInformation(m_Channels, m_SampleRate, m_BitRate, 0, 0);

      SendPacket(&pkt);
    }
    data += rlen;
    size -= rlen;
  }
}

int cParserAC3::FindHeaders(uint8_t **poutbuf, int *poutbuf_size,
                                   uint8_t *buf, int buf_size)
{
  *poutbuf      = NULL;
  *poutbuf_size = 0;

  uint8_t *buf_ptr = buf;
  while (buf_size > 0)
  {
    if (!m_HeaderFound && (buf_size >= 9) &&
        (buf_ptr[0] == 0x0b && buf_ptr[1] == 0x77))
    {
      cBitstream bs(buf_ptr + 2, AC3_HEADER_SIZE * 8);

      /* read ahead to bsid to distinguish between AC-3 and E-AC-3 */
      int bsid = bs.showBits(29) & 0x1F;
      if (bsid > 16)
        return -1;

      if (bsid <= 10)
      {
        /* Normal AC-3 */
        bs.skipBits(16);
        int fscod       = bs.readBits(2);
        int frmsizecod  = bs.readBits(6);
        bs.skipBits(5); // skip bsid, already got it
        bs.skipBits(3); // skip bitstream mode
        int acmod       = bs.readBits(3);

        if (fscod == 3 || frmsizecod > 37)
          return -1;

        if (acmod == AC3_CHMODE_STEREO)
        {
          bs.skipBits(2); // skip dsurmod
        }
        else
        {
          if ((acmod & 1) && acmod != AC3_CHMODE_MONO)
            bs.skipBits(2);
          if (acmod & 4)
            bs.skipBits(2);
        }
        int lfeon = bs.readBits(1);

        int srShift   = max(bsid, 8) - 8;
        m_SampleRate  = AC3SampleRateTable[fscod] >> srShift;
        m_BitRate     = (AC3BitrateTable[frmsizecod>>1] * 1000) >> srShift;
        m_Channels    = AC3ChannelsTable[acmod] + lfeon;
        m_FrameSize   = AC3FrameSizeTable[frmsizecod][fscod] * 2;
      }
      else
      {
        /* Enhanced AC-3 */
        int frametype = bs.readBits(2);
        if (frametype == EAC3_FRAME_TYPE_RESERVED)
          return -1;

        /*int substreamid =*/ bs.readBits(3);

        m_FrameSize = (bs.readBits(11) + 1) << 1;
        if (m_FrameSize < AC3_HEADER_SIZE)
          return -1;

        int numBlocks = 6;
        int sr_code = bs.readBits(2);
        if (sr_code == 3)
        {
          int sr_code2 = bs.readBits(2);
          if (sr_code2 == 3)
            return -1;
          m_SampleRate = AC3SampleRateTable[sr_code2] / 2;
        }
        else
        {
          numBlocks = EAC3Blocks[bs.readBits(2)];
          m_SampleRate = AC3SampleRateTable[sr_code];
        }

        int channelMode = bs.readBits(3);
        int lfeon = bs.readBits(1);

        m_BitRate  = (uint32_t)(8.0 * m_FrameSize * m_SampleRate / (numBlocks * 256.0));
        m_Channels = AC3ChannelsTable[channelMode] + lfeon;
      }
      m_HeaderFound = true;
      m_DTS = m_curDTS;
      if (m_DTS == DVD_NOPTS_VALUE)
        m_DTS = m_NextDTS;
      m_curDTS = DVD_NOPTS_VALUE;
    }

    if (m_HeaderFound)
    {
      if (m_AC3BufferPtr > AC3_MAX_CODED_FRAME_SIZE - 1)
      {
        ERRORLOG("error in AC3 frame size");
        m_AC3BufferPtr = 0;
        m_HeaderFound = false;
      }
      else
        m_AC3Buffer[m_AC3BufferPtr++] = buf_ptr[0];
    }

    if (m_FrameSize && m_AC3BufferPtr >= m_FrameSize)
    {
      *poutbuf        = m_AC3Buffer;
      *poutbuf_size   = m_AC3BufferPtr;

      m_AC3BufferPtr  = 0;
      m_HeaderFound   = false;
      break;
    }

    buf_ptr++;
    buf_size--;
  }
  return (buf_ptr - buf);
}
