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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "platform/util/StdString.h"
#include "xbmc_pvr_types.h"
#include "client.h"
#include "fileOps.h"
#include "cppmyth.h"
#include <vector>
#include <map>
#include <boost/bimap.hpp>
#include <boost/bimap/unordered_set_of.hpp>
#include <boost/shared_ptr.hpp>

/*!
 * @brief PVR macros for string exchange
 */
#define PVR_STRCPY(dest, source) do { strncpy(dest, source, sizeof(dest)-1); dest[sizeof(dest)-1] = '\0'; } while(0)
#define PVR_STRCLR(dest) memset(dest, 0, sizeof(dest))

//const int RECORDING_RULES = 30006;

class RecordingRule : public MythTimer, public std::vector< std::pair< PVR_TIMER, MythProgramInfo > >
{
public:
  RecordingRule(const MythTimer& timer);
  RecordingRule& operator=(const MythTimer& t);
  bool operator==(const int &id);
  RecordingRule* GetParent();
  void SetParent(RecordingRule& parent);
  void AddModifier(RecordingRule& modifier);
  std::vector< RecordingRule* > GetModifiers();
  bool HasModifiers();
  bool SameTimeslot(RecordingRule& rule);
  void push_back(std::pair< PVR_TIMER, MythProgramInfo > &_val);

private:
  void SaveTimerString(PVR_TIMER& timer);
  RecordingRule* m_parent;
  std::vector< RecordingRule* > m_modifiers;
  std::vector< boost::shared_ptr<CStdString> > m_stringStore;
};

struct PVRcmythEpgEntry
{
  int         iBroadcastId;
  std::string strTitle;
  int         iChannelId;
  time_t      startTime;
  time_t      endTime;
  std::string strPlotOutline;
  std::string strPlot;
  std::string strIconPath;
  int         iGenreType;
  int         iGenreSubType;
//  time_t      firstAired;
//  int         iParentalRating;
//  int         iStarRating;
//  bool        bNotify;
//  int         iSeriesNumber;
//  int         iEpisodeNumber;
//  int         iEpisodePartNumber;
//  std::string strEpisodeName;
};

struct PVRcmythChannel
{
  bool                    bRadio;
  int                     iUniqueId;
  int                     iChannelNumber;
  int                     iEncryptionSystem;
  std::string             strChannelName;
  std::string             strIconPath;
  std::string             strStreamURL;
  std::vector<PVRcmythEpgEntry> epg;
};

struct PVRcmythChannelGroup
{
  bool             bRadio;
  int              iGroupId;
  std::string      strGroupName;
  std::vector<int> members;
};

class PVRcmyth
{
public:
  PVRcmyth(void);
  virtual ~PVRcmyth(void);
  
  //General
  virtual bool Connect();
  virtual bool GetLiveTVPriority();
  virtual void SetLiveTVPriority(bool enabled);
  virtual CStdString GetArtWork(FILE_OPTIONS storageGroup, CStdString shwTitle);
  //virtual PVR_ERROR CallMenuHook(const PVR_MENUHOOK &menuhook);
  //virtual const char * GetBackendName();
  //virtual const char * GetBackendVersion();
  //virtual const char * GetConnectionString();
  //virtual bool GetDriveSpace(long long *iTotal, long long *iUsed);
  //virtual int GetNumChannels();

  //Channels
  virtual int GetChannelsAmount(void);
  virtual PVR_ERROR GetChannels(ADDON_HANDLE handle, bool bRadio);
  virtual bool GetChannel(const PVR_CHANNEL &channel, PVRcmythChannel &myChannel);
  virtual int GetChannelGroupsAmount(void);
  virtual PVR_ERROR GetChannelGroups(ADDON_HANDLE handle, bool bRadio);
  virtual PVR_ERROR GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group);
  virtual PVR_ERROR GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd);
  
  //LiveTV
  virtual bool OpenLiveStream(const PVR_CHANNEL &channel);
  virtual void CloseLiveStream();
  virtual int ReadLiveStream(unsigned char *pBuffer, unsigned int iBufferSize);
  virtual long long SeekLiveStream(long long iPosition, int iWhence);
  virtual long long LengthLiveStream();
  virtual bool SwitchChannel(const PVR_CHANNEL &channel);
  virtual PVR_ERROR SignalStatus(PVR_SIGNAL_STATUS &signalStatus);
  //virtual int GetCurrentClientChannel();

  //Timers
  virtual int GetTimersAmount();
  virtual PVR_ERROR GetTimers(ADDON_HANDLE handle);
  virtual PVR_ERROR AddTimer(const PVR_TIMER &timer);
  virtual PVR_ERROR DeleteTimer(const PVR_TIMER &timer, bool bForceDelete);
  virtual PVR_ERROR UpdateTimer(const PVR_TIMER &timer);

  //Recordings
  virtual int GetRecordingsAmount(void);
  virtual PVR_ERROR GetRecordings(ADDON_HANDLE handle);
  virtual PVR_ERROR DeleteRecording(const PVR_RECORDING &recording);
  virtual PVR_ERROR SetRecordingPlayCount(const PVR_RECORDING &recording, int count);
  virtual bool OpenRecordedStream(const PVR_RECORDING &recinfo);
  virtual void CloseRecordedStream();
  virtual int ReadRecordedStream(unsigned char *pBuffer, unsigned int iBufferSize);
  virtual long long SeekRecordedStream(long long iPosition, int iWhence);
  virtual long long LengthRecordedStream();
  
protected:
  virtual bool LoadCategoryMap(void);
private:
//  std::vector<PVRcmythChannelGroup> m_groups;
//  std::vector<PVRcmythChannel>      m_channels;
//  time_t                            m_iEpgStart;
  CMutex m_lock;
  CStdString                       m_strDefaultIcon;
  CStdString                       m_strDefaultMovie;
  
  struct mythcat{};
  struct pvrcat{};
  typedef boost::bimap<
  boost::bimaps::unordered_set_of< boost::bimaps::tagged< CStdString , mythcat >,boost::hash< CStdString > >,
  boost::bimaps::tagged< int , pvrcat >
  > catbimap;

  fileOps2 *m_fOps2_client;
  MythConnection m_con;
  MythEventHandler * m_pEventHandler;
  MythDatabase m_db;
  MythRecorder m_rec;
  catbimap m_categoryMap;
  CStdString m_protocolVersion;
  CStdString m_connectionString;
  std::map< int, std::vector< int > > m_sources;
  boost::unordered_map< CStdString, std::vector< int > > m_groups;
  std::map< int , MythChannel > m_channels;
  time_t m_EPGstart;
  time_t m_EPGend;
  std::vector< MythProgram > m_EPG;
  int Genre(CStdString g);
  CStdString Genre(int g);
 
  MythFile m_file;
  std::vector< RecordingRule > m_recordingRules;
  boost::unordered_map< CStdString, MythProgramInfo > m_recordings;
  void PVRtoMythTimer(const PVR_TIMER timer, MythTimer& mt);
  
  //std::map< int , MythChannel > m_channels;
  //std::map< int, std::vector< int > > m_sources;
  //boost::unordered_map< CStdString, std::vector< int > > m_channelGroups;
  
};
