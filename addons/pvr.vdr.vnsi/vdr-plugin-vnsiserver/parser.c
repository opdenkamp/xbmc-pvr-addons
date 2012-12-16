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
#include <vdr/remux.h>
#include <vdr/channels.h>
#include "config.h"
#include "parser.h"
#include "parser_AAC.h"
#include "parser_AC3.h"
#include "parser_DTS.h"
#include "parser_h264.h"
#include "parser_MPEGAudio.h"
#include "parser_MPEGVideo.h"
#include "parser_Subtitle.h"
#include "parser_Teletext.h"

#define PTS_MASK 0x1ffffffffLL
//#define PTS_MASK 0x7ffffLL

#ifndef INT64_MIN
#define INT64_MIN       (-0x7fffffffffffffffLL-1)
#endif

// --- cParser -------------------------------------------------

cParser::cParser(int pID, cTSStream *stream)
 : m_pID(pID)
{
  m_curPTS    = DVD_NOPTS_VALUE;
  m_curDTS    = DVD_NOPTS_VALUE;
  m_prevDTS   = DVD_NOPTS_VALUE;
  m_PesBuffer = NULL;
  m_IsError = false;
  m_Stream = stream;
  m_IsVideo = false;
  m_PesBufferInitialSize = 1024;
  Reset();
}

cParser::~cParser()
{
  if (m_PesBuffer)
    free(m_PesBuffer);
}


/*
 * Extract DTS and PTS and update current values in stream
 */
int cParser::ParsePESHeader(uint8_t *buf, size_t len)
{
  if (len < 6)
    return -1;

  m_PesPacketLength = buf[4] << 8 | buf[5];

  if (!PesIsVideoPacket(buf) && !PesIsAudioPacket(buf))
    return 6;

  if (len < 9)
    return -1;

  unsigned int hdr_len = PesHeaderLength(buf);

  if (m_PesPacketLength > 0)
    m_PesPacketLength -= (hdr_len-6);

  // scrambled
  if ((buf[6] & 0x30) != 0)
    return hdr_len;

  // parse PTS
  if ((hdr_len >= 13) &&
      ((((buf[7] & 0xC0) == 0x80) && ((buf[9] & 0xF0) == 0x20)) ||
        ((buf[7] & 0xC0) == 0xC0) && ((buf[9] & 0xF0) == 0x30)))
  {
    int64_t pts;
    pts  = ((int64_t)(buf[ 9] & 0x0E)) << 29 ;
    pts |= ((int64_t) buf[10])         << 22 ;
    pts |= ((int64_t)(buf[11] & 0xFE)) << 14 ;
    pts |= ((int64_t) buf[12])         <<  7 ;
    pts |= ((int64_t)(buf[13] & 0xFE)) >>  1 ;
    m_curPTS = pts;
  }
  else
    return hdr_len;

  m_prevDTS = m_curDTS;

  // parse DTS
  if ((hdr_len >= 18) &&
      ((buf[7] & 0xC0) == 0xC0) && ((buf[14] & 0xF0) == 0x10))
  {
    int64_t dts;
    dts  = ((int64_t)( buf[14] & 0x0E)) << 29 ;
    dts |=  (int64_t)( buf[15]          << 22 );
    dts |=  (int64_t)((buf[16] & 0xFE)  << 14 );
    dts |=  (int64_t)( buf[17]          <<  7 );
    dts |=  (int64_t)((buf[18] & 0xFE)  >>  1 );
    m_curDTS = dts;
  }
  else
    m_curDTS = m_curPTS;

  return hdr_len;
}

int cParser::ParsePacketHeader(uint8_t *data)
{
  m_IsPusi  = TsPayloadStart(data);
  int  bytes = TS_SIZE - TsPayloadOffset(data);

  if(bytes < 0 || bytes > TS_SIZE)
    return -1;

  if (TsError(data))
  {
    ERRORLOG("transport error");
    return -1;
  }

  if (!TsHasPayload(data))
  {
    DEBUGLOG("no payload, size %d", bytes);
    return 0;
  }

  /* drop broken PES packets */
  if (m_IsError && !m_IsPusi)
  {
    return -1;
  }

  /* handle new payload unit */
  if (m_IsPusi)
  {
    if (!PesIsHeader(data+TS_SIZE-bytes))
    {
      m_IsError = true;
//      ERRORLOG("--------------- no header");
      Reset();
      return -1;
    }
    m_IsError = false;
  }

  return bytes;
}

