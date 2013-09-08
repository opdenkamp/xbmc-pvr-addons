/*
 *      Copyright (C) 2005-2011 Team XBMC
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

#include <vector>
#include <stdio.h>
#include <stdlib.h>

using namespace std;

#include "platform/os.h" //needed for snprintf
#include "client.h"
#include "timers.h"
#include "utils.h"

using namespace ADDON;

cTimer::cTimer()
{
  m_index              = -1;
  m_active             = true;
  m_channel            = 0;
  m_schedtype          = TvDatabase::Once;
  m_starttime          = 0;
  m_endtime            = 0;
  m_priority           = 0;
  m_keepmethod         = TvDatabase::UntilSpaceNeeded;
  m_keepdate           = cUndefinedDate;
  m_prerecordinterval  = -1; // Use MediaPortal setting instead
  m_postrecordinterval = -1; // Use MediaPortal setting instead
  m_canceled           = cUndefinedDate;
  m_series             = false;
  m_done               = false;
  m_ismanual           = false;
  m_isrecording        = false;
  m_progid             = -1;
}


cTimer::cTimer(const PVR_TIMER& timerinfo)
{

  m_index = timerinfo.iClientIndex;
  m_progid = timerinfo.iEpgUid;

  if(strlen(timerinfo.strDirectory) > 0)
  {
    // Workaround: retrieve the schedule id from the directory name if set
    int schedule_id = 0;
    unsigned int program_id = 0;

    if (sscanf(timerinfo.strDirectory, "%9d/%9u", &schedule_id, &program_id) == 2)
    {
      if (program_id == timerinfo.iClientIndex)
      {
        m_index  = schedule_id;
        m_progid = program_id;
      }
    }
  }

  m_done = (timerinfo.state == PVR_TIMER_STATE_COMPLETED);
  m_active = (timerinfo.state == PVR_TIMER_STATE_SCHEDULED || timerinfo.state == PVR_TIMER_STATE_RECORDING);

  if (!m_active)
  {
    m_canceled = Now();
  }
  else
  {
    // Don't know when it was cancelled, so assume that it was canceled now...
    // backend (TVServerXBMC) will only update the canceled date time when
    // this schedule was just canceled
    m_canceled = cUndefinedDate;
  }
 
  m_title = timerinfo.strTitle;
  m_directory = timerinfo.strDirectory;
  m_channel = timerinfo.iClientChannelUid;

  if (timerinfo.startTime <= 0)
  {
    // Instant timer has starttime = 0, so set current time as starttime.
    m_starttime = Now();
    m_ismanual = true;
  }
  else
  {
    m_starttime = timerinfo.startTime;
    m_ismanual = false;
  }

  m_endtime = timerinfo.endTime;
  //m_firstday = timerinfo.firstday;
  m_isrecording = (timerinfo.state == PVR_TIMER_STATE_RECORDING);
  m_priority = XBMC2MepoPriority(timerinfo.iPriority);

  SetKeepMethod(timerinfo.iLifetime);

  if(timerinfo.bIsRepeating)
  {
    m_schedtype = RepeatFlags2SchedRecType(timerinfo.iWeekdays);
    m_series = true;
  }
  else
  {
    m_schedtype = TvDatabase::Once;
    m_series = false;
  }

  m_prerecordinterval = timerinfo.iMarginStart;
  m_postrecordinterval = timerinfo.iMarginEnd;
}


cTimer::~cTimer()
{
}

/**
 * @brief Fills the PVR_TIMER struct with information from this timer
 * @param tag A reference to the PVR_TIMER struct
 */
void cTimer::GetPVRtimerinfo(PVR_TIMER &tag)
{
  memset(&tag, 0, sizeof(tag));

  if (m_progid != -1)
  {
    // Use the EPG (program) id as unique id to see all scheduled programs in the EPG and timer list
    // Next program (In Home) is always the right one. Mostly seen with programs that are shown daily.
    tag.iClientIndex    = m_progid;
    tag.iEpgUid         = m_index;

    // Workaround: store the schedule and program id as directory name
    // This is needed by the PVR addon to map an XBMC timer back to the MediaPortal schedule
    // because we can't use the iClientIndex as schedule id anymore...
    snprintf(tag.strDirectory, sizeof(tag.strDirectory)-1, "%d/%d", m_index, m_progid);
  }
  else
  {
    tag.iClientIndex   = m_index; //Support older TVServer and Manual Schedule having a program name that does not have a match in MP EPG.
    tag.iEpgUid        = 0;
    PVR_STRCLR(tag.strDirectory);
  }

  if (IsRecording())
    tag.state           = PVR_TIMER_STATE_RECORDING;
  else if (m_active)
    tag.state           = PVR_TIMER_STATE_SCHEDULED;
  else
    tag.state           = PVR_TIMER_STATE_CANCELLED;
  tag.iClientChannelUid = m_channel;
  PVR_STRCPY(tag.strTitle, m_title.c_str());
  tag.startTime         = m_starttime ;
  tag.endTime           = m_endtime ;
  // From the VDR manual
  // firstday: The date of the first day when this timer shall start recording
  //           (only available for repeating timers).
  if(Repeat())
  {
    tag.firstDay        = m_starttime;
  } else {
    tag.firstDay        = 0;
  }
  tag.iPriority         = Priority();
  tag.iLifetime         = GetLifetime();
  tag.bIsRepeating      = Repeat();
  tag.iWeekdays         = RepeatFlags();
  tag.iMarginStart      = m_prerecordinterval;
  tag.iMarginEnd        = m_postrecordinterval;
  tag.iGenreType        = 0;
  tag.iGenreSubType     = 0;
}

