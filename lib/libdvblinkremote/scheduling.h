/***************************************************************************
 * Copyright (C) 2012 Marcus Efraimsson.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included 
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 *
 ***************************************************************************/

#pragma once
 
#include <string>
#include <vector>

namespace dvblinkremote {
  /**
    * Abstract base class for schedules. 
    */
  class Schedule
  {
  public:
    /**
      * An enum for schedule types.
      */
    enum DVBLinkScheduleType
    {
      SCHEDULE_TYPE_MANUAL = 0, /**< Used for manual schedules. */ 
      SCHEDULE_TYPE_BY_EPG = 1 /**< Used for electronic program guide (EPG) schedules. */ 
    };

    /**
      * Initializes a new instance of the dvblinkremote::Schedule class.
      * @param scheduleType a constant DVBLinkScheduleType instance representing the type of 
      * schedule.
      * @param channelId a constant string reference representing the channel identifier.
      * @param recordingsToKeep an optional constant integer representing how many recordings to 
      * keep for a repeated recording. Default value is <tt>0</tt>, i.e. keep all recordings.
      */
    Schedule(const DVBLinkScheduleType scheduleType, const std::string& channelId, const int recordingsToKeep = 0);

    /**
      * Initializes a new instance of the dvblinkremote::Schedule class.
      * @param scheduleType a constant DVBLinkScheduleType instance representing the type of 
      * schedule.
      * @param id a constant string reference representing the schedule identifier.
      * @param channelId a constant string reference representing the channel identifier.
      * @param recordingsToKeep an optional constant integer representing how many recordings to 
      * keep for a repeated recording. Default value is <tt>0</tt>, i.e. keep all recordings.
      */
    Schedule(const DVBLinkScheduleType scheduleType, const std::string& id, const std::string& channelId, const int recordingsToKeep = 0);

    /**
      * Pure virtual destructor for cleaning up allocated memory.
      */
    virtual ~Schedule() = 0;

    /**
      * Gets the identifier of the schedule.
      * @return Schedule identifier
      */
    std::string& GetID();

    /**
      * Gets the channel identifier for the schedule.
      * @return Channel identifier
      */
    std::string& GetChannelID();

    /**
      * The user parameter of the schedule request.
      */
    std::string UserParameter;

    /**
      * A flag indicating that new schedule should be added even if there are other 
      * conflicting schedules present.
      */
    bool ForceAdd;

    /**
      * Indicates how many recordings to keep for a repeated recording.
      * \remark Possible values (1, 2, 3, 4, 5, 6, 7, 10; 0 - keep all)
      */
    int RecordingsToKeep;

    /**
      * Gets the type for the schedule .
      * @return DVBLinkScheduleType instance reference
      */
    DVBLinkScheduleType& GetScheduleType();

  protected:

    /**
      * Protected constructor used to solve diamond problem in adding schedules
      */	 
	Schedule();

  private:

    /**
      * The identifier for the schedule.
      */
    std::string m_id;

    /**
      * The channel identifier of the schedule.
      */
    std::string m_channelId;

    /**
      * The type of the schedule.
      */
    DVBLinkScheduleType m_scheduleType;
  };
  
  /**
    * Abstract base class for manual schedules. 
    */
  class ManualSchedule :  public virtual Schedule
  {
  public:
    /**
      * An enum to be used for constructing a bitflag to be used for defining repeated recordings for manual schedules.
      */
    enum DVBLinkManualScheduleDayMask 
    {
      MANUAL_SCHEDULE_DAY_MASK_SUNDAY = 1, /**< Sunday schedule. */
      MANUAL_SCHEDULE_DAY_MASK_MONDAY = 2, /**< Monday schedule. */
      MANUAL_SCHEDULE_DAY_MASK_TUESDAY = 4, /**< Tuesday schedule. */
      MANUAL_SCHEDULE_DAY_MASK_WEDNESDAY = 8, /**< Wednesday schedule. */
      MANUAL_SCHEDULE_DAY_MASK_THURSDAY = 16, /**< Thursday schedule. */
      MANUAL_SCHEDULE_DAY_MASK_FRIDAY = 32, /**< Friday schedule. */
      MANUAL_SCHEDULE_DAY_MASK_SATURDAY = 64, /**< Saturday schedule. */
      MANUAL_SCHEDULE_DAY_MASK_DAILY = 255 /**< Daily schedule. */
    };

