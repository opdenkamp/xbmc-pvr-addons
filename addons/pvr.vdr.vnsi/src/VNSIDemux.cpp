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
#include "VNSIDemux.h"
#include "responsepacket.h"
#include "requestpacket.h"
#include "vnsicommand.h"
#include "tools.h"

using namespace ADDON;

cVNSIDemux::cVNSIDemux()
{
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
  return m_streams.GetProperties(props);
}

void cVNSIDemux::Abort()
{
  m_streams.Clear();
}

DemuxPacket* cVNSIDemux::Read()
{
  if(ConnectionLost())
  {
    return NULL;
  }

  cResponsePacket *resp = ReadMessage(1000, g_iConnectTimeout * 1000);

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
    int iStreamId = m_streams.GetStreamId(resp->getStreamID());

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

      if (p->dts != DVD_NOPTS_VALUE)
        m_CurrentDTS = p->dts;
      else if (p->pts != DVD_NOPTS_VALUE)
        m_CurrentDTS = p->pts;
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
  else if (resp->getOpCodeID() == VNSI_STREAM_BUFFERSTATS)
  {
    m_bTimeshift = resp->extract_U8();
    m_BufferTimeStart = resp->extract_U32();
    m_BufferTimeEnd = resp->extract_U32();
  }
  else if (resp->getOpCodeID() == VNSI_STREAM_REFTIME)
  {
    m_ReferenceTime = resp->extract_U32();
    m_ReferenceDTS = (double)resp->extract_U64() * DVD_TIME_BASE / 1000000;
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
  if (!vrp2.init(VNSI_CHANNELSTREAM_OPEN) ||
      !vrp2.add_U32(channelinfo.iUniqueId) ||
      !vrp2.add_S32(g_iPriority) ||
      !vrp2.add_U8(1) ||
      !ReadSuccess(&vrp2))
  {
    XBMC->Log(LOG_ERROR, "%s - failed to set channel", __FUNCTION__);
    return false;
  }

  m_channelinfo = channelinfo;
  m_streams.Clear();
  m_MuxPacketSerial = 0;
  m_ReferenceTime = 0;
  m_BufferTimeStart = 0;
  m_BufferTimeEnd = 0;

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

time_t cVNSIDemux::GetPlayingTime()
{
  time_t ret;
  ret = m_ReferenceTime + (m_CurrentDTS - m_ReferenceDTS) / DVD_TIME_BASE;
  return ret;
}

time_t cVNSIDemux::GetBufferTimeStart()
{
  return m_BufferTimeStart;
}

time_t cVNSIDemux::GetBufferTimeEnd()
{
  return m_BufferTimeEnd;
}

void cVNSIDemux::StreamChange(cResponsePacket *resp)
{
  std::vector<PVR_STREAM_PROPERTIES::PVR_STREAM> newStreams;

  while (!resp->end())
  {
    uint32_t    pid = resp->extract_U32();
    const char* type  = resp->extract_String();

    PVR_STREAM_PROPERTIES::PVR_STREAM newStream;
    m_streams.GetStreamData(pid, &newStream);

    CodecDescriptor codecId = CodecDescriptor::GetCodecByName(type);
    if (codecId.Codec().codec_type != XBMC_CODEC_TYPE_UNKNOWN)
    {
      newStream.iPhysicalId     = pid;
      newStream.iCodecType      = codecId.Codec().codec_type;
      newStream.iCodecId        = codecId.Codec().codec_id;
    }
    else
    {
      return;
    }

    if (codecId.Codec().codec_type == XBMC_CODEC_TYPE_AUDIO)
    {
      const char *language = resp->extract_String();

      newStream.iChannels       = resp->extract_U32();
      newStream.iSampleRate     = resp->extract_U32();
      newStream.iBlockAlign     = resp->extract_U32();
      newStream.iBitRate        = resp->extract_U32();
      newStream.iBitsPerSample  = resp->extract_U32();
      newStream.strLanguage[0]  = language[0];
      newStream.strLanguage[1]  = language[1];
      newStream.strLanguage[2]  = language[2];
      newStream.strLanguage[3]  = 0;
      newStream.iIdentifier     = -1;

      newStreams.push_back(newStream);
    }
    else if (codecId.Codec().codec_type == XBMC_CODEC_TYPE_VIDEO)
    {
      newStream.iFPSScale       = resp->extract_U32();
      newStream.iFPSRate        = resp->extract_U32();
      newStream.iHeight         = resp->extract_U32();
      newStream.iWidth          = resp->extract_U32();
      newStream.fAspect         = (float)resp->extract_Double();
      newStream.strLanguage[0]  = 0;
      newStream.strLanguage[1]  = 0;
      newStream.strLanguage[2]  = 0;
      newStream.strLanguage[3]  = 0;
      newStream.iIdentifier     = -1;

      newStreams.push_back(newStream);
    }
    else if (codecId.Codec().codec_type == XBMC_CODEC_TYPE_SUBTITLE)
    {
      const char *language    = resp->extract_String();
      uint32_t composition_id = resp->extract_U32();
      uint32_t ancillary_id   = resp->extract_U32();
      newStream.strLanguage[0]  = language[0];
      newStream.strLanguage[1]  = language[1];
      newStream.strLanguage[2]  = language[2];
      newStream.strLanguage[3]  = 0;
      newStream.iIdentifier     = (composition_id & 0xffff) | ((ancillary_id & 0xffff) << 16);

      newStreams.push_back(newStream);

      delete[] language;
    }
    else
    {
      m_streams.Clear();
      delete[] type;
      return;
    }

    delete[] type;

    if (newStreams.size() >= PVR_STREAM_MAX_STREAMS)
    {
      XBMC->Log(LOG_ERROR, "%s - max amount of streams reached", __FUNCTION__);
      break;
    }
  }

  m_streams.UpdateStreams(newStreams);
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
  ADDON::XbmcStreamProperties streams = m_streams;
  while (!resp->end()) 
  {
    uint32_t pid = resp->extract_U32();
    PVR_STREAM_PROPERTIES::PVR_STREAM* props = streams.GetStreamById(pid);
    if (props)
    {
      if (props->iCodecType == XBMC_CODEC_TYPE_AUDIO)
      {
        const char *language = resp->extract_String();
          
        props->iChannels          = resp->extract_U32();
        props->iSampleRate        = resp->extract_U32();
        props->iBlockAlign        = resp->extract_U32();
        props->iBitRate           = resp->extract_U32();
        props->iBitsPerSample     = resp->extract_U32();
        props->strLanguage[0]     = language[0];
        props->strLanguage[1]     = language[1];
        props->strLanguage[2]     = language[2];
        props->strLanguage[3]     = 0;

        delete[] language;
      }
      else if (props->iCodecType == XBMC_CODEC_TYPE_VIDEO)
      {
        props->iFPSScale         = resp->extract_U32();
        props->iFPSRate          = resp->extract_U32();
        props->iHeight           = resp->extract_U32();
        props->iWidth            = resp->extract_U32();
        props->fAspect           = (float)resp->extract_Double();
      }
      else if (props->iCodecType == XBMC_CODEC_TYPE_SUBTITLE)
      {
        const char *language    = resp->extract_String();
        uint32_t composition_id = resp->extract_U32();
        uint32_t ancillary_id   = resp->extract_U32();
          
        props->iIdentifier    = (composition_id & 0xffff) | ((ancillary_id & 0xffff) << 16);
        props->strLanguage[0] = language[0];
        props->strLanguage[1] = language[1];
        props->strLanguage[2] = language[2];
        props->strLanguage[3] = 0;

        delete[] language;
      }
    }
    else
    {
      XBMC->Log(LOG_ERROR, "%s - unknown stream id: %d", __FUNCTION__, pid);
      break;
    }
  }
  m_streams = streams;
  return true;
}
