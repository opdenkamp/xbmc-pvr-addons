#pragma once 

#include "platform/util/StdString.h"
#include "xmlParser.h"
#include "client.h"
#include "platform/threads/threads.h"

#define CHANNELDAT_HEADER_SIZE       (7)
#define ENCRYPTED_FLAG               (1 << 0)
#define VIDEO_FLAG                   (1 << 3)
#define ADDITIONAL_AUDIO_TRACK_FLAG  (1 << 7)
#define DAY_MINS                     (24 * 60)
#define DAY_SECS                     (24 * 60 * 60)
#define DELPHI_DATE                  (25569)
#define RECORDING_THUMB_POS          (143)
#define MAX_RECORDING_THUMBS         (20)
#define RS_MIN_VERSION               (21)
    
struct ChannelsDat
{
  byte TunerType;
  byte ChannelGroup;
  byte SatModulationSystem;
  byte Flags;
  unsigned int Frequency;
  unsigned int Simbolrate;
  unsigned short LNB_LOF;
  unsigned short PMT_PID;
  unsigned short Reserved1;
  byte SatModulation;
  byte AVFormat;
  byte FEC;
  byte Reserved2;
  unsigned short Reserved3;
  byte Polarity;
  byte Reserved4;
  unsigned short Reserved5;
  byte Tone;
  byte Reserved6;
  unsigned short DiSEqCExt;
  byte DiSEqC;
  byte Reserved7;
  unsigned short Reserved8;
  unsigned short Audio_PID;
  unsigned short Reserved9;
  unsigned short Video_PID;
  unsigned short TransportStream_ID;
  unsigned short Teletext_PID;
  unsigned short OriginalNetwork_ID;
  unsigned short Service_ID;
  unsigned short PCR_PID;
  byte Root_len;
  char Root[25];
  byte ChannelName_len;
  char ChannelName[25];
  byte Category_len;
  char Category[25];
  byte Encrypted;
  byte Reserved10;
};

typedef enum DVB_UPDATE_STATE
{
    DVB_UPDATE_STATE_NONE,
    DVB_UPDATE_STATE_FOUND,
    DVB_UPDATE_STATE_UPDATED,
    DVB_UPDATE_STATE_NEW
} DVB_UPDATE_STATE;

struct DvbChannelGroup {
  std::string strGroupName;
  int iGroupState;

  DvbChannelGroup() 
  { 
    iGroupState = DVB_UPDATE_STATE_NEW;
  }

  bool operator==(const DvbChannelGroup &right) const
  {
    return (! strGroupName.compare(right.strGroupName));
  }

};

struct DvbChannel
{
  bool bRadio;
  int iUniqueId;
  int iChannelNumber;
  int iChannelId;
  uint64_t llEpgId;
  byte Encrypted;
  std::string strGroupName;
  std::string strChannelName;
  std::string strStreamURL;
  std::string strIconPath;
  int iChannelState;

  DvbChannel()
  {
    iChannelState = DVB_UPDATE_STATE_NEW;
  }
  
  bool operator==(const DvbChannel &right) const
  {
    bool bChanged = true;
    bChanged = bChanged && (bRadio == right.bRadio); 
    bChanged = bChanged && (iUniqueId == right.iUniqueId); 
    bChanged = bChanged && (iChannelNumber == right.iChannelNumber); 
    bChanged = bChanged && (! strGroupName.compare(right.strGroupName));
    bChanged = bChanged && (! strChannelName.compare(right.strChannelName));
    bChanged = bChanged && (! strStreamURL.compare(right.strStreamURL));
    bChanged = bChanged && (! strIconPath.compare(right.strIconPath));

    return bChanged;
  }

};

struct DvbEPGEntry 
{
  int iEventId;
  std::string strTitle;
  int iChannelId;
  time_t startTime;
  time_t endTime;
  std::string strPlotOutline;
  std::string strPlot;
};

struct DvbTimer
{
  std::string strTitle;
  std::string strPlot;
  int iChannelId;
  time_t startTime;
  time_t endTime;
  bool bRepeating; 
  int iWeekdays;
  int iEpgID;
  int iTimerID;
  int iPriority;
  int iFirstDay;
  PVR_TIMER_STATE state; 
  int iUpdateState;
  unsigned int iClientIndex;

