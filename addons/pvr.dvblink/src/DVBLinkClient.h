#pragma once

#include "platform/os.h"
#include "libdvblinkremote/dvblinkremote.h"
#include "HttpPostClient.h"
#include "xbmc_pvr_types.h"
#include "libXBMC_addon.h"
#include "libXBMC_pvr.h"
#include "client.h"
#include "platform/threads/mutex.h"
#include <map>


using namespace dvblinkremote;
using namespace dvblinkremotehttp;
using namespace ADDON;


#define DVBLINK_BUILD_IN_RECORDER_SOURCE_ID   "8F94B459-EFC0-4D91-9B29-EC3D72E92677"

class DVBLinkClient
{
public:
	DVBLinkClient(CHelper_libXBMC_addon *XBMC, CHelper_libXBMC_pvr *PVR, std::string clientname, std::string hostname, long port, std::string username, std::string password);
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
	void StopStreaming(bool bUseChlHandle);
	int GetCurrentChannelId();
	long GetFreeDiskSpace();
	long GetTotalDiskSpace();

private:
	bool DoEPGSearch(EpgSearchResult& epgSearchResult, const std::string& channelId, const long startTime, const long endTime, const std::string & programId = "");
	bool LoadChannelsFromFile();


	HttpPostClient* httpClient; 
	IDVBLinkRemoteConnection* dvblinkRemoteCommunication;
	bool connected;
	std::map<int,Channel *> channelMap;
	Stream  * stream;
	int currentChannelId;
	ChannelList* channels;
	long timerCount;
	long recordingCount;

	PLATFORM::CMutex        m_mutex;

	CHelper_libXBMC_pvr *PVR;
	CHelper_libXBMC_addon  *XBMC; 
	DVBLINK_STREAMTYPE streamtype;
	std::string clientname;
	std::string hostname;

	void SetEPGGenre(Program *program, EPG_TAG *tag);
	std::string GetBuildInRecorderObjectID();
	std::string GetRecordedTVByDateObjectID(const std::string& buildInRecoderObjectID);
	int GetInternalUniqueIdFromChannelId(const std::string& channelId);
};

/*!
 * @brief PVR macros
 */
#define PVR_STRCPY(dest, source) do { strncpy(dest, source, sizeof(dest)-1); dest[sizeof(dest)-1] = '\0'; } while(0)
#define PVR_STRCLR(dest) memset(dest, 0, sizeof(dest))
#define PVR_INT2STR(dest, source) sprintf(dest, "%d", source)
#define PVR_STR2INT(dest, source) dest = atoi(source)
#define SAFE_DELETE(p)       do { delete (p);     (p)=NULL; } while (0)
