/*
 *      vdr-plugin-vnsi - XBMC server plugin for VDR
 *
 *      Copyright (C) 2010 Alwin Esch (Team XBMC)
 *      Copyright (C) 2010, 2011 Alexander Pipelka
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
#include <sys/ioctl.h>
#include <time.h>

#include <vdr/channels.h>

#include "config.h"
#include "streamer.h"
#include "cxsocket.h"
#include "vnsicommand.h"
#include "responsepacket.h"
#include "vnsi.h"
#include "videobuffer.h"


// --- cLiveStreamer -------------------------------------------------

cLiveStreamer::cLiveStreamer(int clientID, uint8_t timeshift, uint32_t timeout)
 : cThread("cLiveStreamer stream processor")
 , m_ClientID(clientID)
 , m_scanTimeout(timeout)
 , m_VideoInput(m_Event, m_Mutex, m_IsRetune)
{
  m_Channel         = NULL;
  m_Socket          = NULL;
  m_Frontend        = -1;
  m_IsAudioOnly     = false;
  m_IsMPEGPS        = false;
  m_startup         = true;
  m_SignalLost      = false;
  m_IFrameSeen      = false;
  m_VideoBuffer     = NULL;
  m_Timeshift       = timeshift;
  m_IsRetune        = false;

  memset(&m_FrontendInfo, 0, sizeof(m_FrontendInfo));

  if(m_scanTimeout == 0)
    m_scanTimeout = VNSIServerConfig.stream_timeout;
}

cLiveStreamer::~cLiveStreamer()
{
  DEBUGLOG("Started to delete live streamer");

  Cancel(5);
  Close();

  DEBUGLOG("Finished to delete live streamer");
}

bool cLiveStreamer::Open(int serial)
{
  Close();

#if APIVERSNUM >= 10725
  m_Device = cDevice::GetDevice(m_Channel, m_Priority, true, true);
#else
  m_Device = cDevice::GetDevice(m_Channel, m_Priority, true);
#endif

  if (!m_Device)
    return false;

  bool recording = false;
  if (0) // test harness
  {
    recording = true;
    m_VideoBuffer = cVideoBuffer::Create("/home/xbmc/test.ts");
  }
  else if (serial == -1)
  {
    for (cTimer *timer = Timers.First(); timer; timer = Timers.Next(timer))
    {
      if (timer &&
          timer->Recording() &&
          timer->Channel() == m_Channel)
      {
        Recordings.Load();
        cRecording matchRec(timer, timer->Event());
        cRecording *rec;
        {
          cThreadLock RecordingsLock(&Recordings);
          rec = Recordings.GetByName(matchRec.FileName());
          if (!rec)
          {
            return false;
          }
        }
        m_VideoBuffer = cVideoBuffer::Create(rec);
        recording = true;
        break;
      }
    }
  }
  if (!recording)
  {
    m_VideoBuffer = cVideoBuffer::Create(m_ClientID, m_Timeshift);
  }

  if (!m_VideoBuffer)
    return false;

  if (!recording)
  {
    if (m_Channel && ((m_Channel->Source() >> 24) == 'V'))
      m_IsMPEGPS = true;

    m_IsRetune = false;
    if (!m_VideoInput.Open(m_Channel, m_Priority, m_VideoBuffer))
    {
      ERRORLOG("Can't switch to channel %i - %s", m_Channel->Number(), m_Channel->Name());
      return false;
    }
  }

  m_Demuxer.Open(*m_Channel, m_VideoBuffer);
  if (serial >= 0)
    m_Demuxer.SetSerial(serial);

  return true;
}

void cLiveStreamer::Close(void)
{
  INFOLOG("LiveStreamer::Close - close");
  m_VideoInput.Close();
  m_Demuxer.Close();
  if (m_VideoBuffer)
  {
    delete m_VideoBuffer;
    m_VideoBuffer = NULL;
  }

  if (m_Frontend >= 0)
  {
    close(m_Frontend);
    m_Frontend = -1;
  }
}

void cLiveStreamer::Action(void)
{
  int ret;
  sStreamPacket pkt;
  memset(&pkt, 0, sizeof(sStreamPacket));
  bool requestStreamChange = false;
  cTimeMs last_info(1000);
  cTimeMs bufferStatsTimer(1000);

  while (Running())
  {
    ret = m_Demuxer.Read(&pkt);
    if (ret > 0)
    {
      if (pkt.pmtChange)
      {
        requestStreamChange = true;
      }
      if (pkt.data)
      {
        if (pkt.streamChange || requestStreamChange)
          sendStreamChange();
        requestStreamChange = false;
        if (pkt.reftime)
        {
          sendRefTime(&pkt);
          pkt.reftime = 0;
        }
        sendStreamPacket(&pkt);
      }

      // send signal info every 10 sec.
      if(last_info.Elapsed() >= 10*1000)
      {
        last_info.Set(0);
        sendSignalInfo();
      }

      // send buffer stats
      if(bufferStatsTimer.TimedOut())
      {
        sendBufferStatus();
        bufferStatsTimer.Set(1000);
      }
    }
    else if (ret == -1)
    {
      // no data
      {
        cMutexLock lock(&m_Mutex);
        if (m_IsRetune)
        {
          m_VideoInput.Close();
          m_VideoInput.Open(m_Channel, m_Priority, m_VideoBuffer);
          m_IsRetune = false;
        }
        else
          m_Event.TimedWait(m_Mutex, 10);
      }
      if(m_last_tick.Elapsed() >= (uint64_t)(m_scanTimeout*1000))
      {
        sendStreamStatus();
        m_last_tick.Set(0);
        m_SignalLost = true;
      }
    }
    else if (ret == -2)
    {
      if (!Open(m_Demuxer.GetSerial()))
      {
        m_Socket->Shutdown();
        break;
      }
    }
  }
  INFOLOG("exit streamer thread");
}

bool cLiveStreamer::StreamChannel(const cChannel *channel, int priority, cxSocket *Socket, cResponsePacket *resp)
{
  if (channel == NULL)
  {
    ERRORLOG("Starting streaming of channel without valid channel");
    return false;
  }

  m_Channel   = channel;
  m_Priority  = priority;
  m_Socket    = Socket;

  if (!Open())
    return false;

  // Send the OK response here, that it is before the Stream end message
  resp->add_U32(VNSI_RET_OK);
  resp->finalise();
  m_Socket->write(resp->getPtr(), resp->getLen());

  Activate(true);

  INFOLOG("Successfully switched to channel %i - %s", m_Channel->Number(), m_Channel->Name());
  return true;
}

inline void cLiveStreamer::Activate(bool On)
{
  if (On)
  {
    DEBUGLOG("VDR active, sending stream start message");
    Start();
  }
  else
  {
    DEBUGLOG("VDR inactive, sending stream end message");
    Cancel(5);
  }
}

void cLiveStreamer::sendStreamPacket(sStreamPacket *pkt)
{
  if(pkt == NULL)
    return;

  if(pkt->size == 0)
    return;

  if (!m_streamHeader.initStream(VNSI_STREAM_MUXPKT, pkt->id, pkt->duration, pkt->pts, pkt->dts, pkt->serial))
  {
    ERRORLOG("stream response packet init fail");
    return;
  }
  m_streamHeader.setLen(m_streamHeader.getStreamHeaderLength() + pkt->size);
  m_streamHeader.finaliseStream();

  m_Socket->LockWrite();
  m_Socket->write(m_streamHeader.getPtr(), m_streamHeader.getStreamHeaderLength(), -1, true);
  m_Socket->write(pkt->data, pkt->size);
  m_Socket->UnlockWrite();

  m_last_tick.Set(0);
  m_SignalLost = false;
}

void cLiveStreamer::sendStreamChange()
{
  cResponsePacket *resp = new cResponsePacket();
  if (!resp->initStream(VNSI_STREAM_CHANGE, 0, 0, 0, 0, 0))
  {
    ERRORLOG("stream response packet init fail");
    delete resp;
    return;
  }

  uint32_t FpsScale, FpsRate, Height, Width;
  double Aspect;
  uint32_t Channels, SampleRate, BitRate, BitsPerSample, BlockAlign;
  for (cTSStream* stream = m_Demuxer.GetFirstStream(); stream; stream = m_Demuxer.GetNextStream())
  {
    resp->add_U32(stream->GetPID());
    if (stream->Type() == stMPEG2AUDIO)
    {
      stream->GetAudioInformation(Channels, SampleRate, BitRate, BitsPerSample, BlockAlign);
      resp->add_String("MPEG2AUDIO");
      resp->add_String(stream->GetLanguage());
      resp->add_U32(Channels);
      resp->add_U32(SampleRate);
      resp->add_U32(BlockAlign);
      resp->add_U32(BitRate);
      resp->add_U32(BitsPerSample);
    }
    else if (stream->Type() == stMPEG2VIDEO)
    {
      stream->GetVideoInformation(FpsScale, FpsRate, Height, Width, Aspect);
      resp->add_String("MPEG2VIDEO");
      resp->add_U32(FpsScale);
      resp->add_U32(FpsRate);
      resp->add_U32(Height);
      resp->add_U32(Width);
      resp->add_double(Aspect);
    }
    else if (stream->Type() == stAC3)
    {
      stream->GetAudioInformation(Channels, SampleRate, BitRate, BitsPerSample, BlockAlign);
      resp->add_String("AC3");
      resp->add_String(stream->GetLanguage());
      resp->add_U32(Channels);
      resp->add_U32(SampleRate);
      resp->add_U32(BlockAlign);
      resp->add_U32(BitRate);
      resp->add_U32(BitsPerSample);
    }
    else if (stream->Type() == stH264)
    {
      stream->GetVideoInformation(FpsScale, FpsRate, Height, Width, Aspect);
      resp->add_String("H264");
      resp->add_U32(FpsScale);
      resp->add_U32(FpsRate);
      resp->add_U32(Height);
      resp->add_U32(Width);
      resp->add_double(Aspect);
    }
    else if (stream->Type() == stDVBSUB)
    {
      resp->add_String("DVBSUB");
      resp->add_String(stream->GetLanguage());
      resp->add_U32(stream->CompositionPageId());
      resp->add_U32(stream->AncillaryPageId());
    }
    else if (stream->Type() == stTELETEXT)
    {
      resp->add_String("TELETEXT");
      resp->add_String(stream->GetLanguage());
      resp->add_U32(stream->CompositionPageId());
      resp->add_U32(stream->AncillaryPageId());
    }
    else if (stream->Type() == stAACADTS)
    {
      stream->GetAudioInformation(Channels, SampleRate, BitRate, BitsPerSample, BlockAlign);
      resp->add_String("AAC");
      resp->add_String(stream->GetLanguage());
      resp->add_U32(Channels);
      resp->add_U32(SampleRate);
      resp->add_U32(BlockAlign);
      resp->add_U32(BitRate);
      resp->add_U32(BitsPerSample);
    }
    else if (stream->Type() == stAACLATM)
    {
      stream->GetAudioInformation(Channels, SampleRate, BitRate, BitsPerSample, BlockAlign);
      resp->add_String("AAC_LATM");
      resp->add_String(stream->GetLanguage());
      resp->add_U32(Channels);
      resp->add_U32(SampleRate);
      resp->add_U32(BlockAlign);
      resp->add_U32(BitRate);
      resp->add_U32(BitsPerSample);
    }
    else if (stream->Type() == stEAC3)
    {
      stream->GetAudioInformation(Channels, SampleRate, BitRate, BitsPerSample, BlockAlign);
      resp->add_String("EAC3");
      resp->add_String(stream->GetLanguage());
      resp->add_U32(Channels);
      resp->add_U32(SampleRate);
      resp->add_U32(BlockAlign);
      resp->add_U32(BitRate);
      resp->add_U32(BitsPerSample);
    }
    else if (stream->Type() == stDTS)
    {
      stream->GetAudioInformation(Channels, SampleRate, BitRate, BitsPerSample, BlockAlign);
      resp->add_String("DTS");
      resp->add_String(stream->GetLanguage());
      resp->add_U32(Channels);
      resp->add_U32(SampleRate);
      resp->add_U32(BlockAlign);
      resp->add_U32(BitRate);
      resp->add_U32(BitsPerSample);
    }
  }

  resp->finaliseStream();
  m_Socket->write(resp->getPtr(), resp->getLen());
  delete resp;
}

void cLiveStreamer::sendSignalInfo()
{
  /* If no frontend is found m_Frontend is set to -2, in this case
     return a empty signalinfo package */
  if (m_Frontend == -2)
  {
    cResponsePacket *resp = new cResponsePacket();
    if (!resp->initStream(VNSI_STREAM_SIGNALINFO, 0, 0, 0, 0, 0))
    {
      ERRORLOG("stream response packet init fail");
      delete resp;
      return;
    }

    resp->add_String(*cString::sprintf("Unknown"));
    resp->add_String(*cString::sprintf("Unknown"));
    resp->add_U32(0);
    resp->add_U32(0);
    resp->add_U32(0);
    resp->add_U32(0);

    resp->finaliseStream();
    m_Socket->write(resp->getPtr(), resp->getLen());
    delete resp;
    return;
  }

  if (m_Channel && ((m_Channel->Source() >> 24) == 'V'))
  {
    if (m_Frontend < 0)
    {
      for (int i = 0; i < 8; i++)
      {
        m_DeviceString = cString::sprintf("/dev/video%d", i);
        m_Frontend = open(m_DeviceString, O_RDONLY | O_NONBLOCK);
        if (m_Frontend >= 0)
        {
          if (ioctl(m_Frontend, VIDIOC_QUERYCAP, &m_vcap) < 0)
          {
            ERRORLOG("cannot read analog frontend info.");
            close(m_Frontend);
            m_Frontend = -1;
            memset(&m_vcap, 0, sizeof(m_vcap));
            continue;
          }
          break;
        }
      }
      if (m_Frontend < 0)
        m_Frontend = -2;
    }

    if (m_Frontend >= 0)
    {
      cResponsePacket *resp = new cResponsePacket();
      if (!resp->initStream(VNSI_STREAM_SIGNALINFO, 0, 0, 0, 0, 0))
      {
        ERRORLOG("stream response packet init fail");
        delete resp;
        return;
      }
      resp->add_String(*cString::sprintf("Analog #%s - %s (%s)", *m_DeviceString, (char *) m_vcap.card, m_vcap.driver));
      resp->add_String("");
      resp->add_U32(0);
      resp->add_U32(0);
      resp->add_U32(0);
      resp->add_U32(0);

      resp->finaliseStream();
      m_Socket->write(resp->getPtr(), resp->getLen());
      delete resp;
    }
  }
  else
  {
    if (m_Frontend < 0)
    {
      m_DeviceString = cString::sprintf(FRONTEND_DEVICE, m_Device->CardIndex(), 0);
      m_Frontend = open(m_DeviceString, O_RDONLY | O_NONBLOCK);
      if (m_Frontend >= 0)
      {
        if (ioctl(m_Frontend, FE_GET_INFO, &m_FrontendInfo) < 0)
        {
          ERRORLOG("cannot read frontend info.");
          close(m_Frontend);
          m_Frontend = -2;
          memset(&m_FrontendInfo, 0, sizeof(m_FrontendInfo));
          return;
        }
      }
    }

    if (m_Frontend >= 0)
    {
      cResponsePacket *resp = new cResponsePacket();
      if (!resp->initStream(VNSI_STREAM_SIGNALINFO, 0, 0, 0, 0, 0))
      {
        ERRORLOG("stream response packet init fail");
        delete resp;
        return;
      }

      fe_status_t status;
      uint16_t fe_snr;
      uint16_t fe_signal;
      uint32_t fe_ber;
      uint32_t fe_unc;

      memset(&status, 0, sizeof(status));
      ioctl(m_Frontend, FE_READ_STATUS, &status);

      if (ioctl(m_Frontend, FE_READ_SIGNAL_STRENGTH, &fe_signal) == -1)
        fe_signal = -2;
      if (ioctl(m_Frontend, FE_READ_SNR, &fe_snr) == -1)
        fe_snr = -2;
      if (ioctl(m_Frontend, FE_READ_BER, &fe_ber) == -1)
        fe_ber = -2;
      if (ioctl(m_Frontend, FE_READ_UNCORRECTED_BLOCKS, &fe_unc) == -1)
        fe_unc = -2;

      switch (m_Channel->Source() & cSource::st_Mask)
      {
        case cSource::stSat:
          resp->add_String(*cString::sprintf("DVB-S%s #%d - %s", (m_FrontendInfo.caps & 0x10000000) ? "2" : "",  cDevice::ActualDevice()->CardIndex(), m_FrontendInfo.name));
          break;
        case cSource::stCable:
          resp->add_String(*cString::sprintf("DVB-C #%d - %s", cDevice::ActualDevice()->CardIndex(), m_FrontendInfo.name));
          break;
        case cSource::stTerr:
          resp->add_String(*cString::sprintf("DVB-T #%d - %s", cDevice::ActualDevice()->CardIndex(), m_FrontendInfo.name));
          break;
      }
      resp->add_String(*cString::sprintf("%s:%s:%s:%s:%s", (status & FE_HAS_LOCK) ? "LOCKED" : "-", (status & FE_HAS_SIGNAL) ? "SIGNAL" : "-", (status & FE_HAS_CARRIER) ? "CARRIER" : "-", (status & FE_HAS_VITERBI) ? "VITERBI" : "-", (status & FE_HAS_SYNC) ? "SYNC" : "-"));
      resp->add_U32(fe_snr);
      resp->add_U32(fe_signal);
      resp->add_U32(fe_ber);
      resp->add_U32(fe_unc);

      resp->finaliseStream();
      m_Socket->write(resp->getPtr(), resp->getLen());
      delete resp;
    }
  }
}