  DvbTimer()
  {
    iUpdateState = DVB_UPDATE_STATE_NEW;
  }
  
  bool like(const DvbTimer &right) const
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
  
  bool operator==(const DvbTimer &right) const
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

struct DvbRecording
{
  std::string strRecordingId;
  time_t startTime;
  int iDuration;
  std::string strTitle;
  std::string strStreamURL;
  std::string strPlot;
  std::string strPlotOutline;
  std::string strChannelName;
  std::string strThumbnailPath;
};
 
class Dvb  : public PLATFORM::CThread
{
private:

  // members
  std::string m_strDVBViewerVersion;
  bool  m_bIsConnected;
  std::string m_strServerName;
  std::string m_strURL;
  std::string m_strURLStream;
  std::string m_strURLRecording;
  std::string m_strEPGLanguage;
  int m_iTimezone;
  int m_iNumRecordings;
  int m_iNumChannelGroups;
  int m_iCurrentChannel;
  unsigned int m_iUpdateTimer;
  bool m_bUpdateTimers;
  bool m_bUpdateEPG;
  std::vector<DvbChannel> m_channels;
  std::vector<DvbTimer> m_timers;
  std::vector<DvbRecording> m_recordings;
  std::vector<DvbChannelGroup> m_groups;
  std::vector<std::string> m_locations;

  unsigned int m_iClientIndexCounter;

  PLATFORM::CMutex m_mutex;
  PLATFORM::CCondition<bool> m_started;
 

  // functions

  CStdString GetHttpXML(CStdString& url);
  int GetChannelNumber(CStdString strChannelId);
  CStdString URLEncodeInline(const CStdString& strData);
  void SendSimpleCommand(const CStdString& strCommandURL);
  bool LoadChannels();
  std::vector<DvbTimer> LoadTimers();
  void TimerUpdates();
  void GenerateTimer(const PVR_TIMER &timer, bool bNewtimer = true);
  int GetTimerID(const PVR_TIMER &timer);

  // helper functions
  static bool GetInt(XMLNode xRootNode, const char* strTag, int& iIntValue);
  static bool GetBoolean(XMLNode xRootNode, const char* strTag, bool& bBoolValue);
  static bool GetString(XMLNode xRootNode, const char* strTag, CStdString& strStringValue);
  bool GetStringLng(XMLNode xRootNode, const char* strTag, CStdString& strStringValue);
  CStdString GetPreferredLanguage();
  void RemoveNullChars(CStdString &String);
#ifdef WIN32
  void AnsiToUtf8(std::string &String);
#endif
  bool GetDeviceInfo();


protected:
  virtual void *Process(void);

public:
  Dvb(void);
  ~Dvb();

  const char * GetServerName();
  bool IsConnected(); 
  int GetChannelsAmount(void);
  PVR_ERROR GetChannels(ADDON_HANDLE handle, bool bRadio);
  int ParseDateTime(CStdString strDate, bool iDateFormat = true);
  PVR_ERROR GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd);
  int GetCurrentClientChannel(void);
  int GetTimersAmount(void);
  PVR_ERROR GetTimers(ADDON_HANDLE handle);
  PVR_ERROR AddTimer(const PVR_TIMER &timer);
  PVR_ERROR UpdateTimer(const PVR_TIMER &timer);
  PVR_ERROR DeleteTimer(const PVR_TIMER &timer);
  unsigned int GetRecordingsAmount();
  PVR_ERROR GetRecordings(ADDON_HANDLE handle);
  PVR_ERROR DeleteRecording(const PVR_RECORDING &recinfo);
  unsigned int GetNumChannelGroups(void);
  PVR_ERROR GetChannelGroups(ADDON_HANDLE handle);
  PVR_ERROR GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group);
  PVR_ERROR SignalStatus(PVR_SIGNAL_STATUS &signalStatus);
  const char* GetLiveStreamURL(const PVR_CHANNEL &channelinfo);
  bool OpenLiveStream(const PVR_CHANNEL &channelinfo);
  void CloseLiveStream();
  bool SwitchChannel(const PVR_CHANNEL &channel);
  bool Open();
  void Action();
};

