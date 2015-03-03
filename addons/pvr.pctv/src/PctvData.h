#pragma once

//#include <vector>
#include "platform/util/StdString.h"
#include "client.h"
#include "rest.h"
#include "platform/threads/threads.h"
//#include "tinyxml/tinyxml.h"
#include <json/json.h>

#define PCTV_REST_INTERFACE false

#define CHANNELDATAVERSION  2

class CCurlFile
{
public:
  CCurlFile(void) {};
  ~CCurlFile(void) {};

  bool Get(const std::string &strURL, std::string &strResult);
};

typedef enum PCTV_UPDATE_STATE
{
  PCTV_UPDATE_STATE_NONE,
  PCTV_UPDATE_STATE_FOUND,
  PCTV_UPDATE_STATE_UPDATED,
  PCTV_UPDATE_STATE_NEW
} PCTV_UPDATE_STATE;


struct PctvChannel
{
  bool        bRadio;
  int         iUniqueId;
  int         iChannelNumber;
  int		  iSubChannelNumber;
  int         iEncryptionSystem;  
  std::string strChannelName;
  std::string strLogoPath;
  std::string strStreamURL;  

  bool operator < (const PctvChannel& channel) const
  {
	  return (strChannelName.compare(channel.strChannelName) < 0);
  }
};

struct PctvChannelGroup
{
  bool              bRadio;
  int               iGroupId;
  std::string       strGroupName;
  std::vector<int>  members;
};

struct PctvEpgEntry
{
  int         iBroadcastId;
  int         iChannelId;
  int         iGenreType;
  int         iGenreSubType;
  time_t      startTime;
  time_t      endTime;
  std::string strTitle;
  std::string strPlotOutline;
  std::string strPlot;
  std::string strIconPath;
  std::string strGenreString;
};

struct PctvEpgChannel
{
  std::string               strId;
  std::string               strName;
  std::vector<PctvEpgEntry> epg;
};

struct PctvTimer
{
  int             iId;
  std::string     strTitle;
  int             iChannelId;
  time_t          startTime;
  time_t          endTime;
  int             iStartOffset;
  int             iEndOffset;
  std::string     strProfile;
  std::string     strResult;
  PVR_TIMER_STATE state;  
};

struct PctvRecording
{
  std::string strRecordingId;
  time_t      startTime;
  int         iDuration;
  int         iLastPlayedPosition;
  std::string strTitle;
  std::string strStreamURL;
  std::string strPlot;
  std::string strPlotOutline;
  std::string strChannelName;
  std::string strDirectory;
  std::string strIconPath;
};

struct PctvConfig
{

	std::string Brand;
	std::string Caps;
	std::string Hostname;
	int Port;
	std::string GuestLink;

	void init(const Json::Value& data)
	{
		Brand = data["Brand"].asString();
		Caps = data["Caps"].asString();
		Hostname = data["Hostname"].asString();
		Port = data["Port"].asInt();
		GuestLink = data["GuestLink"].asString();
	}
	
	bool hasCapability(const std::string& cap)
	{
		CStdString caps = ";" + Caps + ";";
		if (caps.find(";" + cap + ";") != std::string::npos) {
			return true;
		}
		return false;
	}
};

class Pctv : public PLATFORM::CThread
{
public:
  /* Class interface */
  Pctv(void);
  ~Pctv();

  /* Server */
  bool Open();  
  bool IsConnected();
  
  /* Common */
  const char* GetBackendName(void);
  const char* GetBackendVersion(void);
  bool        IsSupported(const std::string& cap);

  /* Channels */
  unsigned int GetChannelsAmount(void);
  bool GetChannel(const PVR_CHANNEL &channel, PctvChannel &myChannel);
  PVR_ERROR GetChannels(ADDON_HANDLE handle, bool bRadio);  
  int GetCurrentClientChannel(void);  