time_t cTimer::StartTime(void) const
{
  return m_starttime;
}

time_t cTimer::EndTime(void) const
{
  return m_endtime;
}

bool cTimer::ParseLine(const char *s)
{
  vector<string> schedulefields;
  string data = s;
  uri::decode(data);

  Tokenize(data, schedulefields, "|");

  if (schedulefields.size() >= 10)
  {
    // field 0 = index
    // field 1 = start date + time
    // field 2 = end   date + time
    // field 3 = channel nr
    // field 4 = channel name
    // field 5 = program name
    // field 6 = schedule recording type
    // field 7 = priority
    // field 8 = isdone (finished)
    // field 9 = ismanual
    // field 10 = directory
    // field 11 = keepmethod (0=until space needed, 1=until watched, 2=until keepdate, 3=forever) (TVServerXBMC build >= 100)
    // field 12 = keepdate (2000-01-01 00:00:00 = infinite)  (TVServerXBMC build >= 100)
    // field 13 = preRecordInterval  (TVServerXBMC build >= 100)
    // field 14 = postRecordInterval (TVServerXBMC build >= 100)
    // field 15 = canceled (TVServerXBMC build >= 100)
    // field 16 = series (True/False) (TVServerXBMC build >= 100)
    // field 17 = isrecording (True/False)
    // field 18 = program id (EPG)
    m_index = atoi(schedulefields[0].c_str());
    m_starttime = DateTimeToTimeT(schedulefields[1]);

    if( m_starttime < 0)
      return false;

    m_endtime = DateTimeToTimeT(schedulefields[2]);

    if( m_endtime < 0)
      return false;

    m_channel = atoi(schedulefields[3].c_str());
    m_title = schedulefields[5];

    m_schedtype = (TvDatabase::ScheduleRecordingType) atoi(schedulefields[6].c_str());

    m_priority = atoi(schedulefields[7].c_str());
    m_done = stringtobool(schedulefields[8]);
    m_ismanual = stringtobool(schedulefields[9]);
    m_directory = schedulefields[10];
    
    if(schedulefields.size() >= 18)
    {
      //TVServerXBMC build >= 100
      m_keepmethod = (TvDatabase::KeepMethodType) atoi(schedulefields[11].c_str());
      m_keepdate = DateTimeToTimeT(schedulefields[12]);

      if( m_keepdate < 0)
        return false;

      m_prerecordinterval = atoi(schedulefields[13].c_str());
      m_postrecordinterval = atoi(schedulefields[14].c_str());

      // The DateTime value 2000-01-01 00:00:00 means: active in MediaPortal
      if(schedulefields[15].compare("2000-01-01 00:00:00Z")==0)
      {
        m_canceled = cUndefinedDate;
        m_active = true;
      }
      else
      {
        m_canceled = DateTimeToTimeT(schedulefields[15]);
        m_active = false;
      }

      m_series = stringtobool(schedulefields[16]);
      m_isrecording = stringtobool(schedulefields[17]);

    }
    else
    {
      m_keepmethod = TvDatabase::UntilSpaceNeeded;
      m_keepdate = cUndefinedDate;
      m_prerecordinterval = -1;
      m_postrecordinterval = -1;
      m_canceled = cUndefinedDate;
      m_active = true;
      m_series = false;
      m_isrecording = false;
    }

    if(schedulefields.size() >= 19)
      m_progid = atoi(schedulefields[18].c_str());
    else
      m_progid = -1;

    return true;
  }
  return false;
}

