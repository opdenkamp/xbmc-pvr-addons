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

#include "MythProgramInfo.h"
#include "MythTimestamp.h"
#include "MythPointer.h"

MythProgramInfo::MythProgramInfo()
  : m_proginfo_t()
{
}

MythProgramInfo::MythProgramInfo(cmyth_proginfo_t cmyth_proginfo)
  : m_proginfo_t(new MythPointer<cmyth_proginfo_t>())
{
  *m_proginfo_t = cmyth_proginfo;
}

bool MythProgramInfo::IsNull() const
{
  if (m_proginfo_t == NULL)
    return true;
  return *m_proginfo_t == NULL;
}

CStdString MythProgramInfo::StrUID()
{
  // Creates unique IDs from ChannelID, RecordID and StartTime like "100_200_2011-12-10T12:00:00"
  char buf[40] = "";
  sprintf(buf, "%d_%ld_", ChannelID(), RecordID());
  time_t starttime = StartTime();
  strftime(buf + strlen(buf), 20, "%Y-%m-%dT%H:%M:%S", localtime(&starttime));
  return CStdString(buf);
}

long long MythProgramInfo::UID()
{
  long long retval = RecStart();
  retval <<= 32;
  retval += ChannelID();
  if (retval > 0)
    retval = -retval;
  return retval;
}

CStdString MythProgramInfo::ProgramID()
{
  char* progId = cmyth_proginfo_programid(*m_proginfo_t);
  CStdString retval(progId);
  ref_release(progId);
  return retval;
}

CStdString MythProgramInfo::Title(bool subtitleEncoded)
{
  char* title = cmyth_proginfo_title(*m_proginfo_t);
  CStdString retval(title);
  ref_release(title);

  if (subtitleEncoded)
  {
    CStdString subtitle = Subtitle();
    if (!subtitle.empty())
      retval.Format("%s - %s", retval, subtitle);
  }
  return retval;
}

CStdString MythProgramInfo::Subtitle()
{
  char* subtitle = cmyth_proginfo_subtitle(*m_proginfo_t);
  CStdString retval(subtitle);
  ref_release(subtitle);
  return retval;
}

CStdString MythProgramInfo::Path()
{
  char* path = cmyth_proginfo_pathname(*m_proginfo_t);
  CStdString retval(path);
  ref_release(path);
  return retval;
}

CStdString MythProgramInfo::Description()
{
  char* desc = cmyth_proginfo_description(*m_proginfo_t);
  CStdString retval(desc);
  ref_release(desc);
  return retval;
}

int MythProgramInfo::Duration()
{
  MythTimestamp end = cmyth_proginfo_rec_end(*m_proginfo_t);
  MythTimestamp start = cmyth_proginfo_rec_start(*m_proginfo_t);
  return end.UnixTime() - start.UnixTime();
  //return cmyth_proginfo_length_sec(*m_proginfo_t);
}

CStdString MythProgramInfo::Category()
{
  char* cat = cmyth_proginfo_category(*m_proginfo_t);
  CStdString retval(cat);
  ref_release(cat);
  return retval;
}

time_t MythProgramInfo::StartTime()
{
  MythTimestamp time = cmyth_proginfo_start(*m_proginfo_t);
  time_t retval(time.UnixTime());
  return retval;
}

time_t MythProgramInfo::EndTime()
{
  MythTimestamp time = cmyth_proginfo_end(*m_proginfo_t);
  time_t retval(time.UnixTime());
  return retval;
}

bool MythProgramInfo::IsWatched()
{
  unsigned long recording_flags = cmyth_proginfo_flags(*m_proginfo_t);
  return (recording_flags & 0x00000200) != 0; // FL_WATCHED
}

bool MythProgramInfo::IsDeletePending()
{
  unsigned long recording_flags = cmyth_proginfo_flags(*m_proginfo_t);
  return (recording_flags & 0x00000080) != 0; // FL_DELETEPENDING
}

int MythProgramInfo::ChannelID()
{
  return cmyth_proginfo_chan_id(*m_proginfo_t);
}

CStdString MythProgramInfo::ChannelName()
{
  char* chan = cmyth_proginfo_channame(*m_proginfo_t);
  CStdString retval(chan);
  ref_release(chan);
  return retval;
}

MythProgramInfo::RecordStatus MythProgramInfo::Status()
{
  return cmyth_proginfo_rec_status(*m_proginfo_t);
}

CStdString MythProgramInfo::RecordingGroup()
{
  char* recgroup = cmyth_proginfo_recgroup(*m_proginfo_t);
  CStdString retval(recgroup);
  ref_release(recgroup);
  return retval;
}

unsigned long MythProgramInfo::RecordID()
{
  return cmyth_proginfo_recordid(*m_proginfo_t);
}

time_t MythProgramInfo::RecStart()
{
  MythTimestamp time = cmyth_proginfo_rec_start(*m_proginfo_t);
  time_t retval(time.UnixTime());
  return retval;
}

int MythProgramInfo::Priority()
{
  return cmyth_proginfo_priority(*m_proginfo_t); // Might want to use recpriority2 instead
}
