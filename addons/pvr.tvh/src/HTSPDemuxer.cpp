/*
 *      Copyright (C) 2014 Adam Sutton
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

#include "Tvheadend.h"
#include "client.h"

#include "platform/threads/mutex.h"
#include "platform/util/timeutils.h"
#include "platform/sockets/tcp.h"

extern "C" {
#include "platform/util/atomic.h"
#include "libhts/htsmsg_binary.h"
#include "libhts/sha1.h"
}

#ifdef OPENELEC_32
#include "avcodec.h"
#else
#include "xbmc_codec_descriptor.hpp"
#endif

#define TVH_TO_DVD_TIME(x) ((double)x * DVD_TIME_BASE / 1000000.0)

using namespace std;
using namespace ADDON;
using namespace PLATFORM;

CHTSPDemuxer::CHTSPDemuxer ( CHTSPConnection &conn )
  : m_conn(conn), m_opened(false), m_started(false), m_chnId(0), m_subId(0),
    m_speed(1000), m_pktBuffer((size_t)-1)
{
}

CHTSPDemuxer::~CHTSPDemuxer ( void )
{
}

void CHTSPDemuxer::Connected ( void )
{
  /* Re-subscribe */
  if (m_opened) {
    tvhdebug("demux re-starting stream");
    SendSubscribe(true);
    SendSpeed(true);
  }
}

/* **************************************************************************
 * Demuxer API
 * *************************************************************************/

void CHTSPDemuxer::Close0 ( void )
{
  /* Send unsubscribe */
  if (m_opened)
    SendUnsubscribe();

  /* Clear */
  m_opened  = false;
  m_started = false;
  Flush();
  Abort0();
}

void CHTSPDemuxer::Abort0 ( void )
{
  m_streams.Clear();
  m_streamStat.clear();
}


bool CHTSPDemuxer::Open ( const PVR_CHANNEL &chn )
{
  CLockObject lock(m_conn.Mutex());
  tvhdebug("demux open");

  /* Close current stream */
  Close0();
  
  /* Cache data */
  m_chnId  = chn.iUniqueId;
  m_speed  = 1000;

  /* Open */
  m_started = false;
  m_opened  = SendSubscribe();
  if (!m_opened)
    SendUnsubscribe();
  return m_opened;
}

void CHTSPDemuxer::Close ( void )
{
  CLockObject lock(m_conn.Mutex());
  Close0();
  tvhdebug("demux close");
}

DemuxPacket *CHTSPDemuxer::Read ( void )
{
  DemuxPacket *pkt = NULL;
  if (m_pktBuffer.Pop(pkt, 1000)) {
    tvhtrace("demux read idx :%d pts %lf len %lld",
             pkt->iStreamId, pkt->pts, (long long)pkt->iSize);
    return pkt;
  }
  tvhtrace("demux read nothing");
  
  return PVR->AllocateDemuxPacket(0);
}

void CHTSPDemuxer::Flush ( void )
{
  DemuxPacket *pkt;
  tvhdebug("demux flush");
  while (m_pktBuffer.Pop(pkt))
    PVR->FreeDemuxPacket(pkt);
}

void CHTSPDemuxer::Abort ( void )
{
  tvhdebug("demux abort");
  CLockObject lock(m_conn.Mutex());
  Abort0();
}

bool CHTSPDemuxer::Seek 
  ( int time, bool _unused(backwards), double *startpts )
{
  htsmsg_t *m;

  CLockObject lock(m_conn.Mutex());
  if (!m_subId)
    return false;

  tvhdebug("demux seek %d", time);

  /* Clear */
  m_seekTime = 0;

  /* Build message */
  m = htsmsg_create_map();  
  htsmsg_add_u32(m, "subscriptionId", m_subId);
  htsmsg_add_s64(m, "time",           time * 1000);
  htsmsg_add_u32(m, "absolute",       1);

  /* Send and Wait */
  m = m_conn.SendAndWait("subscriptionSeek", m);
  if (!m)
  {
    tvherror("failed to send subscriptionSeek");
    return false;
  }

  /* Wait for time */
  if (!m_seekCond.Wait(m_conn.Mutex(), m_seekTime, 5000) || m_seekTime < 0)
  {
    tvherror("failed to get subscriptionSeek response");
    return false;
  }

  /* Store */
  *startpts = TVH_TO_DVD_TIME(m_seekTime);
  tvhtrace("startpts = %lf", *startpts);

  return true;
}

void CHTSPDemuxer::Speed ( int speed )
{
  CLockObject lock(m_conn.Mutex());
  if (!m_subId)
    return;
  m_speed = speed;
  SendSpeed();
}

