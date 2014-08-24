/*
 *      Copyright (C) 2005-2014 Team XBMC
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

MythEPGInfo::MythEPGInfo()
  : m_epginfo()
{
}

MythEPGInfo::MythEPGInfo(Myth::ProgramPtr epginfo)
  : m_epginfo()
{
  m_epginfo.swap(epginfo);
}

bool MythEPGInfo::IsNull() const
{
  if (!m_epginfo)
    return true;
  return m_epginfo.get() == NULL;
}

Myth::ProgramPtr MythEPGInfo::GetPtr() const
{
  return m_epginfo;
}

uint32_t MythEPGInfo::ChannelID() const
{
  return (m_epginfo ? m_epginfo->channel.chanId : 0);
}

std::string MythEPGInfo::ChannelName() const
{
  return (m_epginfo ? m_epginfo->channel.channelName : "" );
}

std::string MythEPGInfo::Callsign() const
{
  return (m_epginfo ? m_epginfo->channel.callSign : "");
}

uint32_t MythEPGInfo::SourceID() const
{
  return (m_epginfo ? m_epginfo->channel.sourceId : 0);
}

std::string MythEPGInfo::Title() const
{
  return (m_epginfo ? m_epginfo->title : "");
}

std::string MythEPGInfo::Subtitle() const
{
  return (m_epginfo ? m_epginfo->subTitle : "");
}

std::string MythEPGInfo::Description() const
{
  return (m_epginfo ? m_epginfo->description : "");
}

time_t MythEPGInfo::StartTime() const
{
  return (m_epginfo ? m_epginfo->startTime : (time_t)(-1));
}

time_t MythEPGInfo::EndTime() const
{
  return (m_epginfo ? m_epginfo->endTime : (time_t)(-1));
}

std::string MythEPGInfo::ProgramID() const
{
  return (m_epginfo ? m_epginfo->programId : "");
}

std::string MythEPGInfo::SeriesID() const
{
  return (m_epginfo ? m_epginfo->seriesId : "");
}

std::string MythEPGInfo::Category() const
{
  return (m_epginfo ? m_epginfo->category : "");
}

std::string MythEPGInfo::CategoryType() const
{
  return (m_epginfo ? m_epginfo->catType : "");
}

std::string MythEPGInfo::ChannelNumber() const
{
  return (m_epginfo ? m_epginfo->channel.chanNum : "");
}
