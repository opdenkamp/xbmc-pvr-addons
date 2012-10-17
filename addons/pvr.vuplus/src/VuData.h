#pragma once 

#include "platform/util/StdString.h"
#include "client.h"
#include "platform/threads/threads.h"
#include "tinyxml/tinyxml.h"
    
#define CHANNELDATAVERSION  2

class CCurlFile
{
public:
  CCurlFile(void) {};
  ~CCurlFile(void) {};

  bool Get(const std::string &strURL, std::string &strResult);
};


typedef enum VU_UPDATE_STATE
{
    VU_UPDATE_STATE_NONE,
    VU_UPDATE_STATE_FOUND,
    VU_UPDATE_STATE_UPDATED,
    VU_UPDATE_STATE_NEW
} VU_UPDATE_STATE;

struct VuChannelGroup {
  std::string strServiceReference;
  std::string strGroupName;
  int iGroupState;

  VuChannelGroup() 
  { 
    iGroupState = VU_UPDATE_STATE_NEW;
  }

  bool operator==(const VuChannelGroup &right) const
  {
    return (! strServiceReference.compare(right.strServiceReference)) && (! strGroupName.compare(right.strGroupName));
  }

};

struct VuChannel
{
  bool bRadio;
  int iUniqueId;
  int iChannelNumber;
  std::string strGroupName;
  std::string strChannelName;
  std::string strServiceReference;
  std::string strStreamURL;
  std::string strIconPath;
  int iChannelState;

  VuChannel()
  {
    iChannelState = VU_UPDATE_STATE_NEW;
  }
  
  bool operator==(const VuChannel &right) const
  {
    bool bChanged = true;
    bChanged = bChanged && (bRadio == right.bRadio); 
    bChanged = bChanged && (iUniqueId == right.iUniqueId); 
    bChanged = bChanged && (iChannelNumber == right.iChannelNumber); 
    bChanged = bChanged && (! strGroupName.compare(right.strGroupName));
    bChanged = bChanged && (! strChannelName.compare(right.strChannelName));
    bChanged = bChanged && (! strServiceReference.compare(right.strServiceReference));
    bChanged = bChanged && (! strStreamURL.compare(right.strStreamURL));
    bChanged = bChanged && (! strIconPath.compare(right.strIconPath));

    return bChanged;
  }

};

struct VuEPGEntry 
{
  int iEventId;
  std::string strServiceReference;
  std::string strTitle;
  int iChannelId;
  time_t startTime;
  time_t endTime;
  std::string strPlotOutline;
  std::string strPlot;
};

struct VuTimer
{
  std::string strTitle;
  std::string strPlot;
  int iChannelId;
  time_t startTime;
  time_t endTime;
  bool bRepeating; 
  int iWeekdays;
  int iEpgID;
  PVR_TIMER_STATE state; 
  int iUpdateState;
  unsigned int iClientIndex;

  VuTimer()
  {
    iUpdateState = VU_UPDATE_STATE_NEW;
  }
  
  bool like(const VuTimer &right) const
  {
    bool bChanged = true;
    bChanged = bChanged && (startTime == right.startTime); 
    bChanged = bChanged && (endTime == right.endTime); 
    bChanged = bChanged && (iChannelId == right.iChannelId); 
    bChanged = bChanged && (bRepeating == right.bRepeating); 
    bChanged = bChanged && (iWeekdays == right.iWeekdays); 
    bChanged = bChanged && (iEpgID == right.iEpgID); 

    return bChanged;
  }
  
  bool operator==(const VuTimer &right) const
  {
    bool bChanged = true;
    bChanged = bChanged && (startTime == right.startTime); 
    bChanged = bChanged && (endTime == right.endTime); 
    bChanged = bChanged && (iChannelId == right.iChannelId); 
    bChanged = bChanged && (bRepeating == right.bRepeating); 
    bChanged = bChanged && (iWeekdays == right.iWeekdays); 
    bChanged = bChanged && (iEpgID == right.iEpgID); 
    bChanged = bChanged && (state == right.state); 
    bChanged = bChanged && (! strTitle.compare(right.strTitle));
    bChanged = bChanged && (! strPlot.compare(right.strPlot));

    return bChanged;
  }
};

