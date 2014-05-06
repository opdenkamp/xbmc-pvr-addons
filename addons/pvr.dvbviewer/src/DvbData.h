#pragma once

#ifndef PVR_DVBVIEWER_DVBDATA_H
#define PVR_DVBVIEWER_DVBDATA_H

#include "client.h"
#include "TimeshiftBuffer.h"
#include "platform/util/StdString.h"
#include "platform/threads/threads.h"
#include <list>

#define CHANNELDAT_HEADER_SIZE       (7)
#define ENCRYPTED_FLAG               (1 << 0)
#define RDS_DATA_FLAG                (1 << 2)
#define VIDEO_FLAG                   (1 << 3)
#define AUDIO_FLAG                   (1 << 4)
#define ADDITIONAL_AUDIO_TRACK_FLAG  (1 << 7)
#define DAY_SECS                     (24 * 60 * 60)
#define DELPHI_DATE                  (25569)

// minimum version required
#define RS_VERSION_MAJOR   1
#define RS_VERSION_MINOR   26
#define RS_VERSION_PATCH1  0
#define RS_VERSION_PATCH2  0
#define RS_VERSION_NUM  (RS_VERSION_MAJOR << 24 | RS_VERSION_MINOR << 16 | \
                          RS_VERSION_PATCH1 << 8 | RS_VERSION_PATCH2)
#define RS_VERSION_STR  XSTR(RS_VERSION_MAJOR) "." XSTR(RS_VERSION_MINOR) \
                          "." XSTR(RS_VERSION_PATCH1) "." XSTR(RS_VERSION_PATCH2)

/* forward declaration */
class DvbGroup;

typedef enum DVB_UPDATE_STATE
{
  DVB_UPDATE_STATE_NONE,
  DVB_UPDATE_STATE_FOUND,
  DVB_UPDATE_STATE_UPDATED,
  DVB_UPDATE_STATE_NEW
} DVB_UPDATE_STATE;

class DvbChannel
{
public:
  DvbChannel()
    : backendNr(0), epgId(0)
  {}

public:
  /*!< @brief unique id passed to xbmc database. see FIXME for more details */
  unsigned int id;
  /*!< @brief backend number for generating the stream url */
  unsigned int backendNr;
  /*!< @brief channel number on the frontend */
  unsigned int frontendNr;
  /*!< @brief list of backend ids (e.g AC3, other languages, ...) */
  std::list<uint64_t> backendIds;
  uint64_t epgId;
  CStdString name;
  CStdString streamURL;
  CStdString logoURL;
  bool radio;
  bool hidden;
  bool encrypted;
};

class DvbGroup
{
public:
  CStdString name;
  std::list<DvbChannel *> channels;
  bool radio;
  bool hidden;
};

class DvbEPGEntry
{
public:
  DvbEPGEntry()
    : genre(0)
  {}

public:
  int iEventId;
  CStdString strTitle;
  unsigned int iChannelUid;
  time_t startTime;
  time_t endTime;
  unsigned int genre;
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

class DvbRecording
{
public:
  enum Group
  {
    GroupDisabled = 0,
    GroupByDirectory,
    GroupByDate,
    GroupByFirstLetter,
    GroupByTVChannel,
    GroupBySeries,
  };

public:
  DvbRecording()
    : genre(0)
  {}

public:
  CStdString id;
  time_t startTime;
  int duration;
  unsigned int genre;
  CStdString title;
  CStdString streamURL;
  CStdString plot;
  CStdString plotOutline;
  CStdString channelName;
  CStdString thumbnailPath;
};

typedef std::vector<DvbChannel *> DvbChannels_t;
typedef std::vector<DvbGroup> DvbGroups_t;
typedef std::vector<DvbTimer> DvbTimers_t;

class Dvb
  : public PLATFORM::CThread
{
public:
  Dvb(void);
  ~Dvb();

  bool Open();
  bool IsConnected();

  CStdString GetBackendName();
  CStdString GetBackendVersion();
  PVR_ERROR GetDriveSpace(long long *total, long long *used);

  bool SwitchChannel(const PVR_CHANNEL& channel);
  unsigned int GetCurrentClientChannel(void);
  PVR_ERROR GetChannels(ADDON_HANDLE handle, bool bRadio);
  PVR_ERROR GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL& channel, time_t iStart, time_t iEnd);
  unsigned int GetChannelsAmount(void);

  PVR_ERROR GetChannelGroups(ADDON_HANDLE handle, bool bRadio);
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
  TimeshiftBuffer *GetTimeshiftBuffer();
  CStdString& GetLiveStreamURL(const PVR_CHANNEL& channelinfo);

protected:
  virtual void *Process(void);

private:
  // functions
  CStdString GetHttpXML(const CStdString& url);
  CStdString URLEncodeInline(const CStdString& data);
  bool LoadChannels();
  DvbTimers_t LoadTimers();
  void TimerUpdates();
  void GenerateTimer(const PVR_TIMER& timer, bool newtimer = true);
  int GetTimerId(const PVR_TIMER& timer);

  // helper functions
  void RemoveNullChars(CStdString& str);
  bool CheckBackendVersion();
  bool UpdateBackendStatus(bool updateSettings = false);
  time_t ParseDateTime(const CStdString& strDate, bool iso8601 = true);
  uint64_t ParseChannelString(const CStdString& str, CStdString& channelName);
  unsigned int GetChannelUid(const CStdString& str);
  unsigned int GetChannelUid(const uint64_t channelId);
  CStdString BuildURL(const char* path, ...);
  CStdString BuildExtURL(const CStdString& baseURL, const char* path, ...);
  CStdString ConvertToUtf8(const CStdString& src);
  long GetGMTOffset();

private:
  bool m_connected;
  unsigned int m_backendVersion;

  long m_timezone;
  struct { long long total, used; } m_diskspace;
  std::vector<CStdString> m_recfolders;

  CStdString m_url;
  unsigned int m_currentChannel;

  /* channels + active (not hidden) channels */
  DvbChannels_t m_channels;
  unsigned int m_channelAmount;

  /* channel groups + active (not hidden) groups */
  DvbGroups_t m_groups;
  unsigned int m_groupAmount;

  bool m_updateTimers;
  bool m_updateEPG;
  unsigned int m_recordingAmount;
  TimeshiftBuffer *m_tsBuffer;

  DvbTimers_t m_timers;
  unsigned int m_newTimerIndex;

  PLATFORM::CMutex m_mutex;
  PLATFORM::CCondition<bool> m_started;
};

#endif
