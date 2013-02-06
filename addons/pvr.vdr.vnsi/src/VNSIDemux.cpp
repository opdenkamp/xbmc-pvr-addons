/*
 *      Copyright (C) 2010 Alwin Esch (Team XBMC)
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

#include <stdint.h>
#include <limits.h>
#include <string.h>
#include "avcodec.h" // For codec id's
#include "VNSIDemux.h"
#include "responsepacket.h"
#include "requestpacket.h"
#include "vnsicommand.h"

using namespace ADDON;

cVNSIDemux::cVNSIDemux()
{
  for (unsigned int i = 0; i < PVR_STREAM_MAX_STREAMS; i++)
    m_Streams.stream[i].iCodecType = AVMEDIA_TYPE_UNKNOWN;
  m_Streams.iStreamCount = 0;
  m_StreamIndex.clear();
}

cVNSIDemux::~cVNSIDemux()
{
}

bool cVNSIDemux::OpenChannel(const PVR_CHANNEL &channelinfo)
{
  m_channelinfo = channelinfo;
  if(!cVNSISession::Open(g_szHostname, g_iPort))
    return false;

  if(!cVNSISession::Login())
    return false;

  return SwitchChannel(m_channelinfo);
}

bool cVNSIDemux::GetStreamProperties(PVR_STREAM_PROPERTIES* props)
{
  props->iStreamCount = m_Streams.iStreamCount;
  for (unsigned int i = 0; i < m_Streams.iStreamCount; i++)
  {
    props->stream[i].iPhysicalId     = m_Streams.stream[i].iPhysicalId;
    props->stream[i].iCodecType      = m_Streams.stream[i].iCodecType;
    props->stream[i].iCodecId        = m_Streams.stream[i].iCodecId;
    props->stream[i].strLanguage[0]  = m_Streams.stream[i].strLanguage[0];
    props->stream[i].strLanguage[1]  = m_Streams.stream[i].strLanguage[1];
    props->stream[i].strLanguage[2]  = m_Streams.stream[i].strLanguage[2];
    props->stream[i].strLanguage[3]  = m_Streams.stream[i].strLanguage[3];
    props->stream[i].iIdentifier     = m_Streams.stream[i].iIdentifier;
    props->stream[i].iFPSScale       = m_Streams.stream[i].iFPSScale;
    props->stream[i].iFPSRate        = m_Streams.stream[i].iFPSRate;
    props->stream[i].iHeight         = m_Streams.stream[i].iHeight;
    props->stream[i].iWidth          = m_Streams.stream[i].iWidth;
    props->stream[i].fAspect         = m_Streams.stream[i].fAspect;
    props->stream[i].iChannels       = m_Streams.stream[i].iChannels;
    props->stream[i].iSampleRate     = m_Streams.stream[i].iSampleRate;
    props->stream[i].iBlockAlign     = m_Streams.stream[i].iBlockAlign;
    props->stream[i].iBitRate        = m_Streams.stream[i].iBitRate;
    props->stream[i].iBitsPerSample  = m_Streams.stream[i].iBitsPerSample;


  }
  return (props->iStreamCount > 0);
}

void cVNSIDemux::Abort()
{
  for (unsigned int i = 0; i < PVR_STREAM_MAX_STREAMS; i++)
    m_Streams.stream[i].iCodecType = AVMEDIA_TYPE_UNKNOWN;
  m_Streams.iStreamCount = 0;
  m_StreamIndex.clear();
}

DemuxPacket* cVNSIDemux::Read()
{
  if(ConnectionLost())
  {
    return NULL;
  }

  cResponsePacket *resp = ReadMessage(1000,1000);

  if(resp == NULL)
    return PVR->AllocateDemuxPacket(0);

  if (resp->getChannelID() != VNSI_CHANNEL_STREAM)
  {
    delete resp;
    return NULL;
  }

  if (resp->getOpCodeID() == VNSI_STREAM_CHANGE)
  {
    StreamChange(resp);
    DemuxPacket* pkt = PVR->AllocateDemuxPacket(0);
    pkt->iStreamId  = DMX_SPECIALID_STREAMCHANGE;
    delete resp;
    return pkt;
  }
  else if (resp->getOpCodeID() == VNSI_STREAM_STATUS)
  {
    StreamStatus(resp);
  }
  else if (resp->getOpCodeID() == VNSI_STREAM_SIGNALINFO)
  {
    StreamSignalInfo(resp);
  }
  else if (resp->getOpCodeID() == VNSI_STREAM_CONTENTINFO)
  {
    // send stream updates only if there are changes
    if(StreamContentInfo(resp))
    {
      DemuxPacket* pkt = PVR->AllocateDemuxPacket(0);
      pkt->iStreamId  = DMX_SPECIALID_STREAMCHANGE;
      delete resp;
      return pkt;
    }
  }
  else if (resp->getOpCodeID() == VNSI_STREAM_MUXPKT)
  {
    // figure out the stream id for this packet
    int iStreamId = -1;
    std::map<int,unsigned int>::iterator it = m_StreamIndex.find(resp->getStreamID());
    if (it != m_StreamIndex.end())
      iStreamId = it->second;

    // stream found ?
    if(iStreamId != -1 && resp->getMuxSerial() == m_MuxPacketSerial)
    {
      DemuxPacket* p = (DemuxPacket*)resp->getUserData();
      p->iSize      = resp->getUserDataLength();
      p->duration   = (double)resp->getDuration() * DVD_TIME_BASE / 1000000;
      p->dts        = (double)resp->getDTS() * DVD_TIME_BASE / 1000000;
      p->pts        = (double)resp->getPTS() * DVD_TIME_BASE / 1000000;
      p->iStreamId  = iStreamId;
      delete resp;

      return p;
    }
    else if (iStreamId != -1 && resp->getMuxSerial() != m_MuxPacketSerial)
    {
      // ignore silently, may happen after a seek
      XBMC->Log(LOG_DEBUG, "-------------------- serial: %d", resp->getMuxSerial());
    }
    else
    {
      XBMC->Log(LOG_DEBUG, "stream id %i not found", resp->getStreamID());
    }
  }

  delete resp;
  return PVR->AllocateDemuxPacket(0);
}

bool cVNSIDemux::SeekTime(int time, bool backwards, double *startpts)
{
  cRequestPacket vrp;

  int64_t seek_pts = (int64_t)time * 1000;
  if (startpts)
    *startpts = seek_pts;

  if (!vrp.init(VNSI_CHANNELSTREAM_SEEK) ||
      !vrp.add_S64(seek_pts) ||
      !vrp.add_U8(backwards))
  {
    XBMC->Log(LOG_ERROR, "%s - failed to seek1", __FUNCTION__);
    return false;
  }
  cResponsePacket *resp = ReadResult(&vrp);
  if (!resp)
  {
    XBMC->Log(LOG_ERROR, "%s - failed to seek2", __FUNCTION__);
    return false;
  }
  uint32_t retCode = resp->extract_U32();
  uint32_t serial = resp->extract_U32();
  delete resp;

  if (retCode == VNSI_RET_OK)
  {
    m_MuxPacketSerial = serial;
    return true;
  }
  else
    return false;
}

bool cVNSIDemux::SwitchChannel(const PVR_CHANNEL &channelinfo)
{
  XBMC->Log(LOG_DEBUG, "changing to channel %d", channelinfo.iChannelNumber);

  cRequestPacket vrp1;

  if (!vrp1.init(VNSI_GETSETUP) || !vrp1.add_String(CONFNAME_TIMESHIFT))
  {
    XBMC->Log(LOG_ERROR, "%s - failed to get timeshift mode", __FUNCTION__);
    return false;
  }
  cResponsePacket *resp = ReadResult(&vrp1);
  if (!resp)
  {
    XBMC->Log(LOG_ERROR, "%s - failed to get timeshift mode", __FUNCTION__);
    return false;
  }
  m_bTimeshift = resp->extract_U32();
  delete resp;

  cRequestPacket vrp2;
  if (!vrp2.init(VNSI_CHANNELSTREAM_OPEN) || !vrp2.add_U32(channelinfo.iUniqueId) || !ReadSuccess(&vrp2))
  {
    XBMC->Log(LOG_ERROR, "%s - failed to set channel", __FUNCTION__);
    return false;
  }

  m_channelinfo = channelinfo;
  m_Streams.iStreamCount  = 0;
  m_MuxPacketSerial = 0;

  return true;
}

bool cVNSIDemux::GetSignalStatus(PVR_SIGNAL_STATUS &qualityinfo)
{
  if (m_Quality.fe_name.empty())
    return true;

  strncpy(qualityinfo.strAdapterName, m_Quality.fe_name.c_str(), sizeof(qualityinfo.strAdapterName));
  strncpy(qualityinfo.strAdapterStatus, m_Quality.fe_status.c_str(), sizeof(qualityinfo.strAdapterStatus));
  qualityinfo.iSignal = (uint16_t)m_Quality.fe_signal;
  qualityinfo.iSNR = (uint16_t)m_Quality.fe_snr;
  qualityinfo.iBER = (uint32_t)m_Quality.fe_ber;
  qualityinfo.iUNC = (uint32_t)m_Quality.fe_unc;
  qualityinfo.dVideoBitrate = 0;
  qualityinfo.dAudioBitrate = 0;
  qualityinfo.dDolbyBitrate = 0;

  return true;
}

void cVNSIDemux::StreamChange(cResponsePacket *resp)
{
  PVR_STREAM_PROPERTIES streams;
  streams.iStreamCount = 0;
  std::map<int, unsigned int> streamIndex;

  while (!resp->end())
  {
    uint32_t    pid = resp->extract_U32();
    const char* type  = resp->extract_String();

    streamIndex[pid] = streams.iStreamCount;

    streams.stream[streams.iStreamCount].iFPSScale         = 0;
    streams.stream[streams.iStreamCount].iFPSRate          = 0;
    streams.stream[streams.iStreamCount].iHeight           = 0;
    streams.stream[streams.iStreamCount].iWidth            = 0;
    streams.stream[streams.iStreamCount].fAspect           = 0.0;

    streams.stream[streams.iStreamCount].iChannels         = 0;
    streams.stream[streams.iStreamCount].iSampleRate       = 0;
    streams.stream[streams.iStreamCount].iBlockAlign       = 0;
    streams.stream[streams.iStreamCount].iBitRate          = 0;
    streams.stream[streams.iStreamCount].iBitsPerSample    = 0;

    std::map<int,unsigned int>::iterator it = m_StreamIndex.find(pid);
    if (it != m_StreamIndex.end())
    {
      memcpy((void*)&streams.stream[streams.iStreamCount], (void*)&m_Streams.stream[m_StreamIndex[pid]],
          sizeof(PVR_STREAM_PROPERTIES::PVR_STREAM));
    }

    if(!strcmp(type, "AC3"))
    {
      streams.stream[streams.iStreamCount].iPhysicalId     = pid;
      streams.stream[streams.iStreamCount].iCodecType      = AVMEDIA_TYPE_AUDIO;
      streams.stream[streams.iStreamCount].iCodecId        = CODEC_ID_AC3;
    }
    else if(!strcmp(type, "MPEG2AUDIO"))
    {
      streams.stream[streams.iStreamCount].iPhysicalId     = pid;
      streams.stream[streams.iStreamCount].iCodecType      = AVMEDIA_TYPE_AUDIO;
      streams.stream[streams.iStreamCount].iCodecId        = CODEC_ID_MP2;
    }
    else if(!strcmp(type, "AAC"))
    {
      streams.stream[streams.iStreamCount].iPhysicalId     = pid;
      streams.stream[streams.iStreamCount].iCodecType      = AVMEDIA_TYPE_AUDIO;
      streams.stream[streams.iStreamCount].iCodecId        = CODEC_ID_AAC;
    }
    else if(!strcmp(type, "AACLATM"))
    {
      streams.stream[streams.iStreamCount].iPhysicalId     = pid;
      streams.stream[streams.iStreamCount].iCodecType      = AVMEDIA_TYPE_AUDIO;
      streams.stream[streams.iStreamCount].iCodecId        = CODEC_ID_AAC_LATM;
    }
    else if(!strcmp(type, "DTS"))
    {
      streams.stream[streams.iStreamCount].iPhysicalId     = pid;
      streams.stream[streams.iStreamCount].iCodecType      = AVMEDIA_TYPE_AUDIO;
      streams.stream[streams.iStreamCount].iCodecId        = CODEC_ID_DTS;
    }
    else if(!strcmp(type, "EAC3"))
    {
      streams.stream[streams.iStreamCount].iPhysicalId     = pid;
      streams.stream[streams.iStreamCount].iCodecType      = AVMEDIA_TYPE_AUDIO;
      streams.stream[streams.iStreamCount].iCodecId        = CODEC_ID_EAC3;
    }
    else if(!strcmp(type, "MPEG2VIDEO"))
    {
      streams.stream[streams.iStreamCount].iPhysicalId     = pid;
      streams.stream[streams.iStreamCount].iCodecType      = AVMEDIA_TYPE_VIDEO;
      streams.stream[streams.iStreamCount].iCodecId        = CODEC_ID_MPEG2VIDEO;
    }
    else if(!strcmp(type, "H264"))
    {
      streams.stream[streams.iStreamCount].iPhysicalId     = pid;
      streams.stream[streams.iStreamCount].iCodecType      = AVMEDIA_TYPE_VIDEO;
      streams.stream[streams.iStreamCount].iCodecId        = CODEC_ID_H264;
    }
    else if(!strcmp(type, "DVBSUB"))
    {
      streams.stream[streams.iStreamCount].iPhysicalId     = pid;
      streams.stream[streams.iStreamCount].iCodecType      = AVMEDIA_TYPE_SUBTITLE;
      streams.stream[streams.iStreamCount].iCodecId        = CODEC_ID_DVB_SUBTITLE;
    }
    else if(!strcmp(type, "TEXTSUB"))
    {
      streams.stream[streams.iStreamCount].iPhysicalId     = pid;
      streams.stream[streams.iStreamCount].iCodecType      = AVMEDIA_TYPE_SUBTITLE;
      streams.stream[streams.iStreamCount].iCodecId        = CODEC_ID_TEXT;
    }
    else if(!strcmp(type, "TELETEXT"))
    {
      streams.stream[streams.iStreamCount].iPhysicalId     = pid;
      streams.stream[streams.iStreamCount].iCodecType      = AVMEDIA_TYPE_SUBTITLE;
      streams.stream[streams.iStreamCount].iCodecId        = CODEC_ID_DVB_TELETEXT;
    }
    else
    {
      m_Streams.iStreamCount = 0;
      return;
    }

    if (streams.stream[streams.iStreamCount].iCodecType == AVMEDIA_TYPE_AUDIO)
    {
      const char *language = resp->extract_String();

      streams.stream[streams.iStreamCount].iChannels       = resp->extract_U32();
      streams.stream[streams.iStreamCount].iSampleRate     = resp->extract_U32();
      streams.stream[streams.iStreamCount].iBlockAlign     = resp->extract_U32();
      streams.stream[streams.iStreamCount].iBitRate        = resp->extract_U32();
      streams.stream[streams.iStreamCount].iBitsPerSample  = resp->extract_U32();
      streams.stream[streams.iStreamCount].strLanguage[0]  = language[0];
      streams.stream[streams.iStreamCount].strLanguage[1]  = language[1];
      streams.stream[streams.iStreamCount].strLanguage[2]  = language[2];
      streams.stream[streams.iStreamCount].strLanguage[3]  = 0;
      streams.stream[streams.iStreamCount].iIdentifier     = -1;
      streams.iStreamCount++;

      delete[] language;
    }
    else if (streams.stream[streams.iStreamCount].iCodecType == AVMEDIA_TYPE_VIDEO)
    {
      streams.stream[streams.iStreamCount].iFPSScale       = resp->extract_U32();
      streams.stream[streams.iStreamCount].iFPSRate        = resp->extract_U32();
      streams.stream[streams.iStreamCount].iHeight         = resp->extract_U32();
      streams.stream[streams.iStreamCount].iWidth          = resp->extract_U32();
      streams.stream[streams.iStreamCount].fAspect         = (float)resp->extract_Double();
      streams.stream[streams.iStreamCount].strLanguage[0]  = 0;
      streams.stream[streams.iStreamCount].strLanguage[1]  = 0;
      streams.stream[streams.iStreamCount].strLanguage[2]  = 0;
      streams.stream[streams.iStreamCount].strLanguage[3]  = 0;
      streams.stream[streams.iStreamCount].iIdentifier     = -1;
      streams.iStreamCount++;

    }
    else if (streams.stream[streams.iStreamCount].iCodecType == AVMEDIA_TYPE_SUBTITLE)
    {
      const char *language    = resp->extract_String();
      uint32_t composition_id = resp->extract_U32();
      uint32_t ancillary_id   = resp->extract_U32();
      streams.stream[streams.iStreamCount].strLanguage[0]  = language[0];
      streams.stream[streams.iStreamCount].strLanguage[1]  = language[1];
      streams.stream[streams.iStreamCount].strLanguage[2]  = language[2];
      streams.stream[streams.iStreamCount].strLanguage[3]  = 0;
      streams.stream[streams.iStreamCount].iIdentifier     = (composition_id & 0xffff) | ((ancillary_id & 0xffff) << 16);
      streams.iStreamCount++;

      delete[] language;
    }
    else
    {
      m_Streams.iStreamCount = 0;
      return;
    }

    delete[] type;

    if (streams.iStreamCount >= PVR_STREAM_MAX_STREAMS)
    {
      XBMC->Log(LOG_ERROR, "%s - max amount of streams reached", __FUNCTION__);
      break;
    }
  }

  std::map<int,unsigned int>::iterator itl, itr;
  // delete streams we don't have in streams
  itl = m_StreamIndex.begin();
  while (itl != m_StreamIndex.end())
  {
    itr = streamIndex.find(itl->first);
    if (itr == streamIndex.end())
    {
      m_Streams.stream[itl->second].iCodecType = AVMEDIA_TYPE_UNKNOWN;
      m_Streams.stream[itl->second].iCodecId = CODEC_ID_NONE;
      m_StreamIndex.erase(itl);
      itl = m_StreamIndex.begin();
    }
    else
      ++itl;
  }
  // copy known streams
  for (itl = m_StreamIndex.begin(); itl != m_StreamIndex.end(); ++itl)
  {
    itr = streamIndex.find(itl->first);
    memcpy((void*)&m_Streams.stream[itl->second], (void*)&streams.stream[itr->second],
              sizeof(PVR_STREAM_PROPERTIES::PVR_STREAM));
    streamIndex.erase(itr);
  }

  // place video stream at pos 0
  for (itr = streamIndex.begin(); itr != streamIndex.end(); ++itr)
  {
    if (streams.stream[itr->second].iCodecType == AVMEDIA_TYPE_VIDEO)
      break;
  }
  if (itr != streamIndex.end())
  {
    m_StreamIndex[itr->first] = 0;
    memcpy((void*)&m_Streams.stream[0], (void*)&streams.stream[itr->second],
              sizeof(PVR_STREAM_PROPERTIES::PVR_STREAM));
    streamIndex.erase(itr);
  }

  // fill the gaps or append after highest index
  while (!streamIndex.empty())
  {
    // find first unused index
    unsigned int i;
    for (i = 0; i < PVR_STREAM_MAX_STREAMS; i++)
    {
      if (m_Streams.stream[i].iCodecType == (unsigned)AVMEDIA_TYPE_UNKNOWN)
        break;
    }
    itr = streamIndex.begin();
    m_StreamIndex[itr->first] = i;
    memcpy((void*)&m_Streams.stream[i], (void*)&streams.stream[itr->second],
              sizeof(PVR_STREAM_PROPERTIES::PVR_STREAM));
    streamIndex.erase(itr);
  }

  // set streamCount
  m_Streams.iStreamCount = 0;
  for (itl = m_StreamIndex.begin(); itl != m_StreamIndex.end(); ++itl)
  {
    if (itl->second > m_Streams.iStreamCount)
      m_Streams.iStreamCount = itl->second;
  }
  if (!m_StreamIndex.empty())
    m_Streams.iStreamCount++;
}

void cVNSIDemux::StreamStatus(cResponsePacket *resp)
{
  const char* status = resp->extract_String();
  if(status != NULL)
  {
    XBMC->Log(LOG_DEBUG, "%s - %s", __FUNCTION__, status);
    XBMC->QueueNotification(QUEUE_INFO, status);
  }
  delete[] status;
}

void cVNSIDemux::StreamSignalInfo(cResponsePacket *resp)
{
  const char* name = resp->extract_String();
  const char* status = resp->extract_String();

  m_Quality.fe_name   = name;
  m_Quality.fe_status = status;
  m_Quality.fe_snr    = resp->extract_U32();
  m_Quality.fe_signal = resp->extract_U32();
  m_Quality.fe_ber    = resp->extract_U32();
  m_Quality.fe_unc    = resp->extract_U32();

  delete[] name;
  delete[] status;
}

bool cVNSIDemux::StreamContentInfo(cResponsePacket *resp)
{
  PVR_STREAM_PROPERTIES old = m_Streams;

  while (!resp->end()) 
  {
    uint32_t pid = resp->extract_U32();
    unsigned int i;
    for (i = 0; i < m_Streams.iStreamCount; i++)
    {
      if (pid == m_Streams.stream[i].iPhysicalId)
      {
        if (m_Streams.stream[i].iCodecType == AVMEDIA_TYPE_AUDIO)
        {
          const char *language = resp->extract_String();
          
          m_Streams.stream[i].iChannels          = resp->extract_U32();
          m_Streams.stream[i].iSampleRate        = resp->extract_U32();
          m_Streams.stream[i].iBlockAlign        = resp->extract_U32();
          m_Streams.stream[i].iBitRate           = resp->extract_U32();
          m_Streams.stream[i].iBitsPerSample     = resp->extract_U32();
          m_Streams.stream[i].strLanguage[0]     = language[0];
          m_Streams.stream[i].strLanguage[1]     = language[1];
          m_Streams.stream[i].strLanguage[2]     = language[2];
          m_Streams.stream[i].strLanguage[3]     = 0;
          
          delete[] language;
        }
        else if (m_Streams.stream[i].iCodecType == AVMEDIA_TYPE_VIDEO)
        {
          m_Streams.stream[i].iFPSScale         = resp->extract_U32();
          m_Streams.stream[i].iFPSRate          = resp->extract_U32();
          m_Streams.stream[i].iHeight           = resp->extract_U32();
          m_Streams.stream[i].iWidth            = resp->extract_U32();
          m_Streams.stream[i].fAspect           = (float)resp->extract_Double();
        }
        else if (m_Streams.stream[i].iCodecType == AVMEDIA_TYPE_SUBTITLE)
        {
          const char *language    = resp->extract_String();
          uint32_t composition_id = resp->extract_U32();
          uint32_t ancillary_id   = resp->extract_U32();
          
          m_Streams.stream[i].iIdentifier    = (composition_id & 0xffff) | ((ancillary_id & 0xffff) << 16);
          m_Streams.stream[i].strLanguage[0] = language[0];
          m_Streams.stream[i].strLanguage[1] = language[1];
          m_Streams.stream[i].strLanguage[2] = language[2];
          m_Streams.stream[i].strLanguage[3] = 0;
          
          delete[] language;
        }
        else
          i = m_Streams.iStreamCount;
        break;
      }
    }
    if (i >= m_Streams.iStreamCount)
    {
      XBMC->Log(LOG_ERROR, "%s - unknown stream id: %d", __FUNCTION__, pid);
      break;
    }
  }
  return (memcmp(&old, &m_Streams, sizeof(m_Streams)) != 0);
}
