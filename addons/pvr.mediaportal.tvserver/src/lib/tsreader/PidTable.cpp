/* 
 *  Copyright (C) 2006-2009 Team MediaPortal
 *  http://www.team-mediaportal.com
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "PidTable.h"
#include "client.h" //XBMC->Log

using namespace ADDON;

CPidTable::CPidTable(const CPidTable& pids)
{
  Copy(pids);
}


CPidTable::CPidTable(void)
{
  Reset();
}


CPidTable::~CPidTable(void)
{
}


bool CPidTable::operator ==(const CPidTable& other) const
{
  // Not all members are compared, this is how DeMultiplexer class has 
  // been comparing "PMTs" to detect channel changes.
  if(subtitlePids != other.subtitlePids
    || audioPids != other.audioPids 
    || videoPids != other.videoPids
    || PcrPid != other.PcrPid
    || PmtPid != other.PmtPid)
  {
    return false;
  }
  else
  {
    return true;
  }
}


void CPidTable::Reset()
{
  //XBMC->Log(LOG_DEBUG, "Pid table reset");
  PcrPid=0;
  PmtPid=0;
  ServiceId=-1;

  videoPids.clear();
  audioPids.clear();
  subtitlePids.clear();

  TeletextPid=0;
  // no reason to reset TeletextSubLang
}


CPidTable& CPidTable::operator = (const CPidTable &pids)
{
  if (&pids == this)
  {
    return *this;
  }
  Copy(pids);
  return *this;
}


void CPidTable::Copy(const CPidTable &pids)
{
  //XBMC->Log(LOG_DEBUG, "Pid table copy");
  ServiceId = pids.ServiceId;

  PcrPid = pids.PcrPid;
  PmtPid = pids.PmtPid;

  videoPids = pids.videoPids;
  audioPids = pids.audioPids;
  subtitlePids = pids.subtitlePids;

  TeletextPid = pids.TeletextPid;
  //TeletextInfo=pids.TeletextInfo;
}


bool CPidTable::HasTeletextPageInfo(int page)
{ //MG: todo
  //std::vector<TeletextServiceInfo>::iterator vit = TeletextInfo.begin();
  //while(vit != TeletextInfo.end())
  //{ // is the page already registrered
  //  TeletextServiceInfo& info = *vit;
  //  if(info.page == page)
  //  {
  //    return true;
  //    break;
  //  }
  //  else vit++;
  //}
  return false;
}

void CPidTable::LogPIDs()
{
  XBMC->Log(LOG_DEBUG, " pcr      pid: %4x ",PcrPid);
  XBMC->Log(LOG_DEBUG, " pmt      pid: %4x ",PmtPid);

  // Log all video streams (Blu-ray can have multiple video streams)
  for (unsigned int i(0); i < videoPids.size(); i++)
  {
    XBMC->Log(LOG_DEBUG, " video    pid: %4x type: %s",
      videoPids[i].Pid, 
      StreamFormatAsString(videoPids[i].VideoServiceType));
  }

  // Log all audio streams
  for (unsigned int i(0); i < audioPids.size(); i++)
  {
    XBMC->Log(LOG_DEBUG, " audio    pid: %4x language: %3s type: %s",
    audioPids[i].Pid, 
    audioPids[i].Lang,
    StreamFormatAsString(audioPids[i].AudioServiceType));
  }

  // Log all subtitle streams
  for (unsigned int i(0); i < subtitlePids.size(); i++)
  {
    XBMC->Log(LOG_DEBUG, " Subtitle pid: %4x language: %3s type: %s",
      subtitlePids[i].Pid, 
      subtitlePids[i].Lang,
      StreamFormatAsString(subtitlePids[i].SubtitleServiceType));  
  }
}


const char* CPidTable::StreamFormatAsString(int streamType)
{
  switch (streamType)
  {
    case 0x01:
      return "MPEG1";
    case 0x02:
      return "MPEG2";
    case 0x03:
      return "MPEG1 - audio";
    case 0x04:
      return "MPEG2 - audio";
    case 0x05:
      return "DVB subtitle 1";
    case 0x06:
      return "DVB subtitle 2";
    case 0x10:
      return "MPEG4";
    case 0x1B:
      return "H264";
    case 0xEA:
      return "VC1";
    case 0x80:
      return "LPCM";
    case 0x81:
      return "AC3";
    case 0x82:
      return "DTS";
    case 0x83:
      return "MLP";
    case 0x84:
      return "DD+";
    case 0x85:
      return "DTS-HD";
    case 0x86:
      return "DTS-HD Master Audio";
    case 0x0f:
      return "AAC";
    case 0x11:
      return "LATM AAC";
    case 0xA1:
      return "DD+";
    case 0xA2:
      return "DTS-HD";
    case 0x90:
      return "PGS";
    case 0x91:
      return "IG";
    case 0x92:
      return "Text";
    default:
      return "Unknown";
  }
}

