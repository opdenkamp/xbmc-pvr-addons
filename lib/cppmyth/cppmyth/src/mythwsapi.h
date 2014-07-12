/*
 *      Copyright (C) 2014 Jean-Luc Barriere
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
 *  along with this program; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301 USA
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifndef MYTHWSAPI_H
#define	MYTHWSAPI_H

#include "mythtypes.h"
#include "mythwsstream.h"

namespace PLATFORM
{
  class CMutex;
}

namespace Myth
{

  class WSAPI
  {
  public:
    WSAPI(const std::string& server, unsigned port);
    ~WSAPI();

    unsigned CheckService();
    void InvalidateService();
    std::string GetServerHostName();
    VersionPtr GetVersion();

    /**
     * @brief Query setting by its key
     * @param key
     * @param myhost
     * @return SettingPtr
     */
    SettingPtr GetSetting(const std::string& key, bool myhost);
    /**
     * @brief Query all settings
     * @param myhost
     * @return SettingMapPtr
     */
    SettingMapPtr GetSettings(bool myhost);
    /**
     * @brief Put setting
     * @param key
     * @param value
     * @param myhost
     * @return bool
     */
    bool PutSetting(const std::string& key, const std::string& value, bool myhost);
    /**
     * @brief Query information on all recorded programs
     * @param n
     * @param descending
     * @return ProgramListPtr
     */
    ProgramListPtr GetRecordedList(unsigned n = 0, bool descending = false);
    /**
     * @brief Query information on a single item from recordings
     * @param chanid
     * @param recstartts
     * @return ProgramPtr
     */
    ProgramPtr GetRecorded(uint32_t chanid, time_t recstartts);
    /**
     * @brief Update watched status for a recorded
     * @param chanid
     * @param recstartts
     * @return bool
     */
    bool UpdateRecordedWatchedStatus(uint32_t chanid, time_t recstartts, bool watched)
    {
      unsigned proto = CheckService();
      if (proto >= 79) return UpdateRecordedWatchedStatus79(chanid, recstartts, watched);
      return false;
    }
    /**
     * @brief Get all configured capture devices
     * @return CaptureCardListPtr
     */
    CaptureCardListPtr GetCaptureCardList();
    /**
     * @brief Get all video sources
     * @return VideoSourceListPtr
     */
    VideoSourceListPtr GetVideoSourceList();
    /**
     * @brief Get all configured channels for a video source
     * @param sourceid
     * @param onlyVisible (default true)
     * @return ChannelListPtr
     */
    ChannelListPtr GetChannelList(uint32_t sourceid, bool onlyVisible = true)
    {
      unsigned proto = CheckService();
      if (proto >= 82) return GetChannelList82(sourceid, onlyVisible);
      if (proto >= 75) return GetChannelList75(sourceid, onlyVisible);
      return ChannelListPtr(new ChannelList);
    };
    /**
     * @brief Query the guide information for a particular time period and a channel
     * @param chanid
     * @param starttime
     * @param endtime
     * @return ProgramMapPtr
     */
    ProgramMapPtr GetProgramGuide(uint32_t chanid, time_t starttime, time_t endtime);
    /**
     * @brief Query all configured recording rules
     * @return RecordScheduleListPtr
     */
    RecordScheduleListPtr GetRecordScheduleList();
    /**
     * @brief Get a single recording rule, by record id
     * @param recordid
     * @return RecordSchedulePtr
     */
    RecordSchedulePtr GetRecordSchedule(uint32_t recordid);
    /**
     * @brief Add a new recording rule
     * @param record
     * @return status. On success Id is updated with the new.
     */
    bool AddRecordSchedule(RecordSchedule& record)
    {
      unsigned proto = CheckService();
      if (proto >= 76) return AddRecordSchedule76(record);
      if (proto >= 75) return AddRecordSchedule75(record);
      return false;
    }
    /**
     * @brief Update a recording rule
     * @param record
     * @return status
     */
    bool UpdateRecordSchedule(RecordSchedule& record)
    {
      unsigned proto = CheckService();
      if (proto >= 76) return UpdateRecordSchedule76(record);
      return false;
    }
    /**
     * @brief Disable a recording rule
     * @param recordid
     * @return status
     */
    bool DisableRecordSchedule(uint32_t recordid);
    /**
     * @brief Enable a recording rule
     * @param recordid
     * @return status
     */
    bool EnableRecordSchedule(uint32_t recordid);
    /**
     * @brief Remove a recording rule
     * @param recordid
     * @return status
     */
    bool RemoveRecordSchedule(uint32_t recordid);
    /**
     * @brief Query information on all upcoming programs matching recording rules
     * @return ProgramListPtr
     */
    ProgramListPtr GetUpcomingList()
    {
      unsigned proto = CheckService();
      if (proto >= 79) return GetUpcomingList79();
      if (proto >= 75) return GetUpcomingList75();
      return ProgramListPtr(new ProgramList);
    }
    /**
     * @brief Query information on upcoming items which will not record due to conflicts
     * @return ProgramListPtr
     */
    ProgramListPtr GetConflictList();
    /**
     * @brief Query information on recorded programs which are set to expire
     * @return ProgramListPtr
     */
    ProgramListPtr GetExpiringList();
    /**
     * @brief Download a given file from a given storage group
     * @param filename
     * @param sgname
     * @return WSStreamPtr
     */
    WSStreamPtr GetFile(const std::string& filename, const std::string& sgname);
    /**
     * @brief Get the icon file for a given channel
     * @param chanid
     * @param width (default 0)
     * @param height (default 0)
     * @return WSStreamPtr
     */
    WSStreamPtr GetChannelIcon(uint32_t chanid, unsigned width = 0, unsigned height = 0);
    /**
     * @brief Get, and optionally scale, an preview thumbnail for a given recording by timestamp, chanid and starttime
     * @param chanid
     * @param recstartts
     * @param width (default 0)
     * @param height (default 0)
     * @return WSStreamPtr
     */
    WSStreamPtr GetPreviewImage(uint32_t chanid, time_t recstartts, unsigned width = 0, unsigned height = 0);
    /**
     * @brief Get, and optionally scale, an image file of a given type (coverart, banner, fanart) for a given recording's inetref and season number.
     * @param chanid
     * @param recstartts
     * @param width (default 0)
     * @param height (default 0)
     * @return WSStreamPtr
     */
    WSStreamPtr GetRecordingArtwork(const std::string& type, const std::string& inetref, uint16_t season, unsigned width = 0, unsigned height = 0);

  protected:
    PLATFORM::CMutex *m_mutex;
    std::string m_server;
    unsigned m_port;
    bool m_checked;
    Version m_version;
    std::string m_serverHostName;

  private:
    bool CheckServerHostName();
    bool CheckVersion();

    ChannelListPtr GetChannelList75(uint32_t sourceid, bool onlyVisible);
    ChannelListPtr GetChannelList82(uint32_t sourceid, bool onlyVisible);
    void ProcessRecordIN(unsigned proto, RecordSchedule& record);
    void ProcessRecordOUT(unsigned proto, RecordSchedule& record);
    bool AddRecordSchedule75(RecordSchedule& record);
    bool AddRecordSchedule76(RecordSchedule& record);
    bool UpdateRecordSchedule76(RecordSchedule& record);
    ProgramListPtr GetUpcomingList75();
    ProgramListPtr GetUpcomingList79();
    bool UpdateRecordedWatchedStatus79(uint32_t chanid, time_t recstartts, bool watched);
  };

}

#endif	/* MYTHWSAPI_H */