int CHTSPDemuxer::CurrentId ( void )
{
  return -1;
}

PVR_ERROR CHTSPDemuxer::CurrentStreams ( PVR_STREAM_PROPERTIES *streams )
{
  CLockObject lock(m_mutex);
  if (!m_startCond.Wait(m_mutex, m_started, 5000))
    return PVR_ERROR_SERVER_ERROR;
  return m_streams.GetProperties(streams) ? PVR_ERROR_NO_ERROR
                                          : PVR_ERROR_SERVER_ERROR; 
}

PVR_ERROR CHTSPDemuxer::CurrentSignal ( PVR_SIGNAL_STATUS &sig )
{
  CLockObject lock(m_mutex);
  
  strncpy(sig.strAdapterName,   m_sourceInfo.si_adapter.c_str(),
          sizeof(sig.strAdapterName));
  strncpy(sig.strAdapterStatus, m_signalInfo.fe_status.c_str(),
          sizeof(sig.strAdapterStatus));
  strncpy(sig.strServiceName,   m_sourceInfo.si_service.c_str(),
          sizeof(sig.strServiceName));
  strncpy(sig.strProviderName,  m_sourceInfo.si_provider.c_str(),
          sizeof(sig.strProviderName));
  strncpy(sig.strMuxName,       m_sourceInfo.si_mux.c_str(),
          sizeof(sig.strMuxName));
  
  sig.iSNR      = m_signalInfo.fe_snr;
  sig.iSignal   = m_signalInfo.fe_signal;
  sig.iBER      = m_signalInfo.fe_ber;
  sig.iUNC      = m_signalInfo.fe_unc;

  return PVR_ERROR_NO_ERROR;
}

/* **************************************************************************
 * Send Messages
 * *************************************************************************/

bool CHTSPDemuxer::SendSubscribe ( bool force )
{
  int err;
  htsmsg_t *m;

  /* Reset status */
  m_signalInfo.Clear();
  m_sourceInfo.Clear();

  /* Build message */
  m = htsmsg_create_map();
  htsmsg_add_s32(m, "channelId",       m_chnId);
  htsmsg_add_u32(m, "subscriptionId",  ++m_subId);
  htsmsg_add_u32(m, "timeshiftPeriod", (uint32_t)~0);
  htsmsg_add_u32(m, "normts",          1);

  /* Send and Wait for response */
  tvhdebug("demux subscribe to %08x", m_chnId);
  if (force)
    m = m_conn.SendAndWait0("subscribe", m);
  else
    m = m_conn.SendAndWait("subscribe", m);
  if (m == NULL)
  {
    tvhdebug("demux failed to send subscribe");
    return false;
  }

  /* Error */
  err = htsmsg_get_u32_or_default(m, "error", 0);
  htsmsg_destroy(m);
  if (err)
  {
    tvhdebug("demux failed to subscribe");
    return false;
  }

  tvhdebug("demux succesfully subscribed to %08x", m_chnId);
  return true;
}

void CHTSPDemuxer::SendUnsubscribe ( void )
{
  int err;
  htsmsg_t *m;

  /* Build message */
  m = htsmsg_create_map();
  htsmsg_add_u32(m, "subscriptionId", m_subId);

  /* Send and Wait */
  tvhdebug("demux unsubcribe from %d", m_subId);
  if ((m = m_conn.SendAndWait("unsubscribe", m)) == NULL)
  {
    tvhdebug("demux failed to send unsubcribe");
    return;
  }

  /* Error */
  err = htsmsg_get_u32_or_default(m, "error", 0);
  htsmsg_destroy(m);
  if (err)
  {
    tvhdebug("demux failed to unsubcribe");
    return;
  }

  tvhdebug("demux succesfully unsubscribed %d", m_subId);
}

void CHTSPDemuxer::SendSpeed ( bool force )
{
  htsmsg_t *m;

  /* Build message */
  m = htsmsg_create_map();
  htsmsg_add_u32(m, "subscriptionId", m_subId);
  htsmsg_add_s32(m, "speed",          m_speed / 10);
  tvhdebug("demux send speed %d", m_speed / 10);

  /* Send and Wait */
  if (force)
    m = m_conn.SendAndWait0("subscriptionSpeed", m);
  else
    m = m_conn.SendAndWait("subscriptionSpeed", m);
  if (m)
    htsmsg_destroy(m);
}

/* **************************************************************************
 * Parse incoming data
 * *************************************************************************/

