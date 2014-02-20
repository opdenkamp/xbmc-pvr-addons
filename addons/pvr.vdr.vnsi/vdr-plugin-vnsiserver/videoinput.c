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
 *
 */

#include "config.h"
#include "videoinput.h"
#include "videobuffer.h"
#include "vnsi.h"

#include <vdr/remux.h>
#include <vdr/channels.h>
#include <vdr/device.h>
#include <vdr/receiver.h>
#include <libsi/section.h>
#include <libsi/descriptor.h>

// --- cLiveReceiver -------------------------------------------------

class cLiveReceiver: public cReceiver
{
public:
  cLiveReceiver(cVideoInput *VideoInput, const cChannel *Channel, int Priority);
  virtual ~cLiveReceiver();
  cChannel m_PmtChannel;

protected:
  virtual void Activate(bool On);
  virtual void Receive(uchar *Data, int Length);

  cVideoInput *m_VideoInput;
};

cLiveReceiver::cLiveReceiver(cVideoInput *VideoInput, const cChannel *Channel, int Priority)
 : cReceiver(Channel, Priority)
 , m_VideoInput(VideoInput)
{
  SetPids(NULL);
}

cLiveReceiver::~cLiveReceiver()
{

}

//void cLiveReceiver
void cLiveReceiver::Receive(uchar *Data, int Length)
{
  m_VideoInput->Receive(Data, Length);
}

inline void cLiveReceiver::Activate(bool On)
{
  DEBUGLOG("activate live receiver: %d", On);
  if (!On)
  {
    m_VideoInput->Retune();
  }
}

// --- cLivePatFilter ----------------------------------------------------

class cLivePatFilter : public cFilter
{
private:
  int             m_pmtPid;
  int             m_pmtSid;
  int             m_pmtVersion;
  const cChannel *m_Channel;
  cVideoInput    *m_VideoInput;

  virtual void Process(u_short Pid, u_char Tid, const u_char *Data, int Length);

public:
  cLivePatFilter(cVideoInput *VideoInput, const cChannel *Channel);
};

cLivePatFilter::cLivePatFilter(cVideoInput *VideoInput, const cChannel *Channel)
{
  DEBUGLOG("cStreamdevPatFilter(\"%s\")", Channel->Name());
  m_Channel     = Channel;
  m_VideoInput  = VideoInput;
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
#if APIVERSNUM >= 10728
                     if (Setup.StandardCompliance == STANDARD_ANSISCTE)
#endif
                     { // DigiCipher II VIDEO (ANSI/SCTE 57)
                        Vpid = esPid;
                        Ppid = pmt.getPCRPid();
                        Vtype = 0x02; // compression based upon MPEG-2
                        ProcessCaDescriptors = true;
                        break;
                        }
                     // fall through
             case 0x81: // STREAMTYPE_USER_PRIVATE
#if APIVERSNUM >= 10728
                     if (Setup.StandardCompliance == STANDARD_ANSISCTE)
#endif
                     { // ATSC A/53 AUDIO (ANSI/SCTE 57)
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
#if APIVERSNUM >= 10728
                     if (Setup.StandardCompliance == STANDARD_ANSISCTE)
#endif
                     { // STANDARD SUBTITLE (ANSI/SCTE 27)
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
       DEBUGLOG("Pat/Pmt Filter received pmt change");
       cChannel *pmtChannel = m_VideoInput->PmtChannel();
       pmtChannel->Modification();
       pmtChannel->SetPids(Vpid, Ppid, Vtype, Apids, Atypes, ALangs, Dpids, Dtypes, DLangs, Spids, SLangs, Tpid);
       pmtChannel->SetSubtitlingDescriptors(SubtitlingTypes, CompositionPageIds, AncillaryPageIds);
       m_VideoInput->PmtChange(pmtChannel->Modification(CHANNELMOD_PIDS));
    }
  }
}

// ----------------------------------------------------------------------------

cVideoInput::cVideoInput(cCondVar &condVar, cMutex &mutex, bool &retune) :
    m_Event(condVar), m_Mutex(mutex), m_IsRetune(retune)
{
  m_Device = NULL;;
  m_PatFilter = NULL;
  m_Receiver = NULL;;
  m_Channel = NULL;
  m_VideoBuffer = NULL;
  m_Priority = 0;
  m_PmtChange = false;
}