bool cParser::AddPESPacket(uint8_t *data, int size)
{
  // check for beginning of a PES packet
  if (m_IsPusi)
  {
    if (IsValidStartCode(data, size))
    {
      m_firstPUSIseen = true;
      int hlen = ParsePESHeader(data, size);
      if (hlen <= 0)
      {
        INFOLOG("--------- reset");
        Reset();
        return false;
      }
      data += hlen;
      size -= hlen;
    }
    else
    {
      INFOLOG("--------- reset 2");
      Reset();
    }
  }

  if (!m_firstPUSIseen)
    return false;

  if (m_PesBuffer == NULL)
  {
    m_PesBufferSize = m_PesBufferInitialSize;
    m_PesBuffer = (uint8_t*)malloc(m_PesBufferSize);
    if (m_PesBuffer == NULL)
    {
      ERRORLOG("cParser::AddPESPacket - malloc failed");
      Reset();
      return false;
    }
  }

  if (m_PesBufferPtr + size >= m_PesBufferSize)
  {
    if (m_PesBufferPtr + size >= 500000)
    {
      ERRORLOG("cParser::AddPESPacket - max buffer size reached");
      Reset();
      return false;
    }
    m_PesBufferSize += m_PesBufferInitialSize / 10;
    m_PesBuffer = (uint8_t*)realloc(m_PesBuffer, m_PesBufferSize);
    if (m_PesBuffer == NULL)
    {
      ERRORLOG("cParser::AddPESPacket - realloc failed");
      Reset();
      return false;
    }
  }

  // copy first packet of new frame to front
  if (m_PesNextFramePtr)
  {
    memmove(m_PesBuffer, m_PesBuffer+m_PesNextFramePtr, m_PesBufferPtr-m_PesNextFramePtr);
    m_PesBufferPtr = m_PesBufferPtr-m_PesNextFramePtr;
    m_PesNextFramePtr = 0;
  }

  // copy payload
  memcpy(m_PesBuffer+m_PesBufferPtr, data, size);
  m_PesBufferPtr += size;

  return false;
}

inline bool cParser::IsValidStartCode(uint8_t *buf, int size)
{
  if (size < 4)
    return false;

  uint32_t startcode = buf[0] << 24 | buf[1] << 16 | buf[2] << 8 | buf[3];
  if (m_Stream->Type() == stH264 || m_Stream->Type() == stMPEG2VIDEO)
  {
    if (startcode >= 0x000001e0 && startcode <= 0x000001ef)
      return true;
  }
  else if (m_Stream->Type() == stAC3 ||
      m_Stream->Type() == stEAC3)
  {
    if (PesIsPS1Packet(buf))
      return true;
  }
  else if (m_Stream->Type() == stMPEG2AUDIO ||
      m_Stream->Type() == stAACADTS ||
      m_Stream->Type() == stAACLATM ||
      m_Stream->Type() == stDTS)
  {
    if (startcode >= 0x000001c0 && startcode <= 0x000001df)
      return true;
  }
  else if (m_Stream->Type() == stTELETEXT)
  {
    if (PesIsPS1Packet(buf))
      return true;
  }
  else if (m_Stream->Type() == stDVBSUB ||
      m_Stream->Type() == stTEXTSUB)
  {
    if (startcode == 0x000001bd ||
        startcode == 0x000001bf ||
        (startcode >= 0x000001f0 && startcode <= 0x000001f9))
      return true;
  }
  return false;
}

void cParser::Reset()
{
  m_PesBufferPtr = 0;
  m_PesParserPtr = 0;
  m_firstPUSIseen = false;
  m_PesNextFramePtr = 0;
  m_FoundFrame = false;
  m_PesPacketLength = 0;
}

// --- cTSStream ----------------------------------------------------

cTSStream::cTSStream(eStreamType type, int pid)
  : m_streamType(type)
  , m_pID(pid)
{
  m_pesError        = false;
  m_pesParser       = NULL;
  m_language[0]     = 0;
  m_FpsScale        = 0;
  m_FpsRate         = 0;
  m_Height          = 0;
  m_Width           = 0;
  m_Aspect          = 0.0f;
  m_Channels        = 0;
  m_SampleRate      = 0;
  m_BitRate         = 0;
  m_BitsPerSample   = 0;
  m_BlockAlign      = 0;
  m_IsStreamChange  = false;

  if (m_streamType == stMPEG2VIDEO)
    m_pesParser = new cParserMPEG2Video(m_pID, this);
  else if (m_streamType == stH264)
    m_pesParser = new cParserH264(m_pID, this);
  else if (m_streamType == stMPEG2AUDIO)
    m_pesParser = new cParserMPEG2Audio(m_pID, this);
  else if (m_streamType == stAACADTS)
    m_pesParser = new cParserAAC(m_pID, this);
  else if (m_streamType == stAACLATM)
    m_pesParser = new cParserAAC(m_pID, this);
  else if (m_streamType == stAC3)
    m_pesParser = new cParserAC3(m_pID, this);
  else if (m_streamType == stEAC3)
    m_pesParser = new cParserAC3(m_pID, this);
  else if (m_streamType == stDTS)
    m_pesParser = new cParserDTS(m_pID, this);
  else if (m_streamType == stTELETEXT)
    m_pesParser = new cParserTeletext(m_pID, this);
  else if (m_streamType == stDVBSUB)
    m_pesParser = new cParserSubtitle(m_pID, this);
  else
  {
    ERRORLOG("Unrecognised type %i inside stream %i", m_streamType, m_pID);
    return;
  }
}

