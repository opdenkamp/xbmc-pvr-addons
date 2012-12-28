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

#include "demuxer_AAC.h"

static int aac_sample_rates[16] =
{
  96000, 88200, 64000, 48000, 44100, 32000,
  24000, 22050, 16000, 12000, 11025, 8000, 7350
};


cParserAAC::cParserAAC(cTSDemuxer *demuxer, cLiveStreamer *streamer, int pID)
 : cParser(streamer, pID)
{
  m_demuxer                   = demuxer;
  m_streamBuffer              = NULL;
  m_streamBufferSize          = 0;
  m_streamBufferDataSize      = 0;
  m_streamParserPtr           = 0;
  m_firstPUSIseen             = false;

  m_Configured                = false;
  m_FrameLengthType           = 0;
  m_SampleRate                = 0;
}

cParserAAC::~cParserAAC()
{
  if (m_streamBuffer)
    free(m_streamBuffer);
}

void cParserAAC::Parse(unsigned char *data, int size, bool pusi)
{
  if (pusi)
  {
    /* Payload unit start */
    m_firstPUSIseen   = true;
    m_streamBufferDataSize = 0;
    m_streamParserPtr = 0;
  }

  if (!m_firstPUSIseen)
    return;

  if (m_streamBuffer == NULL)
  {
    m_streamBufferSize = 4000;
    m_streamBuffer = (uint8_t*)malloc(m_streamBufferSize);
  }

  if(m_streamBufferDataSize + size >= m_streamBufferSize)
  {
    m_streamBufferSize += size * 4;
    m_streamBuffer = (uint8_t*)realloc(m_streamBuffer, m_streamBufferSize);
  }

  memcpy(m_streamBuffer + m_streamBufferDataSize, data, size);
  m_streamBufferDataSize += size;

  if (m_streamParserPtr == 0)
  {
    if (m_streamBufferDataSize < 9)
      return;

    int hlen = ParsePESHeader(data, size);
    if (hlen < 0)
      return;

    m_streamParserPtr += hlen;
  }

  int p = m_streamParserPtr;

  int l;
  if (m_demuxer->Type() == stAACLATM)
  {
    while ((l = m_streamBufferDataSize - p) > 3)
    {
      cBitstream bs(&m_streamBuffer[p], l * 8);
      if(bs.readBits(11) == 0x2B7)
      {
        int muxlen = bs.readBits(13);

        if(l < muxlen + 3)
          break;

        ParseLATMAudioMuxElement(m_streamBuffer + p + 3, muxlen);
        p += muxlen + 3;
      }
      else
      {
        p++;
      }
    }
  }
  else if (m_demuxer->Type() == stAACADST)
  {
    while ((l = m_streamBufferDataSize - p) > 3)
    {
      if(m_streamBuffer[p] == 0xFF && (m_streamBuffer[p + 1] & 0xF0) == 0xF0)
      {
        // need at least 7 bytes for header
        if (l < 7)
          break;

        cBitstream bs(&m_streamBuffer[p], l * 8);
        bs.skipBits(15);

        // check if CRC is present, means header is 9 byte long
        int noCrc = bs.readBits(1);
        if (!noCrc && (l < 9))
          break;

        bs.skipBits(2); // profile
        m_SampleRateIndex = bs.readBits(4);
        bs.skipBits(1); // private
        m_ChannelConfig = bs.readBits(3);
        bs.skipBits(4);

        int frameLen = bs.readBits(13);

        if (l < frameLen)
          break;

        m_SampleRate    = aac_sample_rates[m_SampleRateIndex & 0x0E];
        if (m_SampleRate)
          m_FrameDuration = 1024 * 90000 / m_SampleRate;

        m_demuxer->SetAudioInformation(m_ChannelConfig, m_SampleRate, 0, 0, 0);

        sStreamPacket pkt;
        pkt.id       = m_pID;
        pkt.size     = frameLen;
        pkt.data     = &m_streamBuffer[p];
        pkt.dts      = m_curDTS;
        pkt.pts      = m_curDTS;
        pkt.duration = m_FrameDuration;

        m_curDTS += m_FrameDuration;
        SendPacket(&pkt);

        p += frameLen;
      }
      else
      {
        p++;
      }
    }
  }
  m_streamParserPtr = p;
}