int cTimer::SchedRecType2RepeatFlags(TvDatabase::ScheduleRecordingType schedtype)
{
  // margro: the meaning of the XBMC-PVR Weekdays field is undocumented.
  // Assuming that VDR is the source for this field:
  //   This field contains a bitmask that correcsponds to the days of the week at which this timer runs
  //   It is based on the VDR Day field format "MTWTF--"
  //   The format is a 1 bit for every enabled day and a 0 bit for a disabled day
  //   Thus: WeekDays = "0000 0001" = "M------" (monday only)
  //                    "0110 0000" = "-----SS" (saturday and sunday)
  //                    "0001 1111" = "MTWTF--" (all weekdays)

  int weekdays = 0;

  switch (schedtype)
  {
    case TvDatabase::Once:
      weekdays = 0;
      break;
    case TvDatabase::Daily:
      weekdays = 127; // 0111 1111
      break;
    case TvDatabase::Weekly:
      {
        // Not sure what to do with this MediaPortal option...
        // Assumption: record once a week, on the same day and time
        // => determine weekday and set the corresponding bit
        struct tm timeinfo;

        timeinfo = *localtime( &m_starttime );

        int weekday = timeinfo.tm_wday; //days since Sunday [0-6]
        // bit 0 = monday, need to convert weekday value to bitnumber:
        if (weekday == 0)
          weekday = 6; // sunday 0100 0000
        else
          weekday--;

        weekdays = 1 << weekday;
        break;
      }
    case TvDatabase::EveryTimeOnThisChannel:
      // Don't know what to do with this MediaPortal option?
      break;
    case TvDatabase::EveryTimeOnEveryChannel:
      // Don't know what to do with this MediaPortal option?
      break;
    case TvDatabase::Weekends:
      weekdays = 96; // 0110 0000
      break;
    case TvDatabase::WorkingDays:
      weekdays = 31; // 0001 1111
      break;
    default:
      weekdays=0;
  }

  return weekdays;
}

TvDatabase::ScheduleRecordingType cTimer::RepeatFlags2SchedRecType(int repeatflags)
{
  // margro: the meaning of the XBMC-PVR Weekdays field is undocumented.
  // Assuming that VDR is the source for this field:
  //   This field contains a bitmask that correcsponds to the days of the week at which this timer runs
  //   It is based on the VDR Day field format "MTWTF--"
  //   The format is a 1 bit for every enabled day and a 0 bit for a disabled day
  //   Thus: WeekDays = "0000 0001" = "M------" (monday only)
  //                    "0110 0000" = "-----SS" (saturday and sunday)
  //                    "0001 1111" = "MTWTF--" (all weekdays)

  switch (repeatflags)
  {
    case 0:
      return TvDatabase::Once;
      break;
    case 1: //Monday
    case 2: //Tuesday
    case 4: //Wednesday
    case 8: //Thursday
    case 16: //Friday
    case 32: //Saturday
    case 64: //Sunday
      return TvDatabase::Weekly;
      break;
    case 31:  // 0001 1111
      return TvDatabase::WorkingDays;
    case 96:  // 0110 0000
      return TvDatabase::Weekends;
      break;
    case 127: // 0111 1111
      return TvDatabase::Daily;
      break;
    default:
      break;
  }

  return TvDatabase::Once;
}

std::string cTimer::AddScheduleCommand()
{
  char      command[1024];
  struct tm starttime;
  struct tm endtime;
  struct tm keepdate;

  starttime = *localtime( &m_starttime );
  XBMC->Log(LOG_DEBUG, "Start time: %s, marginstart: %i min earlier", asctime(&starttime), m_prerecordinterval);
  endtime = *localtime( &m_endtime );
  XBMC->Log(LOG_DEBUG, "End time: %s, marginstop: %i min later", asctime(&endtime), m_postrecordinterval);
  keepdate = *localtime( &m_keepdate );

  snprintf(command, 1023, "AddSchedule:%i|%s|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i\n",
          m_channel,                                                         //Channel number [0]
          uri::encode(uri::PATH_TRAITS, m_title).c_str(),                    //Program title  [1]
          starttime.tm_year + 1900, starttime.tm_mon + 1, starttime.tm_mday, //Start date     [2..4]
          starttime.tm_hour, starttime.tm_min, starttime.tm_sec,             //Start time     [5..7]
          endtime.tm_year + 1900, endtime.tm_mon + 1, endtime.tm_mday,       //End date       [8..10]
          endtime.tm_hour, endtime.tm_min, endtime.tm_sec,                   //End time       [11..13]
          (int) m_schedtype, m_priority, (int) m_keepmethod,                 //SchedType, Priority, keepMethod [14..16]
          keepdate.tm_year + 1900, keepdate.tm_mon + 1, keepdate.tm_mday,    //Keepdate       [17..19]
          keepdate.tm_hour, keepdate.tm_min, keepdate.tm_sec,                //Keeptime       [20..22]
          m_prerecordinterval, m_postrecordinterval);                        //Prerecord,postrecord [23,24]

  return command;
}

