#pragma once
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
*  the Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
*  MA 02110-1301  USA
*  http://www.gnu.org/copyleft/gpl.html
*
*/

#include <vector>
#include "platform/util/StdString.h"
#include "client.h"
#include "Socket.h"

class Pvr2Wmc 
{
public:
	Pvr2Wmc(void);
	virtual ~Pvr2Wmc(void);

	virtual bool IsServerDown();
	virtual void UnLoading();
	const char *GetBackendVersion(void);

	// channels
	virtual int GetChannelsAmount(void);
	virtual PVR_ERROR GetChannels(ADDON_HANDLE handle, bool bRadio);
	
	virtual int GetChannelGroupsAmount(void);
	virtual PVR_ERROR GetChannelGroups(ADDON_HANDLE handle, bool bRadio);
	virtual PVR_ERROR GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group);

	// epg
	virtual PVR_ERROR GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd);
	
	// timers
	virtual PVR_ERROR GetTimers(ADDON_HANDLE handle);
	virtual PVR_ERROR AddTimer(const PVR_TIMER &timer);
	virtual PVR_ERROR DeleteTimer(const PVR_TIMER &timer, bool bForceDelete);
	virtual int GetTimersAmount(void);

	// recording files
	virtual PVR_ERROR GetRecordings(ADDON_HANDLE handle);
	PVR_ERROR DeleteRecording(const PVR_RECORDING &recording);
	PVR_ERROR RenameRecording(const PVR_RECORDING &recording);
	PVR_ERROR SetRecordingLastPlayedPosition(const PVR_RECORDING &recording, int lastplayedposition);
	int GetRecordingLastPlayedPosition(const PVR_RECORDING &recording);
	PVR_ERROR SetRecordingPlayCount(const PVR_RECORDING &recording, int count);

	// recording streams
	bool OpenRecordedStream(const PVR_RECORDING &recording);
	virtual int GetRecordingsAmount(void);
	void UpdateRecordingTimer(int msec);

	// live tv
	bool OpenLiveStream(const PVR_CHANNEL &channel);
	bool CloseLiveStream(bool notifyServer = true);
	int ReadLiveStream(unsigned char *pBuffer, unsigned int iBufferSize);
	void PauseStream(bool bPaused);
	long long SeekLiveStream(long long iPosition, int iWhence /* = SEEK_SET */) ;
	long long PositionLiveStream(void) ;
	bool SwitchChannel(const PVR_CHANNEL &channel);
	long long LengthLiveStream(void);
	long long ActualFileSize(int count);
	PVR_ERROR SignalStatus(PVR_SIGNAL_STATUS &signalStatus);
	
	bool CheckErrorOnServer();
	void TriggerUpdates(vector<CStdString> results);

private:
	int _serverBuild;
	CStdString Timer2String(const PVR_TIMER &xTmr);
	CStdString Channel2String(const PVR_CHANNEL &xTmr);

	Socket _socketClient;

	int _signalStatusCount;				// call backend for signal status every N calls (because XBMC calls every 1 second!)
	bool _discardSignalStatus;			// flag to discard signal status for channels where the backend had an error

	void* _streamFile;					// handle to a streamed file
	CStdString _streamFileName;			// the name of the stream file
	bool _lostStream;					// set to true if stream is lost
	
	bool _streamWTV;					// if true, stream wtv files
	long long _lastStreamSize;			// last value found for file stream
	bool _isStreamFileGrowing;			// true if server reports that a live/rec stream is still growing
	long long _readCnt;					// keep a count of the number of reads executed during playback

	int _initialStreamResetCnt;			// used to count how many times we reset the stream position (due to 2 pass demuxer)
	long long _initialStreamPosition;	// used to set an initial position (multiple clients watching the same live tv buffer)

	bool _insertDurationHeader;			// if true, insert a duration header for active Rec TS file
	CStdString _durationHeader;			// the header to insert (received from server)
};
