#pragma once
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
#include <string>

namespace MPTV
{

class CDateTime
{
public:
  CDateTime();
  CDateTime(const struct tm& time);
  CDateTime(const time_t& time);
  virtual ~CDateTime();

  int GetDay() const;
  int GetMonth() const;
  int GetYear() const;
  int GetHour() const;
  int GetMinute() const;
  int GetSecond() const;
  int GetDayOfWeek() const;
  int GetMinuteOfDay() const;

  time_t GetAsTime(void) const;

  /**
   * @brief Converts the stored datetime value to a string with the date representation for current locale
   * @param strDate     the date string (return value)
   */
  void GetAsLocalizedDate(std::string& strDate) const;

  /**
   * @brief Converts the stored datetime value to a string with the time representation for current locale
   * @param strDate     the date string (return value)
   */
  void GetAsLocalizedTime(std::string& strTime) const;
  
  /**
   * @brief Sets the date and time from a C# DateTime string
   * Assumes the usage of somedatetimeval.ToString("u") in C#
   */
  bool SetFromDateTime(const std::string& dateTime);

  /**
   * @brief Sets the date and time from a time_t value
   */
  void SetFromTime(const time_t& time);
  void SetFromTM(const struct tm& time);

  int operator -(const CDateTime& right) const;
  const CDateTime& operator =(const time_t& right);
  const CDateTime& operator =(const tm& right);
  bool operator ==(const time_t& right) const;
  const CDateTime& operator +=(const int seconds);

  static time_t Now();
private:
  void InitLocale(void);

  struct tm m_time;
};

} // namespace MPTV
