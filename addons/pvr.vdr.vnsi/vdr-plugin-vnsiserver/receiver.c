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

#include <libsi/section.h>
#include <libsi/descriptor.h>

#include <vdr/remux.h>
#include <vdr/channels.h>
#include <asm/byteorder.h>

#include "config.h"
#include "receiver.h"
#include "cxsocket.h"
#include "vnsicommand.h"
#include "responsepacket.h"
#include "vnsi.h"

// --- cLiveReceiver -------------------------------------------------

class cLiveReceiver: public cReceiver
{
  friend class cLiveStreamer;

private:
  cLiveStreamer *m_Streamer;

protected:
  virtual void Activate(bool On);
  virtual void Receive(uchar *Data, int Length);

public:
  cLiveReceiver(cLiveStreamer *Streamer, const cChannel *Channel, int Priority, const int *Pids);
  virtual ~cLiveReceiver();
};

cLiveReceiver::cLiveReceiver(cLiveStreamer *Streamer, const cChannel *Channel, int Priority, const int *Pids)
 : cReceiver(Channel, Priority)
 , m_Streamer(Streamer)
{
  SetPids(NULL);
  AddPids(Pids);
  DEBUGLOG("Starting live receiver");
}

cLiveReceiver::~cLiveReceiver()
{
  DEBUGLOG("Killing live receiver");
}

//void cLiveReceiver
void cLiveReceiver::Receive(uchar *Data, int Length)
{
  m_Streamer->Receive(Data, Length);
}

inline void cLiveReceiver::Activate(bool On)
{
  m_Streamer->Activate(On);
}

// --- cLivePatFilter ----------------------------------------------------

class cLivePatFilter : public cFilter
{
private:
  int             m_pmtPid;
  int             m_pmtSid;
  int             m_pmtVersion;
  const cChannel *m_Channel;
  cLiveStreamer  *m_Streamer;

  virtual void Process(u_short Pid, u_char Tid, const u_char *Data, int Length);

public:
  cLivePatFilter(cLiveStreamer *Streamer, const cChannel *Channel);
};

cLivePatFilter::cLivePatFilter(cLiveStreamer *Streamer, const cChannel *Channel)
{
  DEBUGLOG("cStreamdevPatFilter(\"%s\")", Channel->Name());
  m_Channel     = Channel;
  m_Streamer    = Streamer;
  m_pmtPid      = 0;
  m_pmtSid      = 0;
  m_pmtVersion  = -1;
  Set(0x00, 0x00);  // PAT

}