bool CHTSPDemuxer::ProcessMessage ( const char *method, htsmsg_t *m )
{
  uint32_t subId;

  CLockObject lock(m_mutex);

  /* No subscriptionId - not for demuxer */
  if (htsmsg_get_u32(m, "subscriptionId", &subId))
    return false;

  /* Not current subscription - ignore */
  else if (subId != m_subId)
    return true;

  /* Subscription messages */
  else if (!strcmp("muxpkt", method))
    ParseMuxPacket(m);
  else if (!strcmp("subscriptionStatus", method))
    ParseSubscriptionStatus(m);
  else if (!strcmp("queueStatus", method))
    ParseQueueStatus(m);
  else if (!strcmp("signalStatus", method))
    ParseSignalStatus(m);
  else if (!strcmp("timeshiftStatus", method))
    ParseTimeshiftStatus(m);
  else if (!strcmp("subscriptionStart", method))
    ParseSubscriptionStart(m);
  else if (!strcmp("subscriptionStop", method))
    ParseSubscriptionStop(m);
  else if (!strcmp("subscriptionSkip", method))
    ParseSubscriptionSkip(m);
  else if (!strcmp("subscriptionSpeed", method))
    ParseSubscriptionSpeed(m);
  else
    tvhdebug("demux unhandled subscription message [%s]",
              method);

  return true;
}

void CHTSPDemuxer::ParseMuxPacket ( htsmsg_t *m )
{
  uint32_t    idx, u32;
  int64_t     s64;
  const void  *bin;
  size_t      binlen;
  DemuxPacket *pkt;
  char        _unused(type) = 0;

  /* Validate fields */
  if (htsmsg_get_u32(m, "stream", &idx) ||
      htsmsg_get_bin(m, "payload", &bin, &binlen))
  { 
    tvherror("malformed muxpkt");
    return;
  }

  /* Record */
  m_streamStat[idx]++;

  /* Allocate buffer */
  if (!(pkt = PVR->AllocateDemuxPacket(binlen)))
    return;
  memcpy(pkt->pData, bin, binlen);
  pkt->iSize = binlen;

  /* Duration */
  if (!htsmsg_get_u32(m, "duration", &u32))
    pkt->duration = TVH_TO_DVD_TIME(u32);
  
  /* Timestamps */
  if (!htsmsg_get_s64(m, "dts", &s64))
    pkt->dts      = TVH_TO_DVD_TIME(s64);
  else
    pkt->dts      = DVD_NOPTS_VALUE;

  if (!htsmsg_get_s64(m, "pts", &s64))
    pkt->pts      = TVH_TO_DVD_TIME(s64);
  else
    pkt->pts      = DVD_NOPTS_VALUE;

  /* Type (for debug only) */
  if (!htsmsg_get_u32(m, "frametype", &u32))
    type = (char)u32;
  if (!type)
    type = '_';

  /* Find the stream */
  pkt->iStreamId = m_streams.GetStreamId(idx);
  tvhtrace("demux pkt idx %d:%d type %c pts %lf len %lld",
           idx, pkt->iStreamId, type, pkt->pts, (long long)binlen);

  /* Drop (could be done earlier) */
  if (pkt->iStreamId < 0)
  {
    tvhdebug("demux drop pkt");
    PVR->FreeDemuxPacket(pkt);
    return;
  }

  /* Store */
  m_pktBuffer.Push(pkt);
}

#if OPENELEC_32
bool GetCodecByName
  ( const char *name, unsigned int *codec_type, unsigned int *codec_id )
{
  static struct {
    const char *name;
    int         type;
    int         id;
  } codecs[] = {
    { "AC3",        AVMEDIA_TYPE_AUDIO,     CODEC_ID_AC3          },
    { "EAC3",       AVMEDIA_TYPE_AUDIO,     CODEC_ID_EAC3         },
    { "MPEG2AUDIO", AVMEDIA_TYPE_AUDIO,     CODEC_ID_MP2          },
    { "AAC",        AVMEDIA_TYPE_AUDIO,     CODEC_ID_AAC          },
    { "AACLATM",    AVMEDIA_TYPE_AUDIO,     CODEC_ID_AAC_LATM     },
    { "VORBIS",     AVMEDIA_TYPE_AUDIO,     CODEC_ID_VORBIS       },
    { "MPEG2VIDEO", AVMEDIA_TYPE_VIDEO,     CODEC_ID_MPEG2VIDEO   },
    { "H264",       AVMEDIA_TYPE_VIDEO,     CODEC_ID_H264         },
    { "VP8",        AVMEDIA_TYPE_VIDEO,     CODEC_ID_VP8          },
    { "MPEG4VIDEO", AVMEDIA_TYPE_VIDEO,     CODEC_ID_MPEG4        },
    { "DVBSUB",     AVMEDIA_TYPE_SUBTITLE,  CODEC_ID_DVB_SUBTITLE },
    { "TEXTSUB",    AVMEDIA_TYPE_SUBTITLE,  CODEC_ID_TEXT         },
    { "TELETEXT",   AVMEDIA_TYPE_SUBTITLE,  CODEC_ID_DVB_TELETEXT },
    { NULL, 0, 0 }
  };
  int i = 0;
  
  while (codecs[i].name) {
    if (!strcmp(name, codecs[i].name)) {
      *codec_type = codecs[i].type;
      *codec_id   = codecs[i].id;
      return true;
    }
    i++;
  }
  return false;
}
#else
bool GetCodecByName
  ( const char *name, xbmc_codec_type_t *codec_type, unsigned int *codec_id )
{
  CodecDescriptor codec = CodecDescriptor::GetCodecByName(name);
  if (codec.Codec().codec_type == XBMC_CODEC_TYPE_UNKNOWN)
    return false;
  *codec_type = codec.Codec().codec_type;
  *codec_id   = codec.Codec().codec_id;
  return true;
}
#endif