cTSStream::~cTSStream()
{
  if (m_pesParser)
  {
    delete m_pesParser;
    m_pesParser = NULL;
  }
}

bool cTSStream::ProcessTSPacket(uint8_t *data, sStreamPacket *pkt, bool iframe)
{
  if (!data)
    return false;

  if (!m_pesParser)
    return false;

  int payloadSize = m_pesParser->ParsePacketHeader(data);
  if (payloadSize < 0)
    return false;

  m_pesParser->AddPESPacket(data+TS_SIZE-payloadSize, payloadSize);
  m_pesParser->Parse(pkt);
  if (iframe && !m_pesParser->IsVideo())
    return false;

  if (pkt->data)
  {
    int64_t dts = pkt->dts;
    int64_t pts = pkt->pts;

    if (dts == DVD_NOPTS_VALUE)
      dts = pts;

    // Rescale for XBMC
    pkt->dts      = Rescale(dts);
    pkt->pts      = Rescale(pts);
    pkt->duration = Rescale(pkt->duration);
    return true;
  }

  return false;
}

int64_t cTSStream::Rescale(int64_t a)
{
  uint64_t b = DVD_TIME_BASE;
  uint64_t c = 90000;
  uint64_t r = c/2;

  if (b<=INT_MAX && c<=INT_MAX){
    if (a<=INT_MAX)
      return (a * b + r)/c;
    else
      return a/c*b + (a%c*b + r)/c;
  }
  else
  {
    uint64_t a0= a&0xFFFFFFFF;
    uint64_t a1= a>>32;
    uint64_t b0= b&0xFFFFFFFF;
    uint64_t b1= b>>32;
    uint64_t t1= a0*b1 + a1*b0;
    uint64_t t1a= t1<<32;

    a0 = a0*b0 + t1a;
    a1 = a1*b1 + (t1>>32) + (a0<t1a);
    a0 += r;
    a1 += a0<r;

    for (int i=63; i>=0; i--)
    {
      a1+= a1 + ((a0>>i)&1);
      t1+=t1;
      if (c <= a1)
      {
        a1 -= c;
        t1++;
      }
    }
    return t1;
  }
}

void cTSStream::SetLanguage(const char *language)
{
  m_language[0] = language[0];
  m_language[1] = language[1];
  m_language[2] = language[2];
  m_language[3] = 0;
}

bool cTSStream::SetVideoInformation(int FpsScale, int FpsRate, int Height, int Width, float Aspect)
{
  if ((m_FpsScale != FpsScale) ||
      (m_FpsRate != FpsRate) ||
      (m_Height != Height) ||
      (m_Width != Width) ||
      (m_Aspect != Aspect))
    m_IsStreamChange = true;

  m_FpsScale        = FpsScale;
  m_FpsRate         = FpsRate;
  m_Height          = Height;
  m_Width           = Width;
  m_Aspect          = Aspect;

  return m_IsStreamChange;
}

void cTSStream::GetVideoInformation(uint32_t &FpsScale, uint32_t &FpsRate, uint32_t &Height, uint32_t &Width, double &Aspect)
{
  FpsScale = m_FpsScale;
  FpsRate = m_FpsRate;
  Height = m_Height;
  Width = m_Width;
  Aspect = m_Aspect;

  m_IsStreamChange = false;
}

bool cTSStream::SetAudioInformation(int Channels, int SampleRate, int BitRate, int BitsPerSample, int BlockAlign)
{
  if ((m_Channels != Channels) ||
      (m_SampleRate != SampleRate) ||
      (m_BlockAlign != BlockAlign) ||
      (m_BitRate != BitRate) ||
      (m_BitsPerSample != BitsPerSample))
    m_IsStreamChange = true;

  m_Channels        = Channels;
  m_SampleRate      = SampleRate;
  m_BlockAlign      = BlockAlign;
  m_BitRate         = BitRate;
  m_BitsPerSample   = BitsPerSample;

  return m_IsStreamChange;
}

void cTSStream::GetAudioInformation(uint32_t &Channels, uint32_t &SampleRate, uint32_t &BitRate, uint32_t &BitsPerSample, uint32_t &BlockAlign)
{
  Channels = m_Channels;
  SampleRate = m_SampleRate;
  BlockAlign = m_BlockAlign;
  BitRate = m_BitRate;
  BitsPerSample = m_BitsPerSample;

  m_IsStreamChange = false;
}

void cTSStream::SetSubtitlingDescriptor(unsigned char SubtitlingType, uint16_t CompositionPageId, uint16_t AncillaryPageId)
{
  m_subtitlingType    = SubtitlingType;
  m_compositionPageId = CompositionPageId;
  m_ancillaryPageId   = AncillaryPageId;
}
