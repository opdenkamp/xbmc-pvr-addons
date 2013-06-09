/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "cppmyth.h"
#include "fileOps.h"
#include "categories.h"

#include <xbmc_pvr_types.h>
#include <platform/threads/mutex.h>

class RecordingRule : public MythRecordingRule, public std::vector<std::pair<PVR_TIMER, MythProgramInfo> >
{
public:
  RecordingRule(const MythRecordingRule &rule);
  RecordingRule& operator=(const MythRecordingRule &rule);
  bool operator==(const unsigned int &id);

  RecordingRule* GetParent() const;
  void SetParent(RecordingRule &parent);

  bool HasOverrideRules() const;
  std::vector<RecordingRule*> GetOverrideRules() const;
  void AddOverrideRule(RecordingRule &overrideRule);

  bool SameTimeslot(RecordingRule &rule) const;

  void push_back(std::pair<PVR_TIMER, MythProgramInfo> &_val);

private:
  void SaveTimerString(PVR_TIMER &timer);

  RecordingRule* m_parent;
  std::vector<RecordingRule*> m_overrideRules;
  std::vector<boost::shared_ptr<CStdString> > m_stringStore;
};

typedef std::vector<RecordingRule> RecordingRuleList;

class PVRClientMythTV : public MythEventObserver
{
public:
  PVRClientMythTV();
  virtual ~PVRClientMythTV();

  // Server
  bool Connect();
  const char *GetBackendName();
  const char *GetBackendVersion();
  const char *GetConnectionString();
  bool GetDriveSpace(long long *iTotal, long long *iUsed);
  void OnSleep();
  void OnWake();

  // EPG
  PVR_ERROR GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd);

  // Channels
  int GetNumChannels();
  PVR_ERROR GetChannels(ADDON_HANDLE handle, bool bRadio);

  // Channel groups
  int GetChannelGroupsAmount();
  PVR_ERROR GetChannelGroups(ADDON_HANDLE handle, bool bRadio);
  PVR_ERROR GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group);

  // Recordings
  int GetRecordingsAmount(void);
  PVR_ERROR GetRecordings(ADDON_HANDLE handle);
  PVR_ERROR DeleteRecording(const PVR_RECORDING &recording);
  PVR_ERROR DeleteAndForgetRecording(const PVR_RECORDING &recording);
  PVR_ERROR SetRecordingPlayCount(const PVR_RECORDING &recording, int count);
  PVR_ERROR SetRecordingLastPlayedPosition(const PVR_RECORDING &recording, int lastplayedposition);
  int GetRecordingLastPlayedPosition(const PVR_RECORDING &recording);
  PVR_ERROR GetRecordingEdl(const PVR_RECORDING &recording, PVR_EDL_ENTRY entries[], int *size);

  // Timers
  int GetTimersAmount();
  PVR_ERROR GetTimers(ADDON_HANDLE handle);
  PVR_ERROR AddTimer(const PVR_TIMER &timer);
  PVR_ERROR DeleteTimer(const PVR_TIMER &timer, bool bForceDelete);
  PVR_ERROR UpdateTimer(const PVR_TIMER &timer);

  // LiveTV
  bool OpenLiveStream(const PVR_CHANNEL &channel);
  void CloseLiveStream();
  int ReadLiveStream(unsigned char *pBuffer, unsigned int iBufferSize);
  int GetCurrentClientChannel();
  bool SwitchChannel(const PVR_CHANNEL &channelinfo);
  long long SeekLiveStream(long long iPosition, int iWhence);
  long long LengthLiveStream();
  PVR_ERROR SignalStatus(PVR_SIGNAL_STATUS &signalStatus);

  // Recording playback
  bool OpenRecordedStream(const PVR_RECORDING &recinfo);
  void CloseRecordedStream();
  int ReadRecordedStream(unsigned char *pBuffer, unsigned int iBufferSize);
  long long SeekRecordedStream(long long iPosition, int iWhence);
  long long LengthRecordedStream();

  // Menu hook
  PVR_ERROR CallMenuHook(const PVR_MENUHOOK &menuhook, const PVR_MENUHOOK_DATA &item);

  // Backend settings
  bool GetLiveTVPriority();
  void SetLiveTVPriority(bool enabled);

private:
  // Backend
  MythConnection m_con;
  MythEventHandler *m_pEventHandler;
  MythDatabase m_db;
  MythRecorder m_rec;
  FileOps *m_fileOps;
  PLATFORM::CMutex m_lock;
  MythFile m_file;

  CStdString m_backendName;
  CStdString m_backendVersion;
  CStdString m_connectionString;

  // Categories
  Categories m_categories;

  // Channels
  ChannelIdMap m_channelsById;
  ChannelNumberMap m_channelsByNumber;
  ChannelGroupMap m_channelGroups;
  void LoadChannelsAndChannelGroups();

  // Recordings
  ProgramInfoMap m_recordings;
  PLATFORM::CMutex m_recordingsLock;
  void EventUpdateRecordings();
  void ForceUpdateRecording(ProgramInfoMap::iterator it);
  int FillRecordings();
  int GetRecordingLastPlayedPosition(MythProgramInfo &programInfo);

  // Timers
  RecordingRuleList m_recordingRules;
  void PVRtoMythRecordingRule(const PVR_TIMER timer, MythRecordingRule &rule);

  CStdString GetArtWork(FileOps::FileType storageGroup, const CStdString &shwTitle);
};
