/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://www.xbmc.org
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <ctime>
#include <clocale>
#include <cstring>
#include <cstdio>
#include "DateTime.h"

namespace MPTV
{

void CDateTime::InitLocale(void)
{
  // Follow system default for date and time formatting
  // because we cannot access XBMC's locale settings from the PVR addon
  setlocale(LC_ALL, "");
}

CDateTime::CDateTime()
{
  InitLocale();
  memset(&m_time, 0, sizeof(m_time));
}

CDateTime::CDateTime(const time_t& dateTime)
{
  InitLocale();
  SetFromTime(dateTime);
}

CDateTime::CDateTime(const struct tm& dateTime)
{
  InitLocale();
  m_time = dateTime;
}

CDateTime::~CDateTime()
{}

int CDateTime::GetDay() const
{
  return m_time.tm_mday;
}

int CDateTime::GetMonth() const
{
  return (m_time.tm_mon + 1);
}

int CDateTime::GetYear() const
{
  return (m_time.tm_year + 1900);
}

int CDateTime::GetHour() const
{
  return (m_time.tm_hour);
}

int CDateTime::GetMinute() const
{
  return (m_time.tm_min);
}

int CDateTime::GetSecond() const
{
  return (m_time.tm_sec);
}

int CDateTime::GetDayOfWeek() const
{
  return (m_time.tm_wday);
}

time_t CDateTime::GetAsTime() const
{
  time_t retval;
  struct tm tm_time = m_time;
  retval = mktime (&tm_time);

  if(retval < 0)
    retval = 0;

  return retval;
}

bool CDateTime::SetFromDateTime(const std::string& dateTime)
{
  int year, month ,day;
  int hour, minute, second;
  int count;

  count = sscanf(dateTime.c_str(), "%4d-%2d-%2d %2d:%2d:%2d", &year, &month, &day, &hour, &minute, &second);

  if(count != 6)
    return false;

  m_time.tm_hour = hour;
  m_time.tm_min = minute;
  m_time.tm_sec = second;
  m_time.tm_year = year - 1900;
  m_time.tm_mon = month - 1;
  m_time.tm_mday = day;
  // Make the other fields empty:
  m_time.tm_isdst = -1;
  m_time.tm_wday = 0;
  m_time.tm_yday = 0;

  return true;
}

void CDateTime::SetFromTime(const time_t& time)
{
  m_time = *localtime( &time );
}

void CDateTime::SetFromTM(const struct tm& time)
{
  m_time = time;
}

void CDateTime::GetAsLocalizedDate(std::string & strDate) const
{
  const unsigned int bufSize = 64;
  char buffer[bufSize];
  
  strftime(buffer, bufSize, "%x", &m_time);
  strDate = buffer;
}

void CDateTime::GetAsLocalizedTime(std::string & strTime) const
{
  const unsigned int bufSize = 64;
  char buffer[bufSize];
  
  strftime(buffer, bufSize, "%H:%M", &m_time);
  strTime = buffer;
}

int CDateTime::operator -(const CDateTime& right) const
{
  time_t leftTime = GetAsTime();
  time_t rightTime = right.GetAsTime();

  return (int) (leftTime-rightTime);
}

const CDateTime& CDateTime::operator =(const time_t& right)
{
  SetFromTime(right);
  return *this;
}

const CDateTime& CDateTime::operator =(const tm& right)
{
  m_time = right;
  return *this;
}

bool CDateTime::operator ==(const time_t& right) const
{
  time_t left = GetAsTime();
  return (left == right);
}

const CDateTime& CDateTime::operator +=(const int seconds)
{
  time_t left = GetAsTime();
  left += seconds;
  SetFromTime(left);
  return *this;
}

time_t CDateTime::Now()
{
  time_t now;

  time(&now);

  return now;
}

} // namespace MPTV