void cLivePatFilter::Process(u_short Pid, u_char Tid, const u_char *Data, int Length)
{
  if (Pid == 0x00)
  {
    if (Tid == 0x00)
    {
      SI::PAT pat(Data, false);
      if (!pat.CheckCRCAndParse())
        return;
      SI::PAT::Association assoc;
      for (SI::Loop::Iterator it; pat.associationLoop.getNext(assoc, it); )
      {
        if (!assoc.isNITPid())
        {
          const cChannel *Channel =  Channels.GetByServiceID(Source(), Transponder(), assoc.getServiceId());
          if (Channel && (Channel == m_Channel))
          {
            int prevPmtPid = m_pmtPid;
            if (0 != (m_pmtPid = assoc.getPid()))
            {
              if (m_pmtPid != prevPmtPid)
              {
                m_pmtSid = assoc.getServiceId();
                Add(m_pmtPid, 0x02);
                m_pmtVersion = -1;
                break;
              }
              return;
            }
          }
        }
      }
    }
  }
  else if (Pid == m_pmtPid && Tid == SI::TableIdPMT && Source() && Transponder())
  {
    SI::PMT pmt(Data, false);
    if (!pmt.CheckCRCAndParse())
      return;
    if (pmt.getServiceId() != m_pmtSid)
      return; // skip broken PMT records
    if (m_pmtVersion != -1)
    {
      if (m_pmtVersion != pmt.getVersionNumber())
      {
        cFilter::Del(m_pmtPid, 0x02);
        m_pmtPid = 0; // this triggers PAT scan
      }
      return;
    }
    m_pmtVersion = pmt.getVersionNumber();

    cChannel *Channel = Channels.GetByServiceID(Source(), Transponder(), pmt.getServiceId());
    if (Channel) {
       // Scan the stream-specific loop:
       SI::PMT::Stream stream;
       int Vpid = 0;
       int Ppid = 0;
       int Vtype = 0;
       int Apids[MAXAPIDS + 1] = { 0 }; // these lists are zero-terminated
       int Atypes[MAXAPIDS + 1] = { 0 };
       int Dpids[MAXDPIDS + 1] = { 0 };
       int Dtypes[MAXDPIDS + 1] = { 0 };
       int Spids[MAXSPIDS + 1] = { 0 };
       uchar SubtitlingTypes[MAXSPIDS + 1] = { 0 };
       uint16_t CompositionPageIds[MAXSPIDS + 1] = { 0 };
       uint16_t AncillaryPageIds[MAXSPIDS + 1] = { 0 };
       char ALangs[MAXAPIDS][MAXLANGCODE2] = { "" };
       char DLangs[MAXDPIDS][MAXLANGCODE2] = { "" };
       char SLangs[MAXSPIDS][MAXLANGCODE2] = { "" };
       int Tpid = 0;
       int NumApids = 0;
       int NumDpids = 0;
       int NumSpids = 0;
       for (SI::Loop::Iterator it; pmt.streamLoop.getNext(stream, it); ) {
           bool ProcessCaDescriptors = false;
           int esPid = stream.getPid();
           switch (stream.getStreamType()) {
             case 1: // STREAMTYPE_11172_VIDEO
             case 2: // STREAMTYPE_13818_VIDEO
             case 0x1B: // MPEG4
                     Vpid = esPid;
                     Ppid = pmt.getPCRPid();
                     Vtype = stream.getStreamType();
                     ProcessCaDescriptors = true;
                     break;
             case 3: // STREAMTYPE_11172_AUDIO
             case 4: // STREAMTYPE_13818_AUDIO
             case 0x0F: // ISO/IEC 13818-7 Audio with ADTS transport syntax
             case 0x11: // ISO/IEC 14496-3 Audio with LATM transport syntax
                     {
                     if (NumApids < MAXAPIDS) {
                        Apids[NumApids] = esPid;
                        Atypes[NumApids] = stream.getStreamType();
                        SI::Descriptor *d;
                        for (SI::Loop::Iterator it; (d = stream.streamDescriptors.getNext(it)); ) {
                            switch (d->getDescriptorTag()) {
                              case SI::ISO639LanguageDescriptorTag: {
                                   SI::ISO639LanguageDescriptor *ld = (SI::ISO639LanguageDescriptor *)d;
                                   SI::ISO639LanguageDescriptor::Language l;
                                   char *s = ALangs[NumApids];
                                   int n = 0;
                                   for (SI::Loop::Iterator it; ld->languageLoop.getNext(l, it); ) {
                                       if (*ld->languageCode != '-') { // some use "---" to indicate "none"
                                          if (n > 0)
                                             *s++ = '+';
                                          strn0cpy(s, I18nNormalizeLanguageCode(l.languageCode), MAXLANGCODE1);
                                          s += strlen(s);
                                          if (n++ > 1)
                                             break;
                                          }
                                       }
                                   }
                                   break;
                              default: ;
                              }
                            delete d;
                            }
                        NumApids++;
                        }
                     ProcessCaDescriptors = true;
                     }
                     break;
             case 5: // STREAMTYPE_13818_PRIVATE
             case 6: // STREAMTYPE_13818_PES_PRIVATE
             //XXX case 8: // STREAMTYPE_13818_DSMCC
                     {
                     int dpid = 0;
                     int dtype = 0;
                     char lang[MAXLANGCODE1] = { 0 };
                     SI::Descriptor *d;
                     for (SI::Loop::Iterator it; (d = stream.streamDescriptors.getNext(it)); ) {
                         switch (d->getDescriptorTag()) {
                           case SI::AC3DescriptorTag:
                           case SI::EnhancedAC3DescriptorTag:
                                dpid = esPid;
                                dtype = d->getDescriptorTag();
                                ProcessCaDescriptors = true;
                                break;
                           case SI::SubtitlingDescriptorTag:
                                if (NumSpids < MAXSPIDS) {
                                   Spids[NumSpids] = esPid;
                                   SI::SubtitlingDescriptor *sd = (SI::SubtitlingDescriptor *)d;
                                   SI::SubtitlingDescriptor::Subtitling sub;
                                   char *s = SLangs[NumSpids];
                                   int n = 0;
                                   for (SI::Loop::Iterator it; sd->subtitlingLoop.getNext(sub, it); ) {
                                       if (sub.languageCode[0]) {
                                          SubtitlingTypes[NumSpids] = sub.getSubtitlingType();
                                          CompositionPageIds[NumSpids] = sub.getCompositionPageId();
                                          AncillaryPageIds[NumSpids] = sub.getAncillaryPageId();
                                          if (n > 0)
                                             *s++ = '+';
                                          strn0cpy(s, I18nNormalizeLanguageCode(sub.languageCode), MAXLANGCODE1);
                                          s += strlen(s);
                                          if (n++ > 1)
                                             break;
                                          }
                                       }
                                   NumSpids++;
                                   }
                                break;
                           case SI::TeletextDescriptorTag:
                                Tpid = esPid;
                                break;
                           case SI::ISO639LanguageDescriptorTag: {
                                SI::ISO639LanguageDescriptor *ld = (SI::ISO639LanguageDescriptor *)d;
                                strn0cpy(lang, I18nNormalizeLanguageCode(ld->languageCode), MAXLANGCODE1);
                                }
                                break;
                           default: ;
                           }
                         delete d;
                         }
                     if (dpid) {
                        if (NumDpids < MAXDPIDS) {
                           Dpids[NumDpids] = dpid;
                           Dtypes[NumDpids] = dtype;
                           strn0cpy(DLangs[NumDpids], lang, MAXLANGCODE1);
                           NumDpids++;
                           }
                        }
                     }
                     break;
             case 0x80: // STREAMTYPE_USER_PRIVATE
                     if (Setup.StandardCompliance == STANDARD_ANSISCTE) { // DigiCipher II VIDEO (ANSI/SCTE 57)
                        Vpid = esPid;
                        Ppid = pmt.getPCRPid();
                        Vtype = 0x02; // compression based upon MPEG-2
                        ProcessCaDescriptors = true;
                        break;
                        }
                     // fall through
             case 0x81: // STREAMTYPE_USER_PRIVATE
                     if (Setup.StandardCompliance == STANDARD_ANSISCTE) { // ATSC A/53 AUDIO (ANSI/SCTE 57)
                        char lang[MAXLANGCODE1] = { 0 };
                        SI::Descriptor *d;
                        for (SI::Loop::Iterator it; (d = stream.streamDescriptors.getNext(it)); ) {
                            switch (d->getDescriptorTag()) {
                              case SI::ISO639LanguageDescriptorTag: {
                                   SI::ISO639LanguageDescriptor *ld = (SI::ISO639LanguageDescriptor *)d;
                                   strn0cpy(lang, I18nNormalizeLanguageCode(ld->languageCode), MAXLANGCODE1);
                                   }
                                   break;
                              default: ;
                              }
                           delete d;
                           }
                        if (NumDpids < MAXDPIDS) {
                           Dpids[NumDpids] = esPid;
                           Dtypes[NumDpids] = SI::AC3DescriptorTag;
                           strn0cpy(DLangs[NumDpids], lang, MAXLANGCODE1);
                           NumDpids++;
                           }
                        ProcessCaDescriptors = true;
                        break;
                        }
                     // fall through
             case 0x82: // STREAMTYPE_USER_PRIVATE
                     if (Setup.StandardCompliance == STANDARD_ANSISCTE) { // STANDARD SUBTITLE (ANSI/SCTE 27)
                        //TODO
                        break;
                        }
                     // fall through
             case 0x83 ... 0xFF: // STREAMTYPE_USER_PRIVATE
                     {
                     char lang[MAXLANGCODE1] = { 0 };
                     bool IsAc3 = false;
                     SI::Descriptor *d;
                     for (SI::Loop::Iterator it; (d = stream.streamDescriptors.getNext(it)); ) {
                         switch (d->getDescriptorTag()) {
                           case SI::RegistrationDescriptorTag: {
                                SI::RegistrationDescriptor *rd = (SI::RegistrationDescriptor *)d;
                                // http://www.smpte-ra.org/mpegreg/mpegreg.html
                                switch (rd->getFormatIdentifier()) {
                                  case 0x41432D33: // 'AC-3'
                                       IsAc3 = true;
                                       break;
                                  default:
                                       //printf("Format identifier: 0x%08X (pid: %d)\n", rd->getFormatIdentifier(), esPid);
                                       break;
                                  }
                                }
                                break;
                           case SI::ISO639LanguageDescriptorTag: {
                                SI::ISO639LanguageDescriptor *ld = (SI::ISO639LanguageDescriptor *)d;
                                strn0cpy(lang, I18nNormalizeLanguageCode(ld->languageCode), MAXLANGCODE1);
                                }
                                break;
                           default: ;
                           }
                        delete d;
                        }
                     if (IsAc3) {
                        if (NumDpids < MAXDPIDS) {
                           Dpids[NumDpids] = esPid;
                           Dtypes[NumDpids] = SI::AC3DescriptorTag;
                           strn0cpy(DLangs[NumDpids], lang, MAXLANGCODE1);
                           NumDpids++;
                           }
                        ProcessCaDescriptors = true;
                        }
                     }
                     break;
             default: ;//printf("PID: %5d %5d %2d %3d %3d\n", pmt.getServiceId(), stream.getPid(), stream.getStreamType(), pmt.getVersionNumber(), Channel->Number());
             }
           }
       cChannel pmtChannel(*Channel);
       pmtChannel.SetPids(Vpid, Ppid, Vtype, Apids, Atypes, ALangs, Dpids, Dtypes, DLangs, Spids, SLangs, Tpid);
       pmtChannel.SetSubtitlingDescriptors(SubtitlingTypes, CompositionPageIds, AncillaryPageIds);
       m_Streamer->SetTpid(Tpid);
       cPatPmtGenerator patPmtGenerator(&pmtChannel);
       m_Streamer->m_bufferLock.Lock();
       m_Streamer->Put(patPmtGenerator.GetPat(), TS_SIZE);
       int Index = 0;
       while (uchar *pmt = patPmtGenerator.GetPmt(Index))
         m_Streamer->Put(pmt, TS_SIZE);
       m_Streamer->m_bufferLock.Unlock();
       }
  }
}