struct VuRecording
{
  std::string strRecordingId;
  time_t startTime;
  int iDuration;
  int iLastPlayedPosition;
  std::string strTitle;
  std::string strStreamURL;
  std::string strPlot;
  std::string strPlotOutline;
  std::string strChannelName;
};
 
class Vu  : public PLATFORM::CThread
{
private:

  // members
  std::string m_strEnigmaVersion;
  std::string m_strImageVersion;
  std::string m_strWebIfVersion;
  bool  m_bIsConnected;
  std::string m_strServerName;
  std::string m_strURL;
  int m_iNumRecordings;
  int m_iNumChannelGroups;
  int m_iCurrentChannel;
  unsigned int m_iUpdateTimer;
  std::vector<VuChannel> m_channels;
  std::vector<VuTimer> m_timers;
  std::vector<VuRecording> m_recordings;
  std::vector<VuChannelGroup> m_groups;
  std::vector<std::string> m_locations;
  bool m_bInitial;
  unsigned int m_iClientIndexCounter;

  PLATFORM::CMutex m_mutex;
  PLATFORM::CCondition<bool> m_started;

  bool m_bUpdating;

  // functions
  void StoreChannelData();
  void LoadChannelData();
  bool StoreLastPlayedPositions();
  bool RestoreLastPlayedPositions();
  CStdString GetHttpXML(CStdString& url);
  int GetChannelNumber(CStdString strServiceReference);
  bool SendSimpleCommand(const CStdString& strCommandURL, CStdString& strResult, bool bIgnoreResult = false);
  CStdString GetGroupServiceReference(CStdString strGroupName);
  bool LoadChannels(CStdString strServerReference, CStdString strGroupName);
  bool LoadChannels();
  bool LoadChannelGroups();
  bool LoadLocations();
  std::vector<VuTimer> LoadTimers();
  void TimerUpdates();
  bool GetDeviceInfo();
  int GetRecordingIndex(CStdString);

  // helper functions
  static long TimeStringToSeconds(const CStdString &timeString);
  static int SplitString(const CStdString& input, const CStdString& delimiter, CStdStringArray &results, unsigned int iMaxStrings = 0);
  bool CheckForGroupUpdate();
  bool CheckForChannelUpdate();
  std::string& Escape(std::string &s, std::string from, std::string to);
  CStdString URLEncodeInline(const CStdString& sSrc);

protected:
  virtual void *Process(void);

public:
  Vu(void);
  ~Vu();

  const char * GetServerName();
  bool IsConnected(); 
  int GetChannelsAmount(void);
  PVR_ERROR GetChannels(ADDON_HANDLE handle, bool bRadio);
  PVR_ERROR GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd);
  int GetCurrentClientChannel(void);
  int GetTimersAmount(void);
  PVR_ERROR GetTimers(ADDON_HANDLE handle);
  PVR_ERROR AddTimer(const PVR_TIMER &timer);
  PVR_ERROR SetRecordingLastPlayedPosition(const PVR_RECORDING &recording, int lastplayedposition);
  bool SetRecordingLastPlayedPosition(CStdString strStreamURL, int lastplayedposition);
  int GetRecordingLastPlayedPosition(const PVR_RECORDING &recording);
  PVR_ERROR UpdateTimer(const PVR_TIMER &timer);
  PVR_ERROR DeleteTimer(const PVR_TIMER &timer);
  bool GetRecordingFromLocation(ADDON_HANDLE handle, CStdString strRecordingFolder);
  unsigned int GetRecordingsAmount();
  PVR_ERROR    GetRecordings(ADDON_HANDLE handle);
  PVR_ERROR    DeleteRecording(const PVR_RECORDING &recinfo);
  unsigned int GetNumChannelGroups(void);
  PVR_ERROR    GetChannelGroups(ADDON_HANDLE handle);
  PVR_ERROR    GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group);
  const char* GetLiveStreamURL(const PVR_CHANNEL &channelinfo);
  bool OpenLiveStream(const PVR_CHANNEL &channelinfo);
  void CloseLiveStream();
  void SendPowerstate();
  bool SwitchChannel(const PVR_CHANNEL &channel);
  bool Open();
  void Action();
};

