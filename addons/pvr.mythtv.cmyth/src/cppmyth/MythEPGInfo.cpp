/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "MythEPGInfo.h"
#include "MythPointer.h"

MythEPGInfo::MythEPGInfo()
  : m_epginfo_t()
{
}

MythEPGInfo::MythEPGInfo(cmyth_epginfo_t cmyth_epginfo)
  : m_epginfo_t(new MythPointer<cmyth_epginfo_t>())
{
  *m_epginfo_t = cmyth_epginfo;
}

bool MythEPGInfo::IsNull() const
{
  if (m_epginfo_t == NULL)
    return true;
  return *m_epginfo_t == NULL;
}

unsigned int MythEPGInfo::ChannelID()
{
  return cmyth_epginfo_chanid(*m_epginfo_t);
}

CStdString MythEPGInfo::ChannelName()
{
  char* buf = cmyth_epginfo_channame(*m_epginfo_t);
  CStdString retval(buf);
  ref_release(buf);
  return retval;
}

CStdString MythEPGInfo::Callsign()
{
  char* buf = cmyth_epginfo_callsign(*m_epginfo_t);
  CStdString retval(buf);
  ref_release(buf);
  return retval;
}

unsigned int MythEPGInfo::SourceID()
{
  return cmyth_epginfo_sourceid(*m_epginfo_t);
}

CStdString MythEPGInfo::Title()
{
  char* buf = cmyth_epginfo_title(*m_epginfo_t);
  CStdString retval(buf);
  ref_release(buf);
  return retval;
}

CStdString MythEPGInfo::Subtitle()
{
  char* buf = cmyth_epginfo_subtitle(*m_epginfo_t);
  CStdString retval(buf);
  ref_release(buf);
  return retval;
}

CStdString MythEPGInfo::Description()
{
  char* buf = cmyth_epginfo_description(*m_epginfo_t);
  CStdString retval(buf);
  ref_release(buf);
  return retval;
}

time_t MythEPGInfo::StartTime()
{
  return cmyth_epginfo_starttime(*m_epginfo_t);
}

time_t MythEPGInfo::EndTime()
{
  return cmyth_epginfo_endtime(*m_epginfo_t);
}

CStdString MythEPGInfo::ProgramID()
{
  char* buf = cmyth_epginfo_programid(*m_epginfo_t);
  CStdString retval(buf);
  ref_release(buf);
  return retval;
}

CStdString MythEPGInfo::SeriesID()
{
  char* buf = cmyth_epginfo_seriesid(*m_epginfo_t);
  CStdString retval(buf);
  ref_release(buf);
  return retval;
}

CStdString MythEPGInfo::Category()
{
  char* buf = cmyth_epginfo_category(*m_epginfo_t);
  CStdString retval(buf);
  ref_release(buf);
  return retval;
}

CStdString MythEPGInfo::CategoryType()
{
  char* buf = cmyth_epginfo_category_type(*m_epginfo_t);
  CStdString retval(buf);
  ref_release(buf);
  return retval;
}

unsigned int MythEPGInfo::ChannelNumberInt()
{
  return cmyth_epginfo_channum(*m_epginfo_t);
}
