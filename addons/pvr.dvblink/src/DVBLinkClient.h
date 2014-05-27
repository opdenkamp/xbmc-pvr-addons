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

class DVBLinkClient : public PLATFORM::CThread
{
public:
  DVBLinkClient(ADDON::CHelper_libXBMC_addon *XBMC, CHelper_libXBMC_pvr *PVR, std::string clientname, std::string hostname, long port, bool showinfomsg, std::string username, std::string password, bool usetimeshift);
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
  const char * GetLiveStreamURL(const PVR_CHANNEL &channel, DVBLINK_STREAMTYPE streamtype, int width, int height, int bitrate, std::string audiotrack);
  bool OpenLiveStream(const PVR_CHANNEL &channel, DVBLINK_STREAMTYPE streamtype, int width, int height, int bitrate, std::string audiotrack);
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

private:
  bool DoEPGSearch(dvblinkremote::EpgSearchResult& epgSearchResult, const std::string& channelId, const long startTime, const long endTime, const std::string & programId = "");
  void SetEPGGenre(dvblinkremote::ItemMetadata& metadata, int& genre_type, int& genre_subtype);
  std::string GetBuildInRecorderObjectID();
  std::string GetRecordedTVByDateObjectID(const std::string& buildInRecoderObjectID);
  int GetInternalUniqueIdFromChannelId(const std::string& channelId);
  virtual void * Process(void);

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
  DVBLINK_STREAMTYPE m_streamtype;
  std::string m_clientname;
  std::string m_hostname;
  TimeShiftBuffer *m_tsBuffer;
  bool m_usetimeshift;
  bool m_showinfomsg;
  bool m_updating;
  std::string m_recordingsid;
};

/*!
 * @brief PVR macros
 */
#define PVR_STRCPY(dest, source) do { strncpy(dest, source, sizeof(dest)-1); dest[sizeof(dest)-1] = '\0'; } while(0)
#define PVR_STRCLR(dest) memset(dest, 0, sizeof(dest))
#define PVR_INT2STR(dest, source) sprintf(dest, "%d", source)
#define PVR_STR2INT(dest, source) dest = atoi(source)
