/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://xbmc.org
 
 *      Copyright (C) 2012 Palle Ehmsen(Barcode Madness)
 *      http://www.barcodemadness.com
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301  USA
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
 
#pragma once

#include "platform/os.h"
#include "libdvblinkremote/dvblinkremote.h"
#include "HttpPostClient.h"
#include "TimeShiftBuffer.h"
#include "xbmc_pvr_types.h"
#include "libXBMC_addon.h"
#include "libXBMC_pvr.h"
#include "client.h"
#include "platform/threads/threads.h"
#include "platform/threads/mutex.h"
#include "platform/util/util.h"
#include <map>

#define DVBLINK_BUILD_IN_RECORDER_SOURCE_ID   "8F94B459-EFC0-4D91-9B29-EC3D72E92677"
#define DVBLINK_RECODINGS_BY_DATA_ID   "F6F08949-2A07-4074-9E9D-423D877270BB"
#define DVBLINK_RECODINGS_BY_SERIES_ID   "0E03FEB8-BD8F-46e7-B3EF-34F6890FB458"

typedef std::map<std::string, std::string> recording_id_to_url_map_t;

class DVBLinkClient : public PLATFORM::CThread
{
public:
    DVBLinkClient(ADDON::CHelper_libXBMC_addon* xbmc, CHelper_libXBMC_pvr* pvr, CHelper_libXBMC_gui* gui, std::string clientname, std::string hostname, long port, 
        bool showinfomsg, std::string username, std::string password, bool add_episode_to_rec_title, bool group_recordings_by_series);
  ~DVBLinkClient(void);
  int GetChannelsAmount();
  PVR_ERROR GetChannels(ADDON_HANDLE handle, bool bRadio);
  PVR_ERROR GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL& channel, time_t iStart, time_t iEnd);
  int GetRecordingsAmount();
  PVR_ERROR GetRecordings(ADDON_HANDLE handle);
  PVR_ERROR DeleteRecording(const PVR_RECORDING& recording);
  int GetTimersAmount();
  PVR_ERROR GetTimers(ADDON_HANDLE handle);
  PVR_ERROR AddTimer(const PVR_TIMER &timer);
  PVR_ERROR DeleteTimer(const PVR_TIMER &timer);
  PVR_ERROR UpdateTimer(const PVR_TIMER &timer);
  bool GetStatus();
  bool StartStreaming(const PVR_CHANNEL &channel, dvblinkremote::StreamRequest* streamRequest, std::string& stream_url);
  bool OpenLiveStream(const PVR_CHANNEL &channel, bool use_timeshift, bool use_transcoder, int width, int height, int bitrate, std::string audiotrack);
  void StopStreaming(bool bUseChlHandle);
  int GetCurrentChannelId();
  void GetDriveSpace(long long *iTotal, long long *iUsed);
  long long SeekLiveStream(long long iPosition, int iWhence);
  long long PositionLiveStream(void);
  long long LengthLiveStream(void);
  int ReadLiveStream(unsigned char *pBuffer, unsigned int iBufferSize);
  time_t GetPlayingTime();
  time_t GetBufferTimeStart();
  time_t GetBufferTimeEnd();
  bool GetRecordingURL(const char* recording_id, std::string& url);

private:
  bool DoEPGSearch(dvblinkremote::EpgSearchResult& epgSearchResult, const std::string& channelId, const long startTime, const long endTime, const std::string & programId = "");
  void SetEPGGenre(dvblinkremote::ItemMetadata& metadata, int& genre_type, int& genre_subtype);
  std::string GetBuildInRecorderObjectID();
  std::string GetRecordedTVByDateObjectID(const std::string& buildInRecoderObjectID);
  int GetInternalUniqueIdFromChannelId(const std::string& channelId);
  virtual void * Process(void);
  bool build_recording_series_map(std::map<std::string, std::string>& rec_id_to_series_name);

  std::string make_timer_hash(const std::string& timer_id, const std::string& schedule_id);
  bool parse_timer_hash(const char* timer_hash, std::string& timer_id, std::string& schedule_id);

  HttpPostClient* m_httpClient; 
  dvblinkremote::IDVBLinkRemoteConnection* m_dvblinkRemoteCommunication;
  bool m_connected;
  std::map<int,dvblinkremote::Channel *> m_channelMap;
  dvblinkremote::Stream * m_stream;
  int m_currentChannelId;
  dvblinkremote::ChannelList* m_channels;
  long m_timerCount;
  long m_recordingCount;
  PLATFORM::CMutex        m_mutex;
  CHelper_libXBMC_pvr *PVR;
  ADDON::CHelper_libXBMC_addon  *XBMC; 
  CHelper_libXBMC_gui   *GUI;
  std::string m_clientname;
  std::string m_hostname;
  LiveStreamerBase* m_live_streamer;
  bool m_add_episode_to_rec_title;
  bool m_group_recordings_by_series;
  bool m_showinfomsg;
  bool m_updating;
  std::string m_recordingsid;
  std::string m_recordingsid_by_date;
  std::string m_recordingsid_by_series;
  recording_id_to_url_map_t m_recording_id_to_url_map;
};

/*!
 * @brief PVR macros
 */
#define PVR_STRCPY(dest, source) do { strncpy(dest, source, sizeof(dest)-1); dest[sizeof(dest)-1] = '\0'; } while(0)
#define PVR_STRCLR(dest) memset(dest, 0, sizeof(dest))
#define PVR_INT2STR(dest, source) sprintf(dest, "%d", source)
#define PVR_STR2INT(dest, source) dest = atoi(source)