// --- cLiveStreamer -------------------------------------------------

cLiveStreamer::cLiveStreamer(uint32_t timeout)
 : cThread("cLiveStreamer stream processor")
 , cRingBufferLinear(MEGABYTE(3), TS_SIZE * 2, true)
 , m_scanTimeout(timeout)
{
  m_Channel         = NULL;
  m_Priority        = 0;
  m_Socket          = NULL;
  m_Device          = NULL;
  m_Receiver        = NULL;
  m_PatFilter       = NULL;
  m_Frontend        = -1;
  m_streamReady     = false;
  m_IsAudioOnly     = false;
  m_IsMPEGPS        = false;
  m_startup         = true;
  m_SignalLost      = false;
  m_IFrameSeen      = false;
  m_PidChange       = false;
  m_Tpid            = 0;

  m_requestStreamChange = false;

  m_packetEmpty = new cResponsePacket;
  m_packetEmpty->initStream(VNSI_STREAM_MUXPKT, 0, 0, 0, 0);

  memset(&m_FrontendInfo, 0, sizeof(m_FrontendInfo));
  for (int idx = 0; idx < MAXRECEIVEPIDS; ++idx)
  {
    m_Pids[idx]    = 0;
  }
  m_Streams.clear();

  if(m_scanTimeout == 0)
    m_scanTimeout = VNSIServerConfig.stream_timeout;

  SetTimeouts(0, 100);
}

