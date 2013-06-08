#pragma once

#ifndef PVR_DVBVIEWER_DVBDATA_H
#define PVR_DVBVIEWER_DVBDATA_H

#include "client.h"
#include "TimeshiftBuffer.h"
#include "xmlParser.h"
#include "platform/util/StdString.h"
#include "platform/threads/threads.h"

#define CHANNELDAT_HEADER_SIZE       (7)
#define ENCRYPTED_FLAG               (1 << 0)
#define RDS_DATA_FLAG                (1 << 2)
#define VIDEO_FLAG                   (1 << 3)
#define AUDIO_FLAG                   (1 << 4)
#define ADDITIONAL_AUDIO_TRACK_FLAG  (1 << 7)
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
  unsigned int Symbolrate;
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
  unsigned short OrbitalPos;
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

class DvbChannelGroup
{
public:
  DvbChannelGroup(const CStdString& name = "")
    : m_name(name), m_state(DVB_UPDATE_STATE_NEW)
  {}

  void setName(const CStdString& name)
  {
    m_name = name;
  }

  const CStdString& getName()
  {
    return m_name;
  }

  bool operator==(const DvbChannelGroup& right) const
  {
    return (m_name == right.m_name);
  }

private:
  CStdString m_name;
  DVB_UPDATE_STATE m_state;
};

class DvbChannel
{
public:
  DvbChannel()
  {
    iChannelState = DVB_UPDATE_STATE_NEW;
  }

  bool operator==(const DvbChannel& right) const
  {
    bool bChanged = true;
    bChanged = bChanged && (bRadio         == right.bRadio);
    bChanged = bChanged && (iUniqueId      == right.iUniqueId);
    bChanged = bChanged && (iChannelNumber == right.iChannelNumber);
    bChanged = bChanged && (group          == right.group);
    bChanged = bChanged && (strChannelName == right.strChannelName);
    bChanged = bChanged && (strStreamURL   == right.strStreamURL);
    bChanged = bChanged && (strIconPath    == right.strIconPath);
    return bChanged;
  }

public:
  bool bRadio;
  unsigned int iUniqueId;
  unsigned int iChannelNumber;
  uint64_t iChannelId;
  uint64_t iEpgId;
  byte Encrypted;
  DvbChannelGroup group;
  CStdString strChannelName;
  CStdString strStreamURL;
  CStdString strIconPath;
  DVB_UPDATE_STATE iChannelState;
};

struct DvbEPGEntry
{
  int iEventId;
  CStdString strTitle;
  unsigned int iChannelUid;
  time_t startTime;
  time_t endTime;
  CStdString strPlotOutline;
  CStdString strPlot;
};

class DvbTimer
{
public:
  DvbTimer()
  {
    iUpdateState = DVB_UPDATE_STATE_NEW;
  }

  bool like(const DvbTimer& right) const
  {
    bool bChanged = true;
    bChanged = bChanged && (startTime   == right.startTime);
    bChanged = bChanged && (endTime     == right.endTime);
    bChanged = bChanged && (iChannelUid == right.iChannelUid);
    bChanged = bChanged && (bRepeating  == right.bRepeating);
    bChanged = bChanged && (iWeekdays   == right.iWeekdays);
    bChanged = bChanged && (iEpgId      == right.iEpgId);
    return bChanged;
  }

  bool operator==(const DvbTimer& right) const
  {
    bool bChanged = true;
    bChanged = bChanged && like(right);
    bChanged = bChanged && (state    == right.state);
    bChanged = bChanged && (strTitle == right.strTitle);
    bChanged = bChanged && (strPlot  == right.strPlot);
    return bChanged;
  }

public:
  CStdString strTitle;
  CStdString strPlot;
  unsigned int iChannelUid;
  time_t startTime;
  time_t endTime;
  bool bRepeating;
  int iWeekdays;
  int iEpgId;
  int iTimerId;
  int iPriority;
  int iFirstDay;
  PVR_TIMER_STATE state;
  DVB_UPDATE_STATE iUpdateState;
  unsigned int iClientIndex;
};

struct DvbRecording
{
  CStdString strRecordingId;
  time_t startTime;
  int iDuration;
  CStdString strTitle;
  CStdString strStreamURL;
  CStdString strPlot;
  CStdString strPlotOutline;
  CStdString strChannelName;
  CStdString strThumbnailPath;
};