    /**
      * Initializes a new instance of the dvblinkremote::ManualSchedule class.
      * @param channelId a constant string reference representing the channel identifier.
      * @param startTime a constant long representing the start time of the schedule.
      * @param duration a constant long representing the duration of the schedule. 
      * @param dayMask a constant long representing the day bitflag of the schedule.
      * \remark Construct the \p dayMask parameter by using bitwize operations on the DVBLinkManualScheduleDayMask.
      * @see DVBLinkManualScheduleDayMask
      * @param title of schedule
      */
    ManualSchedule(const std::string& channelId, const long startTime, const long duration, const long dayMask, const std::string& title = "");

    /**
      * Initializes a new instance of the dvblinkremote::ManualSchedule class.
      * @param id a constant string reference representing the schedule identifier.
      * @param channelId a constant string reference representing the channel identifier.
      * @param startTime a constant long representing the start time of the schedule.
      * @param duration a constant long representing the duration of the schedule. 
      * @param dayMask a constant long representing the day bitflag of the schedule.
      * \remark Construct the \p dayMask parameter by using bitwize operations on the DVBLinkManualScheduleDayMask.
      * @see DVBLinkManualScheduleDayMask
      * @param title of schedule
      */
    ManualSchedule(const std::string& id, const std::string& channelId, const long startTime, const long duration, const long dayMask, const std::string& title = "");

    /**
      * Pure virtual destructor for cleaning up allocated memory.
      */
    virtual ~ManualSchedule() = 0;

    /**
      * The title of the manual schedule request.
      */
    std::string Title;

    /**
      * Gets the start time for the manual schedule request.
      * @return Start time
      */
    long GetStartTime();

    /**
      * Gets the duration for the manual schedule request.
      * @return Duration
      */
    long GetDuration();
    
    /**
      * Gets the day bitmask for the manual schedule request.
      * @return Day bitmask
      */
    long GetDayMask();

  private:
    /**
      * The start time of the manual schedule request.
      */
    long m_startTime;

    /**
      * The duration of the manual schedule request.
      */
    long m_duration;

    /**
      * The day bitmask of the manual schedule request.
      */
    long m_dayMask;
  };

  /**
    * Abstract base class for electronic program guide (EPG) schedules. 
    */
  class EpgSchedule : public virtual Schedule
  {
  public:
    /**
      * Initializes a new instance of the dvblinkremote::EpgSchedule class.
      * @param channelId a constant string reference representing the channel identifier.
      * @param programId a constant string reference representing the program identifier.
      * @param repeat an optional constant boolean representing if the schedule should be 
      * repeated or not. Default value is <tt>false</tt>.
      * @param newOnly an optional constant boolean representing if only new programs 
      * have to be recorded. Default value is <tt>false</tt>.
      * @param recordSeriesAnytime an optional constant boolean representing whether to 
      * record only series starting around original program start time or any of them. 
      * Default value is <tt>false</tt>.
    */
    EpgSchedule(const std::string& channelId, const std::string& programId, const bool repeat = false, const bool newOnly = false, const bool recordSeriesAnytime = false);

    /**
      * Initializes a new instance of the dvblinkremote::EpgSchedule class.
      * @param id a constant string reference representing the schedule identifier.
      * @param channelId a constant string reference representing the channel identifier.
      * @param programId a constant string reference representing the program identifier.
      * @param repeat an optional constant boolean representing if the schedule should be 
      * repeated or not. Default value is <tt>false</tt>.
      * @param newOnly an optional constant boolean representing if only new programs 
      * have to be recorded. Default value is <tt>false</tt>.
      * @param recordSeriesAnytime an optional constant boolean representing whether to 
      * record only series starting around original program start time or any of them. 
      * Default value is <tt>false</tt>.
    */
    EpgSchedule(const std::string& id, const std::string& channelId, const std::string& programId, const bool repeat = false, const bool newOnly = false, const bool recordSeriesAnytime = false);

    /**
      * Pure virtual destructor for cleaning up allocated memory.
      */
    virtual ~EpgSchedule() = 0;

    /**
      * Gets the program identifier for the schedule.
      * @return Program identifier
      */
    std::string& GetProgramID();

    /**
      * The repeat flag for the schedule.
      */
    bool Repeat;

    /**
      * Flag representing if only new programs have to be recorded.
      */
    bool NewOnly;

    /**
      * Flag representing whether to record only series starting around 
      * original program start time or any of them.
      */
    bool RecordSeriesAnytime;

  private:
    /**
      * The program identifier for the schedule.
      */
    std::string m_programId;
  };
};
