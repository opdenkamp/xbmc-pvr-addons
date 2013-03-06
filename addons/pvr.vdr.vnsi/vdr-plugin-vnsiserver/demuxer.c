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
 */


#include "config.h"
#include "demuxer.h"
#include "parser.h"
#include "videobuffer.h"

#include <vdr/channels.h>
#include <libsi/si.h>

cVNSIDemuxer::cVNSIDemuxer()
{
  m_OldPmtVersion = -1;
}

cVNSIDemuxer::~cVNSIDemuxer()
{

}

void cVNSIDemuxer::Open(const cChannel &channel, cVideoBuffer *videoBuffer)
{
  m_CurrentChannel = channel;
  m_VideoBuffer = videoBuffer;
  m_OldPmtVersion = -1;

  if (m_CurrentChannel.Vpid())
    m_WaitIFrame = true;
  else
    m_WaitIFrame = false;
}

void cVNSIDemuxer::Close()
{
  for (std::list<cTSStream*>::iterator it = m_Streams.begin(); it != m_Streams.end(); ++it)
  {
    DEBUGLOG("Deleting stream parser for pid=%i and type=%i", (*it)->GetPID(), (*it)->Type());
    delete (*it);
  }
  m_Streams.clear();
  m_StreamInfos.clear();
}

int cVNSIDemuxer::Read(sStreamPacket *packet)
{
  uint8_t *buf;
  int len;
  cTSStream *stream;

  // clear packet
  if (!packet)
    return -1;
  packet->data = NULL;
  packet->streamChange = false;
  packet->pmtChange = false;

  // read TS Packet from buffer
  len = m_VideoBuffer->Read(&buf, TS_SIZE);
  if (len != TS_SIZE)
    return -1;

  int ts_pid = TsPid(buf);

  // parse PAT/PMT
  if (ts_pid == PATPID)
  {
    m_PatPmtParser.ParsePat(buf, TS_SIZE);
  }
#if APIVERSNUM >= 10733
  else if (m_PatPmtParser.IsPmtPid(ts_pid))
#else
  else if (ts_pid == m_PatPmtParser.PmtPid())
#endif
  {
    int patVersion, pmtVersion;
    m_PatPmtParser.ParsePmt(buf, TS_SIZE);
    if (m_PatPmtParser.GetVersions(patVersion, pmtVersion))
    {
      if (pmtVersion != m_OldPmtVersion)
      {
        cChannel pmtChannel(m_CurrentChannel);
        SetChannelPids(&pmtChannel, &m_PatPmtParser);
        SetChannelStreams(&pmtChannel);
        m_PatPmtParser.Reset();
        m_OldPmtVersion = pmtVersion;
        if (EnsureParsers())
        {
          packet->pmtChange = true;
            return 1;
        }
      }
    }
  }
  else if (stream = FindStream(ts_pid))
  {
    if (stream->ProcessTSPacket(buf, packet, m_WaitIFrame))
    {
      m_WaitIFrame = false;
      return 1;
    }
  }
  else
    return -1;

  return 0;
}

cTSStream *cVNSIDemuxer::GetFirstStream()
{
  m_StreamsIterator = m_Streams.begin();
  if (m_StreamsIterator != m_Streams.end())
    return *m_StreamsIterator;
  else
    return NULL;
}

cTSStream *cVNSIDemuxer::GetNextStream()
{
  ++m_StreamsIterator;
  if (m_StreamsIterator != m_Streams.end())
    return *m_StreamsIterator;
  else
    return NULL;
}

cTSStream *cVNSIDemuxer::FindStream(int Pid)
{
  for (std::list<cTSStream*>::iterator it = m_Streams.begin(); it != m_Streams.end(); ++it)
  {
    if (Pid == (*it)->GetPID())
      return *it;
  }
  return NULL;
}

void cVNSIDemuxer::AddStreamInfo(sStreamInfo &stream)
{
  m_StreamInfos.push_back(stream);
}