cLiveStreamer::~cLiveStreamer()
{
  DEBUGLOG("Started to delete live streamer");

  Cancel(-1);

  if (m_Device)
  {
    if (m_Receiver)
    {
      DEBUGLOG("Detaching Live Receiver");
      m_Device->Detach(m_Receiver);
    }
    else
    {
      DEBUGLOG("No live receiver present");
    }

    if (m_PatFilter)
    {
      DEBUGLOG("Detaching Live Filter");
      m_Device->Detach(m_PatFilter);
    }
    else
    {
      DEBUGLOG("No live filter present");
    }

    for (std::list<cTSDemuxer*>::iterator it = m_Demuxers.begin(); it != m_Demuxers.end(); ++it)
    {
      DEBUGLOG("Deleting stream demuxer for pid=%i and type=%i", (*it)->GetPID(), (*it)->Type());
      delete (*it);
    }
    m_Demuxers.clear();

    if (m_Receiver)
    {
      DEBUGLOG("Deleting Live Receiver");
      DELETENULL(m_Receiver);
    }

    if (m_PatFilter)
    {
      DEBUGLOG("Deleting Live Filter");
      DELETENULL(m_PatFilter);
    }
  }
  if (m_Frontend >= 0)
  {
    close(m_Frontend);
    m_Frontend = -1;
  }

  delete m_packetEmpty;

  DEBUGLOG("Finished to delete live streamer");
}

void cLiveStreamer::Receive(uchar *Data, int Length)
{
  m_bufferLock.Lock();
  int p = Put(Data, Length);

  if (p != Length)
    ReportOverflow(Length - p);
  m_bufferLock.Unlock();
}