std::string cTimer::UpdateScheduleCommand()
{
  char      command[1024];
  struct tm starttime;
  struct tm endtime;
  struct tm keepdate;

  starttime = *localtime( &m_starttime );
  XBMC->Log(LOG_DEBUG, "Start time: %s, marginstart: %i min earlier", asctime(&starttime), m_prerecordinterval);
  endtime = *localtime( &m_endtime );
  XBMC->Log(LOG_DEBUG, "End time: %s, marginstop: %i min later", asctime(&endtime), m_postrecordinterval);
  keepdate = *localtime( &m_keepdate );

  snprintf(command, 1024, "UpdateSchedule:%i|%i|%i|%s|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i\n",
          m_index,                                                           //Schedule index [0]
          m_active,                                                          //Active         [1]
          m_channel,                                                         //Channel number [2]
          uri::encode(uri::PATH_TRAITS,m_title).c_str(),                     //Program title  [3]
          starttime.tm_year + 1900, starttime.tm_mon + 1, starttime.tm_mday, //Start date     [4..6]
          starttime.tm_hour, starttime.tm_min, starttime.tm_sec,             //Start time     [7..9]
          endtime.tm_year + 1900, endtime.tm_mon + 1, endtime.tm_mday,       //End date       [10..12]
          endtime.tm_hour, endtime.tm_min, endtime.tm_sec,                   //End time       [13..15]
          (int) m_schedtype, m_priority, (int) m_keepmethod,                 //SchedType, Priority, keepMethod [16..18]
          keepdate.tm_year + 1900, keepdate.tm_mon + 1, keepdate.tm_mday,    //Keepdate       [19..21]
          keepdate.tm_hour, keepdate.tm_min, keepdate.tm_sec,                //Keeptime       [22..24]
          m_prerecordinterval, m_postrecordinterval, m_progid);              //Prerecord,postrecord,program_id [25,26,27]

  return command;
}


int cTimer::XBMC2MepoPriority(int xbmcprio)
{
  // From XBMC side: 0.99 where 0=lowest and 99=highest priority (like VDR). Default value: 50
  // Meaning of the MediaPortal field is unknown to me. Default seems to be 0.
  // TODO: figure out the mapping
  return 0;
}

int cTimer::Mepo2XBMCPriority(int mepoprio)
{
  return 50; //Default value
}


/*
 * @brief Convert a XBMC Lifetime value to MediaPortals keepMethod+keepDate settings
 * @param lifetime the XBMC lifetime value (in days) (following the VDR syntax)
 * Should be called after setting m_starttime !!
 */
void cTimer::SetKeepMethod(int lifetime)
{
  // XBMC follows the VDR definition of lifetime
  // XBMC: 0 means that this recording may be automatically deleted
  //         at  any  time  by a new recording with higher priority
  //    1-98 means that this recording may not be automatically deleted
  //         in favour of a new recording, until the given number of days
  //         since the start time of the recording has passed by
  //      99 means that this recording will never be automatically deleted
  if (lifetime == 0)
  {
    m_keepmethod = TvDatabase::UntilSpaceNeeded;
    m_keepdate = cUndefinedDate;
  }
  else if (lifetime == 99)
  {
    m_keepmethod = TvDatabase::Always;
    m_keepdate = cUndefinedDate;
  }
  else
  {
    m_keepmethod = TvDatabase::TillDate;
    m_keepdate = m_starttime + (lifetime * cSecsInDay);
  }
}

int cTimer::GetLifetime(void)
{
  // margro: the meaning of the XBMC-PVR Lifetime field is undocumented.
  // Assuming that VDR is the source for this field:
  //  The guaranteed lifetime (in days) of a recording created by this
  //  timer.  0 means that this recording may be automatically deleted
  //  at  any  time  by a new recording with higher priority. 99 means
  //  that this recording will never  be  automatically  deleted.  Any
  //  number  in the range 1...98 means that this recording may not be
  //  automatically deleted in favour of a new  recording,  until  the
  //  given  number  of days since the start time of the recording has
  //  passed by
  switch (m_keepmethod)
  {
    case TvDatabase::UntilSpaceNeeded: //until space needed
    case TvDatabase::UntilWatched: //until watched
      return 0;
      break;
    case TvDatabase::TillDate: //until keepdate
      {
        double diffseconds = difftime(m_keepdate, m_starttime);
        int daysremaining = (int)(diffseconds / cSecsInDay);
        // Calculate value in the range 1...98, based on m_keepdate
        if (daysremaining < 99)
        {
          return daysremaining;
        }
        else
        {
          // > 98 days => return forever
          return 99;
        }
      }
      break;
    case TvDatabase::Always: //forever
      return 99;
    default:
      return 0;
  }
}

time_t cTimer::Now()
{
  time_t now;

  time(&now);

  return now;
}