  /* Groups */
  unsigned int GetChannelGroupsAmount(void);  
  PVR_ERROR GetChannelGroups(ADDON_HANDLE handle, bool bRadio);
  PVR_ERROR GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group);

  /* Recordings */
  PVR_ERROR GetRecordings(ADDON_HANDLE handle);
  bool GetRecordingFromLocation(CStdString strRecordingFolder);
  unsigned int GetRecordingsAmount(void);
  
  /* Timer */
  unsigned int GetTimersAmount(void);
  PVR_ERROR GetTimers(ADDON_HANDLE handle);
  PVR_ERROR AddTimer(const PVR_TIMER &timer);    

  /* EPG */
  PVR_ERROR GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd);  
  bool GetEPG(int id, time_t iStart, time_t iEnd, Json::Value& data);
  
  /* Preview */
  const char* GetLiveStreamURL(const PVR_CHANNEL &channelinfo);
  void CloseLiveStream();
  long long SeekLiveStream(long long iPosition, int iWhence /* = SEEK_SET */);
  long long PositionLiveStream(void);
  long long LengthLiveStream(void);
  int ReadLiveStream(unsigned char *pBuffer, unsigned int iBufferSize);
  
  /* Storage */
  PVR_ERROR GetStorageInfo(long long *total, long long *used);
  
private:
  // helper functions    
  bool LoadChannels();  
  bool GetFreeConfig();
  unsigned int GetEventId(long long EntryId);
  /*
  * \brief Get a channel list from PCTV Device via REST interface
  * \param id The channel list id
  */
  int RESTGetChannelList(int id, Json::Value& response);
  int RESTGetRecordings(Json::Value& response);
  int RESTGetChannelLists(Json::Value& response);
  int RESTGetTimer(Json::Value& response);  
  int RESTGetEpg(int id, time_t iStart, time_t iEnd, Json::Value& response);
  int RESTGetStorage(Json::Value& response);
  int RESTGetFolder(Json::Value& response);

  int RESTAddTimer(const PVR_TIMER &timer, Json::Value& response);  

  // helper functions    
  CStdString URLEncodeInline(const CStdString& sSrc);  
  void TransferChannels(ADDON_HANDLE handle);
  void TransferRecordings(ADDON_HANDLE handle);
  void TransferTimer(ADDON_HANDLE handle); 
  void TransferGroups(ADDON_HANDLE handle);  
  bool replace(std::string& str, const std::string& from, const std::string& to);
  bool IsRecordFolderSet(CStdString& partitionId);
  CStdString GetPreviewParams(ADDON_HANDLE handle, Json::Value entry);
  CStdString GetPreviewUrl(CStdString params);
  CStdString GetTranscodeProfileValue();
  CStdString GetStid(int id);
  CStdString GetChannelLogo(Json::Value entry);
  CStdString GetShortName(Json::Value entry);
  
  void *Process(void);
    
  // members
  PLATFORM::CMutex                  m_mutex;
  PLATFORM::CCondition<bool>        m_started;

  bool                              m_bIsConnected;  
  std::string                       m_strHostname;
  std::string                       m_strBaseUrl;
  std::string                       m_strBackendName;
  std::string                       m_strBackendVersion;
  PctvConfig                        m_config;
  int                               m_iBitrate;
  bool                              m_bTranscode;
  bool                              m_bUsePIN;
  int                               m_iPortWeb;    
  int                               m_iNumChannels;
  int                               m_iNumRecordings;  
  int                               m_iNumGroups;  
  std::string                       m_strPreviewMode;
  CStdString                        m_strStid;  
  bool                              m_bUpdating;  
  CStdString                        m_strBackendUrlNoAuth;
  
  std::vector<PctvEpgChannel>       m_epg;    
  std::vector<PctvChannel>          m_channels;  
  std::vector<PctvChannelGroup>     m_groups;  
  std::vector<PctvRecording>        m_recordings; 
  std::vector<PctvTimer>            m_timer;
  std::vector<std::string>			m_partitions;

};