void CHTSPDemuxer::ParseSubscriptionStart ( htsmsg_t *m )
{
  vector<XbmcPvrStream>  streams;
  htsmsg_t               *l;
  htsmsg_field_t         *f;
  DemuxPacket            *pkt;

  /* Validate */
  if ((l = htsmsg_get_list(m, "streams")) == NULL)
  {
    tvherror("malformed subscriptionStart");
    return;
  }
  m_streamStat.clear();

  /* Process each */
  HTSMSG_FOREACH(f, l)
  {
    uint32_t      idx, u32;
    const char    *type, *str;
    XbmcPvrStream stream;

    if (f->hmf_type != HMF_MAP)
      continue;
    if ((type = htsmsg_get_str(&f->hmf_msg, "type")) == NULL)
      continue;
    if (htsmsg_get_u32(&f->hmf_msg, "index", &idx))
      continue;

    /* Find stream */
    m_streamStat[idx] = 0;
    m_streams.GetStreamData(idx, &stream);
    tvhdebug("demux subscription start");
    if (GetCodecByName(type, &stream.iCodecType, &stream.iCodecId))
    {
      stream.iPhysicalId = idx;

      /* Subtitle ID */
      if ((stream.iCodecType == XBMC_CODEC_TYPE_SUBTITLE) &&
          !strcmp("DVBSUB", type))
      {
        uint32_t composition_id = 0, ancillary_id = 0;
        htsmsg_get_u32(&f->hmf_msg, "composition_id", &composition_id);
        htsmsg_get_u32(&f->hmf_msg, "ancillary_id"  , &ancillary_id);
        stream.iIdentifier = (composition_id & 0xffff)
                           | ((ancillary_id & 0xffff) << 16);
      }

      /* Language */
      if (stream.iCodecType == XBMC_CODEC_TYPE_SUBTITLE ||
          stream.iCodecType == XBMC_CODEC_TYPE_AUDIO)
      {
        if ((str = htsmsg_get_str(&f->hmf_msg, "language")) != NULL)
          strncpy(stream.strLanguage, str, sizeof(stream.strLanguage));
      }

      /* Audio data */
      if (stream.iCodecType == XBMC_CODEC_TYPE_AUDIO)
      {
        stream.iChannels
          = htsmsg_get_u32_or_default(&f->hmf_msg, "channels", 0);
        stream.iSampleRate
          = htsmsg_get_u32_or_default(&f->hmf_msg, "rate", 0);
      }

      /* Video */
      if (stream.iCodecType == XBMC_CODEC_TYPE_VIDEO)
      {
        stream.iWidth   = htsmsg_get_u32_or_default(&f->hmf_msg, "width", 0);
        stream.iHeight  = htsmsg_get_u32_or_default(&f->hmf_msg, "height", 0);
        if ((u32 = htsmsg_get_u32_or_default(&f->hmf_msg, "aspect_den", 0)))
          stream.fAspect
            = (float)htsmsg_get_u32_or_default(&f->hmf_msg, "aspect_num", 0)
            / u32;
        else
          stream.fAspect = 0.0;
        if ((u32 = htsmsg_get_u32_or_default(&f->hmf_msg, "duration", 0)) > 0)
        {
          stream.iFPSScale = u32;
          stream.iFPSRate  = DVD_TIME_BASE;
        }
      }
        
      streams.push_back(stream);
      tvhdebug("  id: %d, type %s, codec: %u", idx, type, stream.iCodecId);
    }
  }

  /* Update streams */
  tvhdebug("demux stream change");
  m_streams.UpdateStreams(streams);
  pkt = PVR->AllocateDemuxPacket(0);
  pkt->iStreamId = DMX_SPECIALID_STREAMCHANGE;
  m_pktBuffer.Push(pkt);

  /* Source data */
  ParseSourceInfo(htsmsg_get_map(m, "sourceinfo"));

  /* Signal */
  m_started = true;
  m_startCond.Broadcast();
}

