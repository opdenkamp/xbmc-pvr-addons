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

#include "MythTimestamp.h"
#include "MythPointer.h"

MythTimestamp::MythTimestamp()
  : m_timestamp_t()
{
}

MythTimestamp::MythTimestamp(cmyth_timestamp_t cmyth_timestamp)
  : m_timestamp_t(new MythPointer<cmyth_timestamp_t>())
{
  *m_timestamp_t = cmyth_timestamp;
}

MythTimestamp::MythTimestamp(CStdString time, bool datetime)
 : m_timestamp_t(new MythPointer<cmyth_timestamp_t>())
{
  // datetime ? DatetimeFromString(time.Buffer()) : cmyth_timestamp_from_string(time.Buffer())
  (void)datetime;

  *m_timestamp_t = (cmyth_timestamp_from_string(time.Buffer()));
}

MythTimestamp::MythTimestamp(time_t time)
  : m_timestamp_t(new MythPointer<cmyth_timestamp_t>())
{
  *m_timestamp_t = cmyth_timestamp_from_unixtime(time);
}

bool MythTimestamp::operator==(const MythTimestamp &other)
{
  return cmyth_timestamp_compare(*m_timestamp_t, *other.m_timestamp_t) == 0;
}

bool MythTimestamp::operator>(const MythTimestamp &other)
{
  return cmyth_timestamp_compare(*m_timestamp_t, *other.m_timestamp_t) == 1;
}

bool MythTimestamp::operator<(const MythTimestamp &other)
{
  return cmyth_timestamp_compare(*m_timestamp_t, *other.m_timestamp_t) == -1;
}

bool MythTimestamp::IsNull() const
{
  if (m_timestamp_t == NULL)
    return true;
  return *m_timestamp_t == NULL;
}

time_t MythTimestamp::UnixTime()
{
  return cmyth_timestamp_to_unixtime(*m_timestamp_t);
}

CStdString MythTimestamp::String()
{
  char time[25];
  bool succeded = cmyth_timestamp_to_string(time, *m_timestamp_t) == 0;
  return succeded ? CStdString(time) : CStdString("");
}

CStdString MythTimestamp::IsoString()
{
  char time[25];
  bool succeded = cmyth_timestamp_to_isostring(time, *m_timestamp_t) == 0;
  return succeded ? CStdString(time) : CStdString("");
}

CStdString MythTimestamp::DisplayString(bool use12hClock)
{
  char time[25];
  bool succeded=cmyth_timestamp_to_display_string(time, *m_timestamp_t, use12hClock) == 0;
  return succeded ? CStdString(time) : CStdString("");
}
