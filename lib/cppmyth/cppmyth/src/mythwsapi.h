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
     * @brief GET Myth/GetSetting
     */
    SettingPtr GetSetting(const std::string& key, const std::string hostname);

    /**
     * @brief GET Myth/GetSetting
     */
    SettingMapPtr GetSettings(const std::string hostname);

    /**
     * @brief GET Myth/GetSetting
     */
    SettingPtr GetSetting(const std::string& key, bool myhost);

    /**
     * @brief GET Myth/GetSetting
     */
    SettingMapPtr GetSettings(bool myhost);

    /**
     * @brief POST Myth/PutSetting
     */
    bool PutSetting(const std::string& key, const std::string& value, bool myhost);

    /**
     * @brief GET Dvr/GetRecordedList
     */
    ProgramListPtr GetRecordedList(unsigned n = 0, bool descending = false);

    /**
     * @brief GET Dvr/GetRecorded
     */
    ProgramPtr GetRecorded(uint32_t chanid, time_t recstartts);

    /**
     * @brief POST Dvr/UpdateRecordedWatchedStatus
     */
    bool UpdateRecordedWatchedStatus(uint32_t chanid, time_t recstartts, bool watched)
    {
      unsigned proto = CheckService();
      if (proto >= 79) return UpdateRecordedWatchedStatus79(chanid, recstartts, watched);
      return false;
    }

    /**
     * @brief POST Dvr/DeleteRecording
     */
    bool DeleteRecording(uint32_t chanid, time_t recstartts, bool forceDelete = false, bool allowRerecord = false)
    {
      unsigned proto = CheckService();
      if (proto >= 82) return DeleteRecording82(chanid, recstartts, forceDelete, allowRerecord);
      return false;
    }

    /**
     * @brief POST Dvr/UnDeleteRecording
     */
    bool UnDeleteRecording(uint32_t chanid, time_t recstartts)
    {
      unsigned proto = CheckService();
      if (proto >= 82) return UnDeleteRecording82(chanid, recstartts);
      return false;
    }

    /**
     * @brief GET Capture/GetCaptureCardList
     */
    CaptureCardListPtr GetCaptureCardList();

    /**
     * @brief GET Channel/GetVideoSourceList
     */
    VideoSourceListPtr GetVideoSourceList();

    /**
     * @brief GET Channel/GetChannelInfoList
     */
    ChannelListPtr GetChannelList(uint32_t sourceid, bool onlyVisible = true)
    {
      unsigned proto = CheckService();
      if (proto >= 82) return GetChannelList82(sourceid, onlyVisible);
      if (proto >= 75) return GetChannelList75(sourceid, onlyVisible);
      return ChannelListPtr(new ChannelList);
    };

    /**
     * @brief GET Guide/GetProgramGuide
     */
    ProgramMapPtr GetProgramGuide(uint32_t chanid, time_t starttime, time_t endtime);

    /**
     * @brief GET Dvr/GetRecordScheduleList
     */
    RecordScheduleListPtr GetRecordScheduleList();

    /**
     * @brief GET Dvr/GetRecordSchedule
     */
    RecordSchedulePtr GetRecordSchedule(uint32_t recordid);

    /**
     * @brief POST Dvr/AddRecordSchedule
     */
    bool AddRecordSchedule(RecordSchedule& record)
    {
      unsigned proto = CheckService();
      if (proto >= 76) return AddRecordSchedule76(record);
      if (proto >= 75) return AddRecordSchedule75(record);
      return false;
    }

    /**
     * @brief POST Dvr/UpdateRecordSchedule
     */
    bool UpdateRecordSchedule(RecordSchedule& record)
    {
      unsigned proto = CheckService();
      if (proto >= 76) return UpdateRecordSchedule76(record);
      return false;
    }

    /**
     * @brief POST Dvr/DisableRecordSchedule
     */
    bool DisableRecordSchedule(uint32_t recordid);

    /**
     * @brief POST Dvr/EnableRecordSchedule
     */
    bool EnableRecordSchedule(uint32_t recordid);

    /**
     * @brief POST Dvr/RemoveRecordSchedule
     */
    bool RemoveRecordSchedule(uint32_t recordid);

    /**
     * @brief GET Dvr/GetUpcomingList
     */
    ProgramListPtr GetUpcomingList()
    {
      unsigned proto = CheckService();
      if (proto >= 79) return GetUpcomingList79();
      if (proto >= 75) return GetUpcomingList75();
      return ProgramListPtr(new ProgramList);
    }

    /**
     * @brief GET Dvr/GetConflictList
     */
    ProgramListPtr GetConflictList();

    /**
     * @brief GET Dvr/GetExpiringList
     */
    ProgramListPtr GetExpiringList();

    /**
     * @brief GET Content/GetFile
     */
    WSStreamPtr GetFile(const std::string& filename, const std::string& sgname);

    /**
     * @brief GET Guide/GetChannelIcon
     */
    WSStreamPtr GetChannelIcon(uint32_t chanid, unsigned width = 0, unsigned height = 0);

    /**
     * @brief GET Content/GetPreviewImage
     */
    WSStreamPtr GetPreviewImage(uint32_t chanid, time_t recstartts, unsigned width = 0, unsigned height = 0);

    /**
     * @brief GET Content/GetRecordingArtwork
     */
    WSStreamPtr GetRecordingArtwork(const std::string& type, const std::string& inetref, uint16_t season, unsigned width = 0, unsigned height = 0);

    /**
     * @brief GET Content/GetRecordingArtworkList
     */
    ArtworkListPtr GetRecordingArtworkList(uint32_t chanid, time_t recstartts);

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
    bool DeleteRecording82(uint32_t chanid, time_t recstartts, bool forceDelete, bool allowRerecord);
    bool UnDeleteRecording82(uint32_t chanid, time_t recstartts);
  };

}

#endif	/* MYTHWSAPI_H */