typedef std::vector<DvbChannel> DvbChannels_t;
typedef std::vector<DvbTimer> DvbTimers_t;
typedef std::vector<DvbRecording> DvbRecordings_t;
typedef std::vector<DvbChannelGroup> DvbChannelGroups_t;

class Dvb
  : public PLATFORM::CThread
{
public:
  Dvb(void);
  ~Dvb();

  CStdString GetServerName();
  bool Open();
  bool IsConnected();
  PVR_ERROR SignalStatus(PVR_SIGNAL_STATUS& signalStatus);

  bool SwitchChannel(const PVR_CHANNEL& channel);
  unsigned int GetCurrentClientChannel(void);
  PVR_ERROR GetChannels(ADDON_HANDLE handle, bool bRadio);
  PVR_ERROR GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL& channel, time_t iStart, time_t iEnd);
  unsigned int GetChannelsAmount(void);

  PVR_ERROR GetChannelGroups(ADDON_HANDLE handle);
  PVR_ERROR GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP& group);
  unsigned int GetChannelGroupsAmount(void);

  PVR_ERROR GetTimers(ADDON_HANDLE handle);
  PVR_ERROR AddTimer(const PVR_TIMER& timer);
  PVR_ERROR UpdateTimer(const PVR_TIMER& timer);
  PVR_ERROR DeleteTimer(const PVR_TIMER& timer);
  unsigned int GetTimersAmount(void);

  PVR_ERROR GetRecordings(ADDON_HANDLE handle);
  PVR_ERROR DeleteRecording(const PVR_RECORDING& recinfo);
  unsigned int GetRecordingsAmount();

  bool OpenLiveStream(const PVR_CHANNEL& channelinfo);
  void CloseLiveStream();
  int ReadLiveStream(unsigned char *pBuffer, unsigned int iBufferSize);
  long long SeekLiveStream(long long iPosition, int iWhence /* = SEEK_SET */);
  long long PositionLiveStream(void);
  long long LengthLiveStream(void);
  CStdString GetLiveStreamURL(const PVR_CHANNEL& channelinfo);

protected:
  virtual void *Process(void);

private:
  // functions
  CStdString GetHttpXML(const CStdString& url);
  CStdString URLEncodeInline(const CStdString& strData);
  void SendSimpleCommand(const CStdString& strCommandURL);
  bool LoadChannels();
  DvbTimers_t LoadTimers();
  void TimerUpdates();
  void GenerateTimer(const PVR_TIMER& timer, bool bNewtimer = true);
  int GetTimerId(const PVR_TIMER& timer);

  // helper functions
  bool GetXMLValue(const XMLNode& node, const char* tag, int& value);
  bool GetXMLValue(const XMLNode& node, const char* tag, bool& value);
  bool GetXMLValue(const XMLNode& node, const char* tag, CStdString& value,
      bool localize = false);
  void GetPreferredLanguage();
  void GetTimeZone();
  void RemoveNullChars(CStdString& str);
  bool GetDeviceInfo();
  time_t ParseDateTime(const CStdString& strDate, bool iso8601 = true);
  uint64_t ParseChannelString(const CStdString& str, CStdString& channelName);
  unsigned int GetChannelUid(const CStdString& str);
  unsigned int GetChannelUid(const uint64_t channelId);
  CStdString ConvertToUtf8(const CStdString& src);

private:
  // members
  CStdString m_strDVBViewerVersion;
  bool m_bIsConnected;
  CStdString m_strServerName;
  CStdString m_strURL;
  CStdString m_strURLStream;
  CStdString m_strURLRecording;
  CStdString m_strEPGLanguage;
  int m_iTimezone;
  unsigned int m_iCurrentChannel;
  unsigned int m_iUpdateTimer;
  bool m_bUpdateTimers;
  bool m_bUpdateEPG;
  DvbChannels_t m_channels;
  DvbTimers_t m_timers;
  DvbRecordings_t m_recordings;
  DvbChannelGroups_t m_groups;
  TimeshiftBuffer *m_tsBuffer;

  unsigned int m_iClientIndexCounter;

  PLATFORM::CMutex m_mutex;
  PLATFORM::CCondition<bool> m_started;
};

#endif