void cLiveStreamer::Action(void)
{
  int size              = 0;
  int used              = 0;
  unsigned char *buf    = NULL;
  m_startup             = true;
  cTimeMs last_info;
  cTimeMs starttime;
  m_last_tick.Set(0);
  cPatPmtParser patPmtParser;
  cTSDemuxer *demuxer;
  int oldPmtVersion = -1;

  while (Running())
  {
    size = 0;
    used = 0;
    buf = Get(size);

    if (!m_Receiver->IsAttached())
    {
      INFOLOG("returning from streamer thread, receiver is no more attached");
      break;
    }

    // if we got no pmt, create demuxers with info in channels.conf
    if (m_Demuxers.size() == 0 && starttime.Elapsed() > (unsigned int)PmtTimeout*1000)
    {
      INFOLOG("Got no PMT, using channel conf for creating demuxers");
      confChannelDemuxers(m_Channel);
    }

    // no data
    if (buf == NULL || size <= TS_SIZE)
    {
      // keep client going
      if(m_last_tick.Elapsed() >= (uint64_t)(m_scanTimeout*1000))
      {
        INFOLOG("No Signal");
        sendStreamStatus();
        m_last_tick.Set(0);
        m_SignalLost = true;
      }
      continue;
    }

    /* Make sure we are looking at a TS packet */
    while (size > TS_SIZE)
    {
      if (buf[0] == TS_SYNC_BYTE && buf[TS_SIZE] == TS_SYNC_BYTE)
        break;
      used++;
      buf++;
      size--;
    }

    // Send stream information as the first packet on startup
    if (IsStarting() && IsReady())
    {
      INFOLOG("streaming of channel started");
      last_info.Set(0);
      m_last_tick.Set(0);
      m_requestStreamChange = true;
      m_startup = false;
    }

    while (size >= TS_SIZE)
    {
      if(!Running())
      {
        break;
      }

      int ts_pid = TsPid(buf);

      if (ts_pid == PATPID)
      {
        patPmtParser.ParsePat(buf, TS_SIZE);
      }
      else if (ts_pid == patPmtParser.PmtPid())
      {
        int patVersion, pmtVersion;
        patPmtParser.ParsePmt(buf, TS_SIZE);
        if (patPmtParser.GetVersions(patVersion, pmtVersion))
        {
          if (pmtVersion != oldPmtVersion)
          {
            cChannel pmtChannel(*m_Channel);
            setChannelPids(&pmtChannel, &patPmtParser);
            confChannelDemuxers(&pmtChannel);
            patPmtParser.Reset();
            oldPmtVersion = pmtVersion;
          }
        }
      }
      else if (demuxer = FindStreamDemuxer(ts_pid))
      {
        if (!demuxer->ProcessTSPacket(buf))
        {
          used += TS_SIZE;
          break;
        }
      }
      else
        INFOLOG("no muxer found");

      buf += TS_SIZE;
      size -= TS_SIZE;
      used += TS_SIZE;
    }
    Del(used);

    if(last_info.Elapsed() >= 10*1000 && IsReady())
    {
      last_info.Set(0);
      sendStreamInfo();
      sendSignalInfo();
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
  m_Device    = cDevice::GetDevice(channel, m_Priority, true);

  if (m_Device != NULL)
  {
    DEBUGLOG("Successfully found following device: %p (%d) for receiving", m_Device, m_Device ? m_Device->CardIndex() + 1 : 0);

    if (m_Device->SwitchChannel(m_Channel, false))
    {
      if (!m_Channel->Vpid())
      {
        /* m_streamReady is set by the Video demuxers, to have always valid stream informations
         * like height and width. But if no Video PID is present like for radio channels
         * VNSI will deadlock
         */
        m_streamReady = true;
        m_IsAudioOnly = true;
      }

      /* Send the OK response here, that it is before the Stream end message */
      resp->add_U32(VNSI_RET_OK);
      resp->finalise();
      m_Socket->write(resp->getPtr(), resp->getLen());

      if (m_Channel && ((m_Channel->Source() >> 24) == 'V')) m_IsMPEGPS = true;

      if (m_Socket)
      {
        dsyslog("VNSI: Creating new live Receiver");
        m_PatFilter = new cLivePatFilter(this, m_Channel);
        m_Device->AttachFilter(m_PatFilter);
        m_Receiver = new cLiveReceiver(this, m_Channel, m_Priority, m_Pids);
        m_Device->AttachReceiver(m_Receiver);
      }

      INFOLOG("Successfully switched to channel %i - %s", m_Channel->Number(), m_Channel->Name());
      return true;
    }
    else
    {
      ERRORLOG("Can't switch to channel %i - %s", m_Channel->Number(), m_Channel->Name());
    }
  }
  else
  {
    ERRORLOG("Can't get device for channel %i - %s", m_Channel->Number(), m_Channel->Name());
  }
  return false;
}

cTSDemuxer *cLiveStreamer::FindStreamDemuxer(int Pid)
{
  for (std::list<cTSDemuxer*>::iterator it = m_Demuxers.begin(); it != m_Demuxers.end(); ++it)
  {
    if (Pid == (*it)->GetPID())
      return *it;
  }
  return NULL;
}

void cLiveStreamer::AddStream(sStream &stream)
{
  m_Streams.push_back(stream);
}

inline void cLiveStreamer::Activate(bool On)
{
  if (m_PidChange)
    return;

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

  if(!m_IsAudioOnly && !m_IFrameSeen && (pkt->frametype != PKT_I_FRAME))
    return;

  if(m_requestStreamChange)
    sendStreamChange();

  m_IFrameSeen = true;

  if (!m_streamHeader.initStream(VNSI_STREAM_MUXPKT, pkt->id, pkt->duration, pkt->pts, pkt->dts))
  {
    ERRORLOG("stream response packet init fail");
    return;
  }
  m_streamHeader.setLen(m_streamHeader.getStreamHeaderLength() + pkt->size);
  m_streamHeader.finaliseStream();

  m_Socket->write(m_streamHeader.getPtr(), m_streamHeader.getStreamHeaderLength(), -1, true);
  m_Socket->write(pkt->data, pkt->size);

  m_last_tick.Set(0);
  m_SignalLost = false;
}

void cLiveStreamer::sendStreamChange()
{
  cResponsePacket *resp = new cResponsePacket();
  if (!resp->initStream(VNSI_STREAM_CHANGE, 0, 0, 0, 0))
  {
    ERRORLOG("stream response packet init fail");
    delete resp;
    return;
  }

  for (std::list<cTSDemuxer*>::iterator it = m_Demuxers.begin(); it != m_Demuxers.end(); ++it)
  {
    resp->add_U32((*it)->GetPID());
    if ((*it)->Type() == stMPEG2AUDIO)
    {
      resp->add_String("MPEG2AUDIO");
      resp->add_String((*it)->GetLanguage());
      resp->add_U32((*it)->GetChannels());
      resp->add_U32((*it)->GetSampleRate());
      resp->add_U32((*it)->GetBlockAlign());
      resp->add_U32((*it)->GetBitRate());
      resp->add_U32((*it)->GetBitsPerSample());
    }
    else if ((*it)->Type() == stMPEG2VIDEO)
    {
      resp->add_String("MPEG2VIDEO");
      resp->add_U32((*it)->GetFpsScale());
      resp->add_U32((*it)->GetFpsRate());
      resp->add_U32((*it)->GetHeight());
      resp->add_U32((*it)->GetWidth());
      resp->add_double((*it)->GetAspect());
    }
    else if ((*it)->Type() == stAC3)
    {
      resp->add_String("AC3");
      resp->add_String((*it)->GetLanguage());
      resp->add_U32((*it)->GetChannels());
      resp->add_U32((*it)->GetSampleRate());
      resp->add_U32((*it)->GetBlockAlign());
      resp->add_U32((*it)->GetBitRate());
      resp->add_U32((*it)->GetBitsPerSample());
    }
    else if ((*it)->Type() == stH264)
    {
      resp->add_String("H264");
      resp->add_U32((*it)->GetFpsScale());
      resp->add_U32((*it)->GetFpsRate());
      resp->add_U32((*it)->GetHeight());
      resp->add_U32((*it)->GetWidth());
      resp->add_double((*it)->GetAspect());
    }
    else if ((*it)->Type() == stDVBSUB)
    {
      resp->add_String("DVBSUB");
      resp->add_String((*it)->GetLanguage());
      resp->add_U32((*it)->CompositionPageId());
      resp->add_U32((*it)->AncillaryPageId());
    }
    else if ((*it)->Type() == stTELETEXT)
    {
      resp->add_String("TELETEXT");
      resp->add_String((*it)->GetLanguage());
      resp->add_U32((*it)->CompositionPageId());
      resp->add_U32((*it)->AncillaryPageId());
    }
    else if ((*it)->Type() == stAACADST)
    {
      resp->add_String("AAC");
      resp->add_String((*it)->GetLanguage());
      resp->add_U32((*it)->GetChannels());
      resp->add_U32((*it)->GetSampleRate());
      resp->add_U32((*it)->GetBlockAlign());
      resp->add_U32((*it)->GetBitRate());
      resp->add_U32((*it)->GetBitsPerSample());
    }
    else if ((*it)->Type() == stAACLATM)
    {
      resp->add_String("AACLATM");
      resp->add_String((*it)->GetLanguage());
      resp->add_U32((*it)->GetChannels());
      resp->add_U32((*it)->GetSampleRate());
      resp->add_U32((*it)->GetBlockAlign());
      resp->add_U32((*it)->GetBitRate());
      resp->add_U32((*it)->GetBitsPerSample());
    }
    else if ((*it)->Type() == stEAC3)
    {
      resp->add_String("EAC3");
      resp->add_String((*it)->GetLanguage());
      resp->add_U32((*it)->GetChannels());
      resp->add_U32((*it)->GetSampleRate());
      resp->add_U32((*it)->GetBlockAlign());
      resp->add_U32((*it)->GetBitRate());
      resp->add_U32((*it)->GetBitsPerSample());
    }
    else if ((*it)->Type() == stDTS)
    {
      resp->add_String("DTS");
      resp->add_String((*it)->GetLanguage());
      resp->add_U32((*it)->GetChannels());
      resp->add_U32((*it)->GetSampleRate());
      resp->add_U32((*it)->GetBlockAlign());
      resp->add_U32((*it)->GetBitRate());
      resp->add_U32((*it)->GetBitsPerSample());
    }
  }

  resp->finaliseStream();
  m_Socket->write(resp->getPtr(), resp->getLen(), -1, true);
  delete resp;

  m_requestStreamChange = false;

  sendStreamInfo();
}

void cLiveStreamer::sendSignalInfo()
{
  /* If no frontend is found m_Frontend is set to -2, in this case
     return a empty signalinfo package */
  if (m_Frontend == -2)
  {
    cResponsePacket *resp = new cResponsePacket();
    if (!resp->initStream(VNSI_STREAM_SIGNALINFO, 0, 0, 0, 0))
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
    m_Socket->write(resp->getPtr(), resp->getLen(), -1, true);
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
      if (!resp->initStream(VNSI_STREAM_SIGNALINFO, 0, 0, 0, 0))
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
      m_Socket->write(resp->getPtr(), resp->getLen(), -1, true);
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
      if (!resp->initStream(VNSI_STREAM_SIGNALINFO, 0, 0, 0, 0))
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
      m_Socket->write(resp->getPtr(), resp->getLen(), -1, true);
      delete resp;
    }
  }
}

void cLiveStreamer::sendStreamInfo()
{
  if(m_Demuxers.size() == 0)
  {
    return;
  }

  cResponsePacket *resp = new cResponsePacket();
  if (!resp->initStream(VNSI_STREAM_CONTENTINFO, 0, 0, 0, 0))
  {
    ERRORLOG("stream response packet init fail");
    delete resp;
    return;
  }

  for (std::list<cTSDemuxer*>::iterator it = m_Demuxers.begin(); it != m_Demuxers.end(); ++it)
  {
    if ((*it)->Type() == stMPEG2AUDIO ||
        (*it)->Type() == stAC3 ||
        (*it)->Type() == stEAC3 ||
        (*it)->Type() == stDTS ||
        (*it)->Type() == stAACADST ||
        (*it)->Type() == stAACLATM)
    {
      resp->add_U32((*it)->GetPID());
      resp->add_String((*it)->GetLanguage());
      resp->add_U32((*it)->GetChannels());
      resp->add_U32((*it)->GetSampleRate());
      resp->add_U32((*it)->GetBlockAlign());
      resp->add_U32((*it)->GetBitRate());
      resp->add_U32((*it)->GetBitsPerSample());
    }
    else if ((*it)->Type() == stMPEG2VIDEO || (*it)->Type() == stH264)
    {
      resp->add_U32((*it)->GetPID());
      resp->add_U32((*it)->GetFpsScale());
      resp->add_U32((*it)->GetFpsRate());
      resp->add_U32((*it)->GetHeight());
      resp->add_U32((*it)->GetWidth());
      resp->add_double((*it)->GetAspect());
    }
    else if ((*it)->Type() == stDVBSUB)
    {
      resp->add_U32((*it)->GetPID());
      resp->add_String((*it)->GetLanguage());
      resp->add_U32((*it)->CompositionPageId());
      resp->add_U32((*it)->AncillaryPageId());
    }
  }

  resp->finaliseStream();
  m_Socket->write(resp->getPtr(), resp->getLen());
  delete resp;
}

void cLiveStreamer::sendStreamStatus()
{
  cResponsePacket *resp = new cResponsePacket();
  if (!resp->initStream(VNSI_STREAM_STATUS, 0, 0, 0, 0))
  {
    ERRORLOG("stream response packet init fail");
    delete resp;
    return;
  }
  resp->add_String("No Signal");
  resp->finaliseStream();
  m_Socket->write(resp->getPtr(), resp->getLen());
  delete resp;
}

void cLiveStreamer::ensureDemuxers()
{
  std::list<cTSDemuxer*>::iterator it = m_Demuxers.begin();
  while (it != m_Demuxers.end())
  {
    std::list<sStream>::iterator its;
    for (its = m_Streams.begin(); its != m_Streams.end(); ++its)
    {
      if ((its->pID == (*it)->GetPID()) && (its->type == (*it)->Type()))
      {
        break;
      }
    }
    if (its == m_Streams.end())
    {
      INFOLOG("Deleting stream demuxer for pid=%i and type=%i", (*it)->GetPID(), (*it)->Type());
      m_Demuxers.erase(it);
      it = m_Demuxers.begin();
      m_requestStreamChange = true;
    }
    else
      ++it;
  }

  for (std::list<sStream>::iterator it = m_Streams.begin(); it != m_Streams.end(); ++it)
  {
    cTSDemuxer *demuxer = FindStreamDemuxer(it->pID);
    if (demuxer)
    {
      demuxer->SetLanguage(it->language);
      continue;
    }

    if (it->type == stH264)
    {
      demuxer = new cTSDemuxer(this, stH264, it->pID);
    }
    else if (it->type == stMPEG2VIDEO)
    {
      demuxer = new cTSDemuxer(this, stMPEG2VIDEO, it->pID);
    }
    else if (it->type == stMPEG2AUDIO)
    {
      demuxer = new cTSDemuxer(this, stMPEG2AUDIO, it->pID);
      demuxer->SetLanguage(it->language);
    }
    else if (it->type == stAACADST)
    {
      demuxer = new cTSDemuxer(this, stAACADST, it->pID);
      demuxer->SetLanguage(it->language);
    }
    else if (it->type == stAACLATM)
    {
      demuxer = new cTSDemuxer(this, stAACLATM, it->pID);
      demuxer->SetLanguage(it->language);
    }
    else if (it->type == stAC3)
    {
      demuxer = new cTSDemuxer(this, stAC3, it->pID);
      demuxer->SetLanguage(it->language);
    }
    else if (it->type == stEAC3)
    {
      demuxer = new cTSDemuxer(this, stEAC3, it->pID);
      demuxer->SetLanguage(it->language);
    }
    else if (it->type == stDVBSUB)
    {
      demuxer = new cTSDemuxer(this, stDVBSUB, it->pID);
      demuxer->SetLanguage(it->language);
#if APIVERSNUM >= 10709
      demuxer->SetSubtitlingDescriptor(it->subtitlingType, it->compositionPageId, it->ancillaryPageId);
#endif
    }
    else if (it->type == stTELETEXT)
    {
      demuxer = new cTSDemuxer(this, stTELETEXT, it->pID);
    }
    else
      continue;

    m_Demuxers.push_back(demuxer);
    INFOLOG("Created stream demuxer for pid=%i and type=%i", demuxer->GetPID(), demuxer->Type());
    m_requestStreamChange = true;
  }

  if (m_requestStreamChange)
  {
    int index = 0;
    for (std::list<cTSDemuxer*>::iterator it = m_Demuxers.begin(); it != m_Demuxers.end(); ++it)
    {
      m_Pids[index] = (*it)->GetPID();
      index++;
    }
    m_Pids[index] = 0;

    if (m_Receiver)
    {
      m_PidChange = true;
      m_Device->Detach(m_Receiver);
      m_Receiver->SetPids(NULL);
      m_Receiver->AddPids(m_Pids);
      m_Device->AttachReceiver(m_Receiver);
      m_PidChange = false;
    }
  }

  m_Streams.clear();
}

void cLiveStreamer::confChannelDemuxers(const cChannel *channel)
{
  sStream newStream;
  if (channel->Vpid())
  {
    newStream.pID = channel->Vpid();
#if APIVERSNUM >= 10701
    if (channel->Vtype() == 0x1B)
      newStream.type = stH264;
    else
#endif
      newStream.type = stMPEG2VIDEO;

    AddStream(newStream);
  }

  const int *APids = channel->Apids();
  for ( ; *APids; APids++)
  {
    int index = 0;
    if (!FindStreamDemuxer(*APids))
    {
      newStream.pID = *APids;
      newStream.type = stMPEG2AUDIO;
#if APIVERSNUM >= 10715
      if (channel->Atype(index) == 0x0F)
        newStream.type = stAACADST;
      else if (channel->Atype(index) == 0x11)
        newStream.type = stAACLATM;
#endif
      newStream.SetLanguage(channel->Alang(index));
      AddStream(newStream);
    }
    index++;
  }

  const int *DPids = channel->Dpids();
  for ( ; *DPids; DPids++)
  {
    int index = 0;
    if (!FindStreamDemuxer(*DPids))
    {
      newStream.pID = *DPids;
      newStream.type = stAC3;
#if APIVERSNUM >= 10715
      if (channel->Dtype(index) == SI::EnhancedAC3DescriptorTag)
        newStream.type = stEAC3;
#endif
      newStream.SetLanguage(channel->Dlang(index));
      AddStream(newStream);
    }
    index++;
  }

  const int *SPids = channel->Spids();
  if (SPids)
  {
    int index = 0;
    for ( ; *SPids; SPids++)
    {
      if (!FindStreamDemuxer(*SPids))
      {
        newStream.pID = *SPids;
        newStream.type = stDVBSUB;
        newStream.SetLanguage(channel->Slang(index));
#if APIVERSNUM >= 10709
        newStream.subtitlingType = channel->SubtitlingType(index);
        newStream.compositionPageId = channel->CompositionPageId(index);
        newStream.ancillaryPageId = channel->AncillaryPageId(index);
#endif
        AddStream(newStream);
      }
      index++;
    }
  }

  if (m_Tpid)
  {
    newStream.pID = m_Tpid;
    newStream.type = stTELETEXT;
    AddStream(newStream);
  }

  ensureDemuxers();
}

void cLiveStreamer::setChannelPids(cChannel *channel, cPatPmtParser *patPmtParser)
{
  int Apids[MAXAPIDS + 1] = { 0 };
  int Atypes[MAXAPIDS + 1] = { 0 };
  int Dpids[MAXDPIDS + 1] = { 0 };
  int Dtypes[MAXDPIDS + 1] = { 0 };
  int Spids[MAXSPIDS + 1] = { 0 };
  char ALangs[MAXAPIDS][MAXLANGCODE2] = { "" };
  char DLangs[MAXDPIDS][MAXLANGCODE2] = { "" };
  char SLangs[MAXSPIDS][MAXLANGCODE2] = { "" };
  int index = 0;

  const int *aPids = patPmtParser->Apids();
  index = 0;
  for ( ; *aPids; aPids++)
  {
    Apids[index] = patPmtParser->Apid(index);
    Atypes[index] = patPmtParser->Atype(index);
    strn0cpy(ALangs[index], patPmtParser->Alang(index), MAXLANGCODE2);
    index++;
  }

  const int *dPids = patPmtParser->Dpids();
  index = 0;
  for ( ; *dPids; dPids++)
  {
    Dpids[index] = patPmtParser->Dpid(index);
    Dtypes[index] = patPmtParser->Dtype(index);
    strn0cpy(DLangs[index], patPmtParser->Dlang(index), MAXLANGCODE2);
    index++;
  }

  const int *sPids = patPmtParser->Spids();
  index = 0;
  for ( ; *sPids; sPids++)
  {
    Spids[index] = patPmtParser->Spid(index);
    strn0cpy(SLangs[index], patPmtParser->Slang(index), MAXLANGCODE2);
    index++;
  }

  int Vpid = patPmtParser->Vpid();
  int Ppid = patPmtParser->Ppid();
  int VType = patPmtParser->Vtype();
  int Tpid = m_Channel->Tpid();
  channel->SetPids(Vpid, Ppid, VType,
                   Apids, Atypes, ALangs,
                   Dpids, Dtypes, DLangs,
                   Spids, SLangs,
                   Tpid);
}