void CHTSPDemuxer::ParseSourceInfo ( htsmsg_t *m )
{
  const char *str;
  
  /* Ignore */
  if (!m) return;
  
  tvhdebug("demux sourceInfo:");
  if ((str = htsmsg_get_str(m, "adapter")) != NULL)
  {
    tvhdebug("  adapter : %s", str);
    m_sourceInfo.si_adapter  = str;
  }
  if ((str = htsmsg_get_str(m, "network")) != NULL)
  {
    tvhdebug("  network : %s", str);
    m_sourceInfo.si_network  = str;
  }
  if ((str = htsmsg_get_str(m, "mux")) != NULL)
  {
    tvhdebug("  mux     : %s", str);
    m_sourceInfo.si_mux      = str;
  }
  if ((str = htsmsg_get_str(m, "provider")) != NULL)
  {
    tvhdebug("  provider : %s", str);
    m_sourceInfo.si_provider = str;
  }
  if ((str = htsmsg_get_str(m, "service")) != NULL)
  {
    tvhdebug("  service : %s", str);
    m_sourceInfo.si_service  = str;
  }
}

void CHTSPDemuxer::ParseSubscriptionStop ( htsmsg_t *_unused(m) )
{
}

void CHTSPDemuxer::ParseSubscriptionSkip ( htsmsg_t *m )
{
  CLockObject lock(m_conn.Mutex());
  int64_t s64;
  if (htsmsg_get_s64(m, "time", &s64)) {
    m_seekTime = -1;
  } else {
    m_seekTime = s64;
  }
  m_seekCond.Broadcast();
}

void CHTSPDemuxer::ParseSubscriptionSpeed ( htsmsg_t *m )
{
  uint32_t u32;
  if (!htsmsg_get_u32(m, "speed", &u32))
    tvhdebug("recv speed %d", u32);
}

void CHTSPDemuxer::ParseSubscriptionStatus ( htsmsg_t *_unused(m) )
{
}

void CHTSPDemuxer::ParseQueueStatus ( htsmsg_t *_unused(m) )
{
  map<int,int>::iterator it;
  tvhdebug("stream stats:");
  for (it = m_streamStat.begin(); it != m_streamStat.end(); it++)
    tvhdebug("stream idx:%d num:%d", it->first, it->second);
}

void CHTSPDemuxer::ParseSignalStatus ( htsmsg_t *m )
{
  uint32_t u32;
  const char *str;

  /* Reset */
  m_signalInfo.Clear();

  /* Parse */
  tvhdebug("signalStatus:");
  if ((str = htsmsg_get_str(m, "feStatus")) != NULL)
  {
    tvhdebug("  status : %s", str);
    m_signalInfo.fe_status = str;
  }
  if (!htsmsg_get_u32(m, "feSNR", &u32))
  {
    tvhdebug("  snr    : %d", u32);
    m_signalInfo.fe_snr    = u32;
  }
  if (!htsmsg_get_u32(m, "feBER", &u32))
  {
    tvhdebug("  ber    : %d", u32);
    m_signalInfo.fe_ber    = u32;
  }
  if (!htsmsg_get_u32(m, "feUNC", &u32))
  {
    tvhdebug("  unc    : %d", u32);
    m_signalInfo.fe_unc    = u32;
  }
  if (!htsmsg_get_u32(m, "feSignal", &u32))
  {
    tvhdebug("  signal    : %d", u32);
    m_signalInfo.fe_signal = u32;
  }
}

void CHTSPDemuxer::ParseTimeshiftStatus ( htsmsg_t *m )
{
  uint32_t u32;
  int64_t s64;
  tvhdebug("timeshiftStatus:");
  if (!htsmsg_get_u32(m, "full", &u32))
    tvhdebug("  full  : %d", u32);
  if (!htsmsg_get_s64(m, "start", &s64))
    tvhdebug("  start : %ld", (long)s64);
  if (!htsmsg_get_s64(m, "end", &s64))
    tvhdebug("  end   : %ld", (long)s64);
  if (!htsmsg_get_s64(m, "shift", &s64))
    tvhdebug("  shift : %ld", (long)s64);
}