cVideoInput::~cVideoInput()
{
  Close();
}

bool cVideoInput::Open(const cChannel *channel, int priority, cVideoBuffer *videoBuffer)
{
  m_VideoBuffer = videoBuffer;
  m_Channel = channel;
  m_Priority = priority;
  m_Device = cDevice::GetDevice(m_Channel, m_Priority, false);

  if (m_Device != NULL)
  {
    DEBUGLOG("Successfully found following device: %p (%d) for receiving", m_Device, m_Device ? m_Device->CardIndex() + 1 : 0);

    if (m_Device->SwitchChannel(m_Channel, false))
    {
      DEBUGLOG("Creating new live Receiver");
      m_SeenPmt = false;
      m_PatFilter = new cLivePatFilter(this, m_Channel);
      m_Receiver0 = new cLiveReceiver(this, m_Channel, m_Priority);
      m_Receiver = new cLiveReceiver(this, m_Channel, m_Priority);
      m_Device->AttachReceiver(m_Receiver0);
      m_Device->AttachFilter(m_PatFilter);
      m_VideoBuffer->AttachInput(true);
      Start();
      return true;
    }
  }
  return false;
}

void cVideoInput::Close()
{
  Cancel(5);

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

    if (m_Receiver0)
    {
      DEBUGLOG("Detaching Live Receiver0");
      m_Device->Detach(m_Receiver0);
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

    if (m_Receiver)
    {
      DEBUGLOG("Deleting Live Receiver");
      DELETENULL(m_Receiver);
    }

    if (m_Receiver0)
    {
      DEBUGLOG("Deleting Live Receiver0");
      DELETENULL(m_Receiver0);
    }

    if (m_PatFilter)
    {
      DEBUGLOG("Deleting Live Filter");
      DELETENULL(m_PatFilter);
    }
  }
  m_Channel = NULL;
  if (m_VideoBuffer)
  {
    m_VideoBuffer->AttachInput(false);
  }
}

bool cVideoInput::IsOpen()
{
  if (m_Channel)
    return true;
  else
    return false;
}

cChannel *cVideoInput::PmtChannel()
{
  return &m_Receiver->m_PmtChannel;
}

void cVideoInput::PmtChange(int pidChange)
{
  if (pidChange)
  {
    INFOLOG("Video Input - new pmt, attaching receiver");
    m_Device->Detach(m_Receiver);
    m_Receiver->SetPids(NULL);
    m_Receiver->SetPids(&m_Receiver->m_PmtChannel);
    m_Receiver->AddPid(m_Receiver->m_PmtChannel.Tpid());
    m_PmtChange = true;
    m_Device->AttachReceiver(m_Receiver);
    m_SeenPmt = true;
  }
}

inline void cVideoInput::Receive(uchar *data, int length)
{
  if (m_PmtChange)
  {
     // generate pat/pmt so we can configure parsers later
     cPatPmtGenerator patPmtGenerator(&m_Receiver->m_PmtChannel);
     m_VideoBuffer->Put(patPmtGenerator.GetPat(), TS_SIZE);
     int Index = 0;
     while (uchar *pmt = patPmtGenerator.GetPmt(Index))
       m_VideoBuffer->Put(pmt, TS_SIZE);
     m_PmtChange = false;
  }
  m_VideoBuffer->Put(data, length);
}

void cVideoInput::Retune()
{
  cMutexLock lock(&m_Mutex);
  m_IsRetune = true;
  m_Event.Broadcast();
}

void cVideoInput::Action()
{
  cTimeMs starttime;

  while (Running())
  {
    if (starttime.Elapsed() > (unsigned int)PmtTimeout*1000)
    {
      INFOLOG("VideoInput: no pat/pmt within timeout, falling back to channel pids");
      m_Receiver->m_PmtChannel = *m_Channel;
      PmtChange(true);
    }
    if (m_SeenPmt)
      break;

    usleep(1000);
  }
}