void cLiveStreamer::sendStreamStatus()
{
  cResponsePacket *resp = new cResponsePacket();
  if (!resp->initStream(VNSI_STREAM_STATUS, 0, 0, 0, 0, 0))
  {
    ERRORLOG("stream response packet init fail");
    delete resp;
    return;
  }
  uint16_t error = m_Demuxer.GetError();
  if (error & ERROR_PES_SCRAMBLE)
  {
    INFOLOG("Channel: scrambled %d", error);
    resp->add_String(cString::sprintf("Channel: scrambled (%d)", error));
  }
  else if (error & ERROR_PES_STARTCODE)
  {
    INFOLOG("Channel: startcode %d", error);
    resp->add_String(cString::sprintf("Channel: encrypted? (%d)", error));
  }
  else if (error & ERROR_DEMUX_NODATA)
  {
    INFOLOG("Channel: no data %d", error);
    resp->add_String(cString::sprintf("Channel: no data"));
  }
  else
  {
    INFOLOG("Channel: unknown error %d", error);
    resp->add_String(cString::sprintf("Channel: unknown error (%d)", error));
  }

  resp->finaliseStream();
  m_Socket->write(resp->getPtr(), resp->getLen());
  delete resp;
}

void cLiveStreamer::sendBufferStatus()
{
  cResponsePacket *resp = new cResponsePacket();
  if (!resp->initStream(VNSI_STREAM_BUFFERSTATS, 0, 0, 0, 0, 0))
  {
    ERRORLOG("stream response packet init fail");
    delete resp;
    return;
  }
  uint32_t start, end;
  bool timeshift;
  m_Demuxer.BufferStatus(timeshift, start, end);
  resp->add_U8(timeshift);
  resp->add_U32(start);
  resp->add_U32(end);
  resp->finaliseStream();
  m_Socket->write(resp->getPtr(), resp->getLen());
  delete resp;
}

void cLiveStreamer::sendRefTime(sStreamPacket *pkt)
{
  if(pkt == NULL)
    return;

  cResponsePacket *resp = new cResponsePacket();
  if (!resp->initStream(VNSI_STREAM_REFTIME, 0, 0, 0, 0, 0))
  {
    ERRORLOG("stream response packet init fail");
    delete resp;
    return;
  }

  resp->add_U32(pkt->reftime);
  resp->add_U64(pkt->pts);
  resp->finaliseStream();
  m_Socket->write(resp->getPtr(), resp->getLen());
  delete resp;
}

bool cLiveStreamer::SeekTime(int64_t time, uint32_t &serial)
{
  bool ret = m_Demuxer.SeekTime(time);
  serial = m_Demuxer.GetSerial();
  return ret;
}

void cLiveStreamer::RetuneChannel(const cChannel *channel)
{
  if (m_Channel != channel || !m_VideoInput.IsOpen())
    return;

  INFOLOG("re-tune to channel %s", m_Channel->Name());
  cMutexLock lock(&m_Mutex);
  m_IsRetune = true;
  m_Event.Broadcast();
}
