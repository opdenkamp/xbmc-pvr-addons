/*
 *      Copyright (C) 2011 Pulse-Eight
 *      http://www.pulse-eight.com/
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

#include "cppmyth.h"
#include <vector>
#include "platform/util/StdString.h"
#include "client.h"
#include <map>
#include <boost/bimap.hpp>
#include <boost/bimap/unordered_set_of.hpp>
#include "fileOps.h"
//#include "../../../xbmc/xbmc_pvr_types.h"

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
  
  //NEW
  virtual bool Connect();
  virtual bool GetLiveTVPriority();
  virtual void SetLiveTVPriority(bool enabled);
  virtual CStdString GetArtWork(FILE_OPTIONS storageGroup, CStdString shwTitle);
  virtual bool OpenLiveStream(const PVR_CHANNEL &channel);
  virtual void CloseLiveStream();
  virtual int ReadLiveStream(unsigned char *pBuffer, unsigned int iBufferSize);
  virtual long long SeekLiveStream(long long iPosition, int iWhence);
  virtual long long LengthLiveStream();
  virtual bool SwitchChannel(const PVR_CHANNEL &channel);
  //NEW
  
  virtual int GetChannelsAmount(void);
  virtual PVR_ERROR GetChannels(ADDON_HANDLE handle, bool bRadio);
  virtual bool GetChannel(const PVR_CHANNEL &channel, PVRcmythChannel &myChannel);

  virtual int GetChannelGroupsAmount(void);
  virtual PVR_ERROR GetChannelGroups(ADDON_HANDLE handle, bool bRadio);
  virtual PVR_ERROR GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group);

  virtual PVR_ERROR GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd);

protected:
  virtual bool LoadCategoryMap(void);
private:
//  std::vector<PVRcmythChannelGroup> m_groups;
//  std::vector<PVRcmythChannel>      m_channels;
//  time_t                           m_iEpgStart;
  CStdString                       m_strDefaultIcon;
  CStdString                       m_strDefaultMovie;
  
  //NEW
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
  CMutex m_lock;
  
  //MythFile m_file;
  //std::vector< RecordingRule > m_recordingRules;
  //std::map< int , MythChannel > m_channels;
  //std::map< int, std::vector< int > > m_sources;
  //boost::unordered_map< CStdString, MythProgramInfo > m_recordings;
  //boost::unordered_map< CStdString, std::vector< int > > m_channelGroups;
  //void PVRtoMythTimer(const PVR_TIMER timer, MythTimer& mt);

};