void cParserAAC::ParseLATMAudioMuxElement(uint8_t *data, int len)
{
  cBitstream bs(data, len * 8);

  if (!bs.readBits1())
    ReadStreamMuxConfig(&bs);

  if (!m_Configured)
    return;

  if (m_FrameLengthType != 0)
    return;

  int tmp;
  unsigned int slotLen = 0;
  do
  {
    tmp = bs.readBits(8);
    slotLen += tmp;
  } while (tmp == 255);

  if (slotLen * 8 > bs.remainingBits())
    return;

  if (m_curDTS == DVD_NOPTS_VALUE)
    return;

  sStreamPacket pkt;
  pkt.id       = m_pID;
  pkt.size     = len + 3;
  pkt.data     = data-3;
  pkt.dts      = m_curDTS;
  pkt.pts      = m_curDTS;
  pkt.duration = m_FrameDuration;

  m_curDTS += m_FrameDuration;

  SendPacket(&pkt);
  return;
}

void cParserAAC::ReadStreamMuxConfig(cBitstream *bs)
{
  int AudioMuxVersion = bs->readBits(1);
  m_AudioMuxVersion_A = 0;
  if (AudioMuxVersion)                       // audioMuxVersion
    m_AudioMuxVersion_A = bs->readBits(1);

  if(m_AudioMuxVersion_A)
    return;

  if (AudioMuxVersion)
    LATMGetValue(bs);                      // taraFullness

  bs->skipBits(1);                         // allStreamSameTimeFraming = 1
  bs->skipBits(6);                         // numSubFrames = 0
  bs->skipBits(4);                         // numPrograms = 0

  // for each program (which there is only on in DVB)
  bs->skipBits(3);                         // numLayer = 0

  // for each layer (which there is only on in DVB)
  if (!AudioMuxVersion)
    ReadAudioSpecificConfig(bs);
  else
    return;

  // these are not needed... perhaps
  m_FrameLengthType = bs->readBits(3);
  switch (m_FrameLengthType)
  {
    case 0:
      bs->readBits(8);
      break;
    case 1:
      bs->readBits(9);
      break;
    case 3:
    case 4:
    case 5:
      bs->readBits(6);                 // celp_table_index
      break;
    case 6:
    case 7:
      bs->readBits(1);                 // hvxc_table_index
      break;
  }

  if (bs->readBits(1))
  {                   // other data?
    if (AudioMuxVersion == 1)
    {
      LATMGetValue(bs);              // other_data_bits
    }
    else
    {
      int esc;
      do
      {
        esc = bs->readBits(1);
        bs->skipBits(8);
      } while (esc);
    }
  }

  if (bs->readBits(1))                   // crc present?
    bs->skipBits(8);                     // config_crc
  m_Configured = true;
}

void cParserAAC::ReadAudioSpecificConfig(cBitstream *bs)
{
  int aot = bs->readBits(5);
  if (aot == 31)
    aot = 32 + bs->readBits(6);

  m_SampleRateIndex = bs->readBits(4);

  if (m_SampleRateIndex == 0xf)
    m_SampleRate = bs->readBits(24);
  else
    m_SampleRate = aac_sample_rates[m_SampleRateIndex & 0xf];

  if (m_SampleRate)
    m_FrameDuration = 1024 * 90000 / m_SampleRate;
  m_ChannelConfig = bs->readBits(4);

  if (aot == 5) { // AOT_SBR
    if (bs->readBits(4) == 0xf) { // extensionSamplingFrequencyIndex
      bs->skipBits(24);
    }
    aot = bs->readBits(5); // this is the main object type (i.e. non-extended)
    if (aot == 31)
      aot = 32 + bs->readBits(6);
  }

  if(aot != 2)
    return;

  bs->skipBits(1);      //framelen_flag
  if (bs->readBits1())  // depends_on_coder
    bs->skipBits(14);

  if (bs->readBits(1))  // ext_flag
    bs->skipBits(1);    // ext3_flag

  m_demuxer->SetAudioInformation(m_ChannelConfig, m_SampleRate, 0, 0, 0);
}
