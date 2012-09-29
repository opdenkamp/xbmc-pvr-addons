#include "cppmyth.h"
#include "fileOps.h"

#include <xbmc_pvr_types.h>

#include <map>
#include <boost/bimap.hpp>
#include <boost/bimap/unordered_set_of.hpp>

const int RECORDING_RULES = 30006;

class RecordingRule : public MythTimer, public std::vector<std::pair<PVR_TIMER, MythProgramInfo> >
{
public:
  RecordingRule(const MythTimer &timer);
  RecordingRule& operator=(const MythTimer &timer);
  bool operator==(const int &id);

  RecordingRule* GetParent() const;
  void SetParent(RecordingRule &parent);

  bool HasModifiers() const;
  std::vector<RecordingRule*> GetModifiers() const;
  void AddModifier(RecordingRule &modifier);

  bool SameTimeslot(RecordingRule &rule) const;

  void push_back(std::pair<PVR_TIMER, MythProgramInfo> &_val);

private:
  void SaveTimerString(PVR_TIMER &timer);

  RecordingRule* m_parent;
  std::vector<RecordingRule*> m_modifiers;
  std::vector<boost::shared_ptr<CStdString> > m_stringStore;
};

class PVRClientMythTV
{
public:
  PVRClientMythTV();
  ~PVRClientMythTV();

  // Server
  bool Connect();
  const char *GetBackendName();
  const char *GetBackendVersion() const;
  const char *GetConnectionString() const;
  bool GetDriveSpace(long long *iTotal, long long *iUsed);

  // EPG
  PVR_ERROR GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd);

  // Channels
  int GetNumChannels();
  PVR_ERROR GetChannels(ADDON_HANDLE handle, bool bRadio);

  // Channel groups
  int GetChannelGroupsAmount() const;
  PVR_ERROR GetChannelGroups(ADDON_HANDLE handle, bool bRadio);
  PVR_ERROR GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group);

  // Recordings
  int GetRecordingsAmount(void);
  PVR_ERROR GetRecordings(ADDON_HANDLE handle);
  PVR_ERROR DeleteRecording(const PVR_RECORDING &recording);
  PVR_ERROR SetRecordingPlayCount(const PVR_RECORDING &recording, int count);
  PVR_ERROR SetRecordingLastPlayedPosition(const PVR_RECORDING &recording, int lastplayedposition);
  int GetRecordingLastPlayedPosition(const PVR_RECORDING &recording);

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
  PVR_ERROR CallMenuHook(const PVR_MENUHOOK &menuhook);

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
  CMutex m_lock;
  MythFile m_file;

  CStdString m_protocolVersion;
  CStdString m_connectionString;

  // Categories
  struct mythcat{};
  struct pvrcat{};
  typedef boost::bimap<
    boost::bimaps::unordered_set_of<boost::bimaps::tagged<CStdString, mythcat>, boost::hash<CStdString> >,
    boost::bimaps::tagged<int, pvrcat>
    > catbimap;
  catbimap m_categoryMap;

  int Genre(CStdString g);
  CStdString Genre(int g);

  // EPG
  time_t m_EPGstart;
  time_t m_EPGend;
  std::vector<MythProgram> m_EPG;

  // Channels
  std::map<int, MythChannel> m_channels;
  std::map<int, std::vector<int> > m_sources;
  boost::unordered_map<CStdString, std::vector<int> > m_channelGroups;

  // Recordings
  boost::unordered_map<CStdString, MythProgramInfo> m_recordings;
  static bool IsRecordingVisible(MythProgramInfo &recording);
  float GetRecordingFrameRate(MythProgramInfo &recording);

  // Timers
  std::vector<RecordingRule> m_recordingRules;
  void PVRtoMythTimer(const PVR_TIMER timer, MythTimer &mt);

  CStdString GetArtWork(FILE_OPTIONS storageGroup, const CStdString &shwTitle);
};