bool cVNSIDemuxer::EnsureParsers()
{
  bool streamChange = false;

  std::list<cTSStream*>::iterator it = m_Streams.begin();
  while (it != m_Streams.end())
  {
    std::list<sStreamInfo>::iterator its;
    for (its = m_StreamInfos.begin(); its != m_StreamInfos.end(); ++its)
    {
      if ((its->pID == (*it)->GetPID()) && (its->type == (*it)->Type()))
      {
        break;
      }
    }
    if (its == m_StreamInfos.end())
    {
      INFOLOG("Deleting stream for pid=%i and type=%i", (*it)->GetPID(), (*it)->Type());
      m_Streams.erase(it);
      it = m_Streams.begin();
      streamChange = true;
    }
    else
      ++it;
  }

  for (std::list<sStreamInfo>::iterator it = m_StreamInfos.begin(); it != m_StreamInfos.end(); ++it)
  {
    cTSStream *stream = FindStream(it->pID);
    if (stream)
    {
      // TODO: check for change in lang
      stream->SetLanguage(it->language);
      continue;
    }

    if (it->type == stH264)
    {
      stream = new cTSStream(stH264, it->pID);
    }
    else if (it->type == stMPEG2VIDEO)
    {
      stream = new cTSStream(stMPEG2VIDEO, it->pID);
    }
    else if (it->type == stMPEG2AUDIO)
    {
      stream = new cTSStream(stMPEG2AUDIO, it->pID);
      stream->SetLanguage(it->language);
    }
    else if (it->type == stAACADTS)
    {
      stream = new cTSStream(stAACADTS, it->pID);
      stream->SetLanguage(it->language);
    }
    else if (it->type == stAACLATM)
    {
      stream = new cTSStream(stAACLATM, it->pID);
      stream->SetLanguage(it->language);
    }
    else if (it->type == stAC3)
    {
      stream = new cTSStream(stAC3, it->pID);
      stream->SetLanguage(it->language);
    }
    else if (it->type == stEAC3)
    {
      stream = new cTSStream(stEAC3, it->pID);
      stream->SetLanguage(it->language);
    }
    else if (it->type == stDVBSUB)
    {
      stream = new cTSStream(stDVBSUB, it->pID);
      stream->SetLanguage(it->language);
#if APIVERSNUM >= 10709
      stream->SetSubtitlingDescriptor(it->subtitlingType, it->compositionPageId, it->ancillaryPageId);
#endif
    }
    else if (it->type == stTELETEXT)
    {
      stream = new cTSStream(stTELETEXT, it->pID);
    }
    else
      continue;

    m_Streams.push_back(stream);
    INFOLOG("Created stream for pid=%i and type=%i", stream->GetPID(), stream->Type());
    streamChange = true;
  }
  m_StreamInfos.clear();

  return streamChange;
}

void cVNSIDemuxer::SetChannelStreams(const cChannel *channel)
{
  sStreamInfo newStream;
  if (channel->Vpid())
  {
    newStream.pID = channel->Vpid();
#if APIVERSNUM >= 10701
    if (channel->Vtype() == 0x1B)
      newStream.type = stH264;
    else
#endif
      newStream.type = stMPEG2VIDEO;

    AddStreamInfo(newStream);
  }

  const int *APids = channel->Apids();
  for ( ; *APids; APids++)
  {
    int index = 0;
    if (!FindStream(*APids))
    {
      newStream.pID = *APids;
      newStream.type = stMPEG2AUDIO;
#if APIVERSNUM >= 10715
      if (channel->Atype(index) == 0x0F)
        newStream.type = stAACADTS;
      else if (channel->Atype(index) == 0x11)
        newStream.type = stAACLATM;
#endif
      newStream.SetLanguage(channel->Alang(index));
      AddStreamInfo(newStream);
    }
    index++;
  }

  const int *DPids = channel->Dpids();
  for ( ; *DPids; DPids++)
  {
    int index = 0;
    if (!FindStream(*DPids))
    {
      newStream.pID = *DPids;
      newStream.type = stAC3;
#if APIVERSNUM >= 10715
      if (channel->Dtype(index) == SI::EnhancedAC3DescriptorTag)
        newStream.type = stEAC3;
#endif
      newStream.SetLanguage(channel->Dlang(index));
      AddStreamInfo(newStream);
    }
    index++;
  }

  const int *SPids = channel->Spids();
  if (SPids)
  {
    int index = 0;
    for ( ; *SPids; SPids++)
    {
      if (!FindStream(*SPids))
      {
        newStream.pID = *SPids;
        newStream.type = stDVBSUB;
        newStream.SetLanguage(channel->Slang(index));
#if APIVERSNUM >= 10709
        newStream.subtitlingType = channel->SubtitlingType(index);
        newStream.compositionPageId = channel->CompositionPageId(index);
        newStream.ancillaryPageId = channel->AncillaryPageId(index);
#endif
        AddStreamInfo(newStream);
      }
      index++;
    }
  }

  if (channel->Tpid())
  {
    newStream.pID = channel->Tpid();
    newStream.type = stTELETEXT;
    AddStreamInfo(newStream);
  }
}

void cVNSIDemuxer::SetChannelPids(cChannel *channel, cPatPmtParser *patPmtParser)
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
  int Tpid = m_CurrentChannel.Tpid();
  channel->SetPids(Vpid, Ppid, VType,
                   Apids, Atypes, ALangs,
                   Dpids, Dtypes, DLangs,
                   Spids, SLangs,
                   Tpid);
}
