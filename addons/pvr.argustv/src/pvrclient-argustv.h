#pragma once
/*
 *      Copyright (C) 2010-2012 Marcel Groothuis, Fred Hoogduin
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

#include "platform/os.h"

#include <vector>

/* Master defines for client control */
#include "xbmc_pvr_types.h"

#include "channel.h"
#include "recording.h"
#include "guideprogram.h"

#include "KeepAliveThread.h"

class CTsReader;

#undef ATV_DUMPTS

class cPVRClientArgusTV
{
public:
  /* Class interface */
  cPVRClientArgusTV();
  ~cPVRClientArgusTV();

  /* Server handling */
  bool Connect();
  void Disconnect();
  bool IsUp();
  bool ShareErrorsFound(void);

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
  PVR_ERROR DeleteRecording(const PVR_RECORDING &recinfo);
  PVR_ERROR RenameRecording(const PVR_RECORDING &recinfo);
  PVR_ERROR SetRecordingLastPlayedPosition(const PVR_RECORDING &recinfo, int lastplayedposition);
  int GetRecordingLastPlayedPosition(const PVR_RECORDING &recinfo);
  PVR_ERROR SetRecordingPlayCount(const PVR_RECORDING &recinfo, int playcount);

  /* Timer handling */
  int GetNumTimers(void);
  PVR_ERROR GetTimers(ADDON_HANDLE handle);
  PVR_ERROR AddTimer(const PVR_TIMER &timer);
  PVR_ERROR DeleteTimer(const PVR_TIMER &timer, bool bForceDelete = false);
  PVR_ERROR UpdateTimer(const PVR_TIMER &timer);

  /* Live stream handling */
  bool OpenLiveStream(const PVR_CHANNEL &channel);
  void CloseLiveStream();
  int ReadLiveStream(unsigned char *pBuffer, unsigned int iBufferSize);
  long long SeekLiveStream(long long pos, int whence);
  long long PositionLiveStream(void);
  long long LengthLiveStream(void);
  int GetCurrentClientChannel();
  bool SwitchChannel(const PVR_CHANNEL &channel);
  PVR_ERROR SignalStatus(PVR_SIGNAL_STATUS &signalStatus);
  bool CanPauseAndSeek(void);
  void PauseStream(bool bPaused);

  /* Record stream handling */
  bool OpenRecordedStream(const PVR_RECORDING &recording);
  void CloseRecordedStream(void);
  int ReadRecordedStream(unsigned char *pBuffer, unsigned int iBufferSize);
  long long SeekRecordedStream(long long iPosition, int iWhence = SEEK_SET);
  long long LengthRecordedStream(void);
  long long PositionRecordedStream(void);

  /* Used for rtsp streaming */
  const char* GetLiveStreamURL(const PVR_CHANNEL &channel);

private:
  cChannel* FetchChannel(int channelid, bool LogError = true);
  cChannel* FetchChannel(std::vector<cChannel*> m_Channels, int channelid, bool LogError = true);
  void FreeChannels(std::vector<cChannel*> m_Channels);
  void Close();
  bool _OpenLiveStream(const PVR_CHANNEL &channel);

  int                     m_iCurrentChannel;
  bool                    m_bConnected;
  bool                    m_bTimeShiftStarted;
  std::string             m_PlaybackURL;
  std::string             m_BackendName;
  int                     m_iBackendVersion;
  std::string             m_sBackendVersion;
  time_t                  m_BackendUTCoffset;
  time_t                  m_BackendTime;

  std::vector<cChannel*>   m_TVChannels; // Local TV channel cache list needed for id to guid conversion
  std::vector<cChannel*>   m_RadioChannels; // Local Radio channel cache list needed for id to guid conversion
  int                     m_epg_id_offset;
  int                     m_signalqualityInterval;
  CTsReader*              m_tsreader;
  CKeepAliveThread        m_keepalive;
#if defined(ATV_DUMPTS)
  char ofn[25];
  int ofd;
#endif

};
