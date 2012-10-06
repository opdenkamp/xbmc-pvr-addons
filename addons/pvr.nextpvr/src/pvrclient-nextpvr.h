#pragma once
/*
 *      Copyright (C) 2005-2011 Team XBMC
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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <vector>

/* Master defines for client control */
#include "xbmc_pvr_types.h"

/* Local includes */
#include "Socket.h"
#include "platform/threads/mutex.h"
#include "RingBuffer.h"

#define SAFE_DELETE(p)       do { delete (p);     (p)=NULL; } while (0)

class cPVRClientNextPVR
{
public:
  /* Class interface */
  cPVRClientNextPVR();
  ~cPVRClientNextPVR();

  /* VTP Listening Thread */
  static void* Process(void*);

  /* Server handling */
  bool Connect();
  void Disconnect();
  bool IsUp();

  /* General handling */
  const char* GetBackendName(void);
  const char* GetBackendVersion(void);
  const char* GetConnectionString(void);
  PVR_ERROR GetDriveSpace(long long *iTotal, long long *iUsed);
  PVR_ERROR GetBackendTime(time_t *localTime, int *gmtOffset);

  /* EPG handling */
  PVR_ERROR GetEpg(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart = NULL, time_t iEnd = NULL);

  /* Channel handling */
  int GetNumChannels(void);
  PVR_ERROR GetChannels(ADDON_HANDLE handle, bool bRadio);  

  /* Channel group handling */
  int GetChannelGroupsAmount(void);
  PVR_ERROR GetChannelGroups(ADDON_HANDLE handle, bool bRadio);
  PVR_ERROR GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group);

  /* Record handling **/
  int GetNumRecordings(void);
  PVR_ERROR GetRecordings(ADDON_HANDLE handle);
  PVR_ERROR DeleteRecording(const PVR_RECORDING &recording);
  PVR_ERROR RenameRecording(const PVR_RECORDING &recording);

  /* Timer handling */
  int GetNumTimers(void);
  PVR_ERROR GetTimers(ADDON_HANDLE handle);
  PVR_ERROR GetTimerInfo(unsigned int timernumber, PVR_TIMER &timer);
  PVR_ERROR AddTimer(const PVR_TIMER &timer);
  PVR_ERROR DeleteTimer(const PVR_TIMER &timer, bool bForceDelete = false);
  PVR_ERROR UpdateTimer(const PVR_TIMER &timer);

  /* Live stream handling */
  bool OpenLiveStream(const PVR_CHANNEL &channel);
  void CloseLiveStream();
  int ReadLiveStream(unsigned char *pBuffer, unsigned int iBufferSize);
  int GetCurrentClientChannel();
  bool SwitchChannel(const PVR_CHANNEL &channel);
  PVR_ERROR SignalStatus(PVR_SIGNAL_STATUS &signalStatus);
  const char* GetLiveStreamURL(const PVR_CHANNEL &channel);
  long long SeekLiveStream(long long iPosition, int iWhence = SEEK_SET);
  long long LengthLiveStream(void);
  long long PositionLiveStream(void);

  /* Record stream handling */
  bool OpenRecordedStream(const PVR_RECORDING &recording);
  void CloseRecordedStream(void);
  int ReadRecordedStream(unsigned char *pBuffer, unsigned int iBufferSize);
  long long SeekRecordedStream(long long iPosition, int iWhence = SEEK_SET);
  long long LengthRecordedStream(void);
  long long PositionRecordedStream(void);

protected:
  NextPVR::Socket           *m_tcpclient;
  NextPVR::Socket           *m_streamingclient;

private:
  bool GetChannel(unsigned int number, PVR_CHANNEL &channeldata);
  bool LoadGenreXML(const std::string &filename);
  int DoRequest(const char *resource, CStdString &response);
  bool OpenRecordingInternal(long long seekOffset);
  CStdString GetChannelIcon(int channelID);
  void Close();

  int                     m_iCurrentChannel;
  bool                    m_bConnected;  
  std::string             m_BackendName;
  PLATFORM::CMutex        m_mutex;

  CRingBuffer			  m_incomingStreamBuffer;

  char					  m_currentRecordingID[1024];
  long long				  m_currentRecordingLength;
  long long				  m_currentRecordingPosition;

  CStdString			  m_channelIconDirectory;
  CStdString			  m_PlaybackURL;

  char					  m_sid[64];

  int					  m_iChannelCount;  
  
};
