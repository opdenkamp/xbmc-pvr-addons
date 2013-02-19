#pragma once

#include "platform/os.h"
#include "libdvblinkremote/dvblinkremote.h"
#include "HttpPostClient.h"
#include "TimeShiftBuffer.h"
#include "xbmc_pvr_types.h"
#include "libXBMC_addon.h"
#include "libXBMC_pvr.h"
#include "client.h"
#include "platform/threads/mutex.h"
#include "platform/util/util.h"
#include <map>

using namespace dvblinkremote;
using namespace dvblinkremotehttp;
using namespace ADDON;

#define DVBLINK_BUILD_IN_RECORDER_SOURCE_ID   "8F94B459-EFC0-4D91-9B29-EC3D72E92677"

class DVBLinkClient
{
public:
  DVBLinkClient(CHelper_libXBMC_addon *XBMC, CHelper_libXBMC_pvr *PVR, std::string clientname, std::string hostname, long port, bool showinfomsg, std::string username, std::string password, bool usetimeshift, std::string timeshiftpath);
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

private:
  bool DoEPGSearch(EpgSearchResult& epgSearchResult, const std::string& channelId, const long startTime, const long endTime, const std::string & programId = "");
  bool LoadChannelsFromFile();
  void SetEPGGenre(Program *program, EPG_TAG *tag);
  std::string GetBuildInRecorderObjectID();
  std::string GetRecordedTVByDateObjectID(const std::string& buildInRecoderObjectID);
  int GetInternalUniqueIdFromChannelId(const std::string& channelId);


  HttpPostClient* m_httpClient; 
  IDVBLinkRemoteConnection* m_dvblinkRemoteCommunication;
  bool m_connected;
  std::map<int,Channel *> m_channelMap;
  Stream * m_stream;
  int m_currentChannelId;
  ChannelList* m_channels;
  long m_timerCount;
  long m_recordingCount;
  PLATFORM::CMutex        m_mutex;
  CHelper_libXBMC_pvr *PVR;
  CHelper_libXBMC_addon  *XBMC; 
  DVBLINK_STREAMTYPE m_streamtype;
  std::string m_clientname;
  std::string m_hostname;
  TimeShiftBuffer * m_tsBuffer;
  std::string m_timeshiftpath;
  bool m_usetimeshift;
  bool m_showinfomsg;
};

/*!
 * @brief PVR macros
 */
#define PVR_STRCPY(dest, source) do { strncpy(dest, source, sizeof(dest)-1); dest[sizeof(dest)-1] = '\0'; } while(0)
#define PVR_STRCLR(dest) memset(dest, 0, sizeof(dest))
#define PVR_INT2STR(dest, source) sprintf(dest, "%d", source)
#define PVR_STR2INT(dest, source) dest = atoi(source)
