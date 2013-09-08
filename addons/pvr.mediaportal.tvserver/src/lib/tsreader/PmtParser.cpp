/* 
*  Copyright (C) 2006 Team MediaPortal
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

#include "os-dependent.h"
#include "client.h" // XBMC->Log
#include "PmtParser.h"
#include "ChannelInfo.h"
#include <cassert>

using namespace ADDON;

CPmtParser::CPmtParser()
{
  m_pmtCallback = NULL;
  m_isFound = false;
}

CPmtParser::~CPmtParser(void)
{
}

void CPmtParser::SetPmtCallBack(IPmtCallBack* callback)
{
  m_pmtCallback = callback;
}

bool CPmtParser::IsReady()
{
  return m_isFound;
}

void CPmtParser::OnNewSection(CSection& section)
{
  if (section.table_id != 2)
  {
    return;
  }

  try
  {
    bool lpcm_audio_found = false;
    int program_number = section.table_id_extension;
    int pcr_pid = ((section.Data[8] & 0x1F) << 8) + section.Data[9];
    int program_info_length = ((section.Data[10] & 0xF) << 8) + section.Data[11];
    int len2 = program_info_length;
    int pointer = 12;
    int len1 = section.section_length -(9 + program_info_length + 4);
    int x;

    if (!m_isFound)
    {
      XBMC->Log(LOG_DEBUG, "got pmt:%x service id:%x", GetPid(), program_number);
      m_isFound = true;
      if (m_pmtCallback != NULL)
      {
        m_pmtCallback->OnPmtReceived(GetPid());
      }
    }

    // loop 1
    while (len2 > 0)
    {
      //int indicator=section.Data[pointer];
      int descriptorLen=section.Data[pointer+1];
      len2 -= (descriptorLen+2);
      pointer += (descriptorLen+2);
    }
    // loop 2
    int stream_type = 0;
    short elementary_PID = 0;
    short ES_info_length = 0;
    vector<TempPid> tempPids;

    m_pidInfo.Reset();
    m_pidInfo.PmtPid = GetPid();
    m_pidInfo.ServiceId = program_number;

    while (len1 > 0)
    {
      //if (start+pointer+4>=sectionLen+9) return ;
      stream_type = section.Data[pointer];
      elementary_PID = ((section.Data[pointer+1] & 0x1F) << 8) + section.Data[pointer+2];
      ES_info_length = ((section.Data[pointer+3] & 0xF) << 8) + section.Data[pointer+4];
      XBMC->Log(LOG_DEBUG, "pmt: pid:%x type:%x",elementary_PID, stream_type);

      if (stream_type == SERVICE_TYPE_VIDEO_MPEG1
        || stream_type == SERVICE_TYPE_VIDEO_MPEG2
        || stream_type == SERVICE_TYPE_VIDEO_MPEG4
        || stream_type == SERVICE_TYPE_VIDEO_H264 )
      {
        VideoPid pid;
        pid.Pid = elementary_PID;
        pid.VideoServiceType = stream_type;
        m_pidInfo.videoPids.push_back(pid);
      }
      if (stream_type == SERVICE_TYPE_AUDIO_MPEG1 ||
        stream_type == SERVICE_TYPE_AUDIO_MPEG2 ||
        stream_type == SERVICE_TYPE_AUDIO_AC3 ||
        stream_type == SERVICE_TYPE_AUDIO_AAC ||
        stream_type == SERVICE_TYPE_AUDIO_LATM_AAC ||
        stream_type == SERVICE_TYPE_AUDIO_DD_PLUS )
      {
        AudioPid pid;
        pid.Pid = elementary_PID;
        pid.AudioServiceType = (short) stream_type;
        m_pidInfo.audioPids.push_back(pid);
      }
      m_pidInfo.PcrPid = pcr_pid;

      pointer += 5;
      len1 -= 5;
      len2 = ES_info_length;

      while (len2 > 0)
      {
        if (pointer + 1 >= section.section_length)
        {
          XBMC->Log(LOG_DEBUG, "pmt parser check1");
          return;
        }

        int indicator = section.Data[pointer];
        x = section.Data[pointer + 1] + 2;

        if(indicator == DESCRIPTOR_DVB_AC3 || indicator == DESCRIPTOR_DVB_E_AC3)
        {
          AudioPid pid;
          pid.Pid = elementary_PID;
          pid.AudioServiceType = (indicator == DESCRIPTOR_DVB_AC3) ? SERVICE_TYPE_AUDIO_AC3 : SERVICE_TYPE_AUDIO_DD_PLUS;

          for (unsigned int i(0); i < tempPids.size(); i++)
          {
            if (tempPids[i].Pid == elementary_PID)
            {
              pid.Lang[0] = tempPids[i].Lang[0];
              pid.Lang[1] = tempPids[i].Lang[1];
              pid.Lang[2] = tempPids[i].Lang[2];
              pid.Lang[3] = tempPids[i].Lang[3]; // should be null if no extra data is available
              pid.Lang[4] = tempPids[i].Lang[4];
              pid.Lang[5] = tempPids[i].Lang[5];
              tempPids.pop_back();
              break;
            }
          }

          m_pidInfo.audioPids.push_back(pid);
        }

        // audio and subtitle languages
        if (indicator == DESCRIPTOR_MPEG_ISO639_Lang)
        {
          if (pointer + 4 >= section.section_length)
          {
            XBMC->Log(LOG_DEBUG, "pmt parser check2");
            return;
          }

          bool pidFound(false);

          // Find corresponding audio stream by PID, if not found
          // the stream type should be unknown to us
          for (unsigned int i(0); i < m_pidInfo.audioPids.size(); i++)
          {
            if (m_pidInfo.audioPids[i].Pid == elementary_PID)
            {
              int descriptorLen = section.Data[pointer+1];

              m_pidInfo.audioPids[i].Lang[0] = section.Data[pointer+2];
              m_pidInfo.audioPids[i].Lang[1] = section.Data[pointer+3];
              m_pidInfo.audioPids[i].Lang[2] = section.Data[pointer+4];
              m_pidInfo.audioPids[i].Lang[3] = 0;

              // Get the additional language descriptor data (NORSWE etc.)
              if (descriptorLen == 8)
              {
                m_pidInfo.audioPids[i].Lang[3] = section.Data[pointer+6];
                m_pidInfo.audioPids[i].Lang[4] = section.Data[pointer+7];
                m_pidInfo.audioPids[i].Lang[5] = section.Data[pointer+8];
                m_pidInfo.audioPids[i].Lang[6] = 0;
              }

              pidFound = true;
            }
            // Find corresponding subtitle stream by PID, if not found
            // the stream type is be unknown to us
            for (unsigned int j(0); j < m_pidInfo.subtitlePids.size(); j++)
            {
              if(m_pidInfo.subtitlePids[j].Pid == elementary_PID)
              {
                m_pidInfo.subtitlePids[j].Lang[0] = section.Data[pointer+2];
                m_pidInfo.subtitlePids[j].Lang[1] = section.Data[pointer+3];
                m_pidInfo.subtitlePids[j].Lang[2] = section.Data[pointer+4];
                m_pidInfo.subtitlePids[j].Lang[3] = 0;
                pidFound = true;
              }
            }

            if (!pidFound)
            {
              int descriptorLen = section.Data[pointer+1];
              TempPid pid;
              pid.Pid = elementary_PID;
              pid.Lang[0] = section.Data[pointer+2];
              pid.Lang[1] = section.Data[pointer+3];
              pid.Lang[2] = section.Data[pointer+4];
              // Get the additional language descriptor data (NORSWE etc.)
              if( descriptorLen == 8 )
              {
                pid.Lang[3] = section.Data[pointer+6];
                pid.Lang[4] = section.Data[pointer+7];
                pid.Lang[5] = section.Data[pointer+8];
              }
              else
              {
                pid.Lang[3] = 0;
                pid.Lang[4] = 0;
                pid.Lang[5] = 0;
              }

              tempPids.push_back(pid);
            }
          }
        }
        if (indicator == DESCRIPTOR_VBI_TELETEXT)
        {
          XBMC->Log(LOG_DEBUG, "VBI teletext descriptor");
        }
        if (indicator == DESCRIPTOR_DVB_TELETEXT /*&& m_pidInfo.TeletextPid==0*/)
        {
          m_pidInfo.TeletextPid = elementary_PID;
          assert(section.Data[pointer+0] == DESCRIPTOR_DVB_TELETEXT);
          int descriptorLen = section.Data[pointer+1];

          int varBytes = 5; // 4 additional fields for a total of 32 bits (see 6.2.40)

          assert(descriptorLen % varBytes == 0); // there shouldnt be any left over bytes :)

          int N = descriptorLen / varBytes; 

          //BYTE b = 0x02 << 3;

          XBMC->Log(LOG_DEBUG, "Descriptor length %i, N= %i", descriptorLen, N);
          for (int j = 0; j < N; j++)
          {
            //BYTE ISO_639_language_code[3];
            //ISO_639_language_code[0] = section.Data[pointer + varBytes*j + 2];
            //ISO_639_language_code[1] = section.Data[pointer + varBytes*j + 3];
            //ISO_639_language_code[2] = section.Data[pointer + varBytes*j + 4];

            byte b3 = section.Data[pointer + varBytes*j + 5];
            byte teletext_type = (b3 & 0xF8) >> 3; // 5 first(msb) bits

            assert(teletext_type <= 0x05); // 0x06 and upwards reserved for future use and shouldnt appear
            //for (int i = 0; i < 8; i++){
            //  if( ((b3 << i) & 128) != 0) LogDebug("1");
            //  else LogDebug("0");
            //}

            int teletext_magazine_number = (b3 & 0x07); // last(lsb) 3 bits

            int teletext_page_number = (section.Data[pointer + varBytes*j + 6]);

            int real_page_tens  = (teletext_page_number & 0xF0) >> 4;
            int real_page_units = teletext_page_number & 0x0F;

            int real_page = teletext_magazine_number*100 + real_page_tens*10 + real_page_units;

            //XBMC->Log(LOG_DEBUG, "Mag: %i, tens %i, units %i, total ?= %i", teletext_magazine_number,real_page_tens,real_page_units,real_page);

            //if its a teletext subtitle service (standard / hard of hearing respectively)
            if (teletext_type == 0x02 || teletext_type == 0x05)
            { 
              if (!m_pidInfo.HasTeletextPageInfo(real_page))
              {
                XBMC->Log(LOG_DEBUG, "TODO: Teletext subtitles in PMT: PID %i, mag %i, page %i", elementary_PID, teletext_magazine_number, real_page);
                //XBMC->Log(LOG_DEBUG, "Teletext subtitles in PMT: PID %i, mag %i, page %i, prevPage %i", elementary_PID, teletext_magazine_number, real_page, m_pidInfo.TeletextSubPage);
                //TeletextServiceInfo info;
                //info.page = real_page;
                //info.type = teletext_type;
                //info.lang[0] = ISO_639_language_code[0];
                //info.lang[1] = ISO_639_language_code[1];
                //info.lang[2] = ISO_639_language_code[2];
                //m_pidInfo.TeletextInfo.push_back(info);
              }
            }
            else
            {
              XBMC->Log(LOG_DEBUG, "Teletext SI: Page %i Type %X",real_page,teletext_type);
            }
          }
        }
        if (indicator == DESCRIPTOR_DVB_SUBTITLING )
        {
          if (stream_type == SERVICE_TYPE_DVB_SUBTITLES2)
          {
            SubtitlePid pid;
            pid.Pid = elementary_PID;
            pid.Lang[0] = section.Data[pointer+2];
            pid.Lang[1] = section.Data[pointer+3];
            pid.Lang[2] = section.Data[pointer+4];
            pid.Lang[3] = 0;
            pid.SubtitleServiceType = SERVICE_TYPE_DVB_SUBTITLES2;
            m_pidInfo.subtitlePids.push_back(pid);
          }
        }
        if (indicator == DESCRIPTOR_REGISTRATION)
        {
          if (section.Data[pointer+2] == 'H' &&
              section.Data[pointer+3] == 'D' &&
              section.Data[pointer+4] == 'M' &&
              section.Data[pointer+5] == 'V' &&
              stream_type == SERVICE_TYPE_DCII_OR_LPCM)
          {
            AudioPid pid;
            pid.Pid = elementary_PID;
            pid.AudioServiceType = (short) stream_type;
            m_pidInfo.audioPids.push_back(pid);
            lpcm_audio_found = true;
          }
        }
        len2 -= x;
        len1 -= x;
        pointer += x;
      }
      if (stream_type == SERVICE_TYPE_DCII_OR_LPCM && !lpcm_audio_found)
      {
        VideoPid pid;
        pid.Pid = elementary_PID;
        pid.VideoServiceType = SERVICE_TYPE_VIDEO_MPEG2;
        m_pidInfo.videoPids.push_back(pid);
      }
    }
    if (m_pmtCallback != NULL)
    {
      XBMC->Log(LOG_DEBUG, "DecodePMT pid:0x%x pcrpid:0x%x sid:%x", m_pidInfo.PmtPid, m_pidInfo.PcrPid, m_pidInfo.ServiceId);
      m_pmtCallback->OnPidsReceived(m_pidInfo);
    }
  } 
  catch (...) 
  { 
    XBMC->Log(LOG_DEBUG, "Exception in PmtParser");
  }
}


CPidTable& CPmtParser::GetPidInfo()
{
  return m_pidInfo;
}
